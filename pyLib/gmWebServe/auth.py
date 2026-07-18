"""auth — pilot-grade authentication for gmWebServe-based eng_serve gateways.

Provides everything a FastAPI ``eng_serve`` app needs so a small, fixed set of
users can log in and receive an opaque bearer token used to isolate their game
sessions from every other user's:

- Password hashing: PBKDF2-HMAC-SHA256 with a random per-user salt (OWASP
  Password Storage Cheat Sheet, 2023 revision) — plaintext passwords are
  never stored, logged, or transmitted back.
- :class:`AuthConfig` — loads a small JSON file of fixed users plus the
  per-user session limits they are governed by (see
  ``gmWebServe.tools.manage_users`` to create/update it without ever
  hand-computing a password hash).
- :class:`TokenManager` / :class:`AuthService` — issue short-lived opaque
  tokens (``secrets.token_urlsafe``) and resolve them back to a username.

Deliberately NOT included (out of scope for "pilot-grade"): registration,
password reset, refresh tokens, login rate limiting/lockout. Revisit before
any real production deployment.
"""
from __future__ import annotations

import hashlib
import hmac
import json
import secrets
import time
from dataclasses import dataclass, field
from pathlib import Path

__all__ = [
    "DEFAULT_MAX_SESSIONS_PER_USER",
    "DEFAULT_SESSION_IDLE_TIMEOUT_S",
    "DEFAULT_TOKEN_TTL_S",
    "hash_password",
    "verify_password",
    "UserAccount",
    "AuthConfig",
    "AuthSession",
    "TokenManager",
    "AuthService",
    "InvalidCredentialsError",
    "InvalidTokenError",
]

DEFAULT_MAX_SESSIONS_PER_USER = 2
DEFAULT_SESSION_IDLE_TIMEOUT_S = 600.0
DEFAULT_TOKEN_TTL_S = 8 * 3600.0

_PBKDF2_ALGORITHM = "sha256"
_PBKDF2_ITERATIONS = 600_000
_SALT_BYTES = 16


class InvalidCredentialsError(RuntimeError):
    """Raised when a login attempt has an unknown username or a wrong password."""


class InvalidTokenError(RuntimeError):
    """Raised when a bearer token is missing from the store, or has expired."""


def hash_password(password: str, salt: bytes | None = None) -> tuple[str, str]:
    """Hashes *password* with PBKDF2-HMAC-SHA256, returning ``(hash_hex, salt_hex)``.

    Args:
        password: Plaintext password to hash. Never stored or logged.
        salt: Existing 16-byte salt to reuse (as raw bytes). When omitted, a
            new random salt is generated with ``secrets.token_bytes``.

    Returns:
        Tuple of ``(password_hash_hex, password_salt_hex)``, both safe to
        persist to disk/JSON.
    """
    if salt is None:
        salt = secrets.token_bytes(_SALT_BYTES)
    digest = hashlib.pbkdf2_hmac(
        _PBKDF2_ALGORITHM, password.encode("utf-8"), salt, _PBKDF2_ITERATIONS
    )
    return digest.hex(), salt.hex()


def verify_password(password: str, password_hash: str, password_salt: str) -> bool:
    """Returns True if *password* matches the stored *password_hash*/*password_salt*.

    Args:
        password: Plaintext candidate password.
        password_hash: Previously stored hash (hex), from :func:`hash_password`.
        password_salt: Previously stored salt (hex), from :func:`hash_password`.

    Uses :func:`hmac.compare_digest` for a constant-time comparison so the
    check itself does not leak timing information about the stored hash.
    """
    candidate_hash, _ = hash_password(password, bytes.fromhex(password_salt))
    return hmac.compare_digest(candidate_hash, password_hash)


@dataclass(frozen=True)
class UserAccount:
    """One pilot-grade user account: a username plus its hashed password."""

    username: str
    password_hash: str
    password_salt: str


@dataclass
class AuthConfig:
    """Fixed user registry plus the per-user session limits it governs.

    JSON schema (see ``gmWebServe.tools.manage_users`` to create/update this
    file instead of hand-computing a password hash)::

        {
          "users": [
            {"username": "demo", "password_hash": "...", "password_salt": "..."}
          ],
          "max_sessions_per_user": 2,
          "session_idle_timeout_seconds": 600
        }
    """

    users: dict[str, UserAccount] = field(default_factory=dict)
    max_sessions_per_user: int = DEFAULT_MAX_SESSIONS_PER_USER
    session_idle_timeout_seconds: float = DEFAULT_SESSION_IDLE_TIMEOUT_S

    @classmethod
    def load(cls, path: Path) -> AuthConfig:
        """Loads the JSON config at *path* (see class docstring for schema).

        Raises:
            FileNotFoundError: If *path* does not exist.
            ValueError: If the file is not valid JSON or a user entry is
                missing a required field.
        """
        raw = json.loads(path.read_text(encoding="utf-8"))
        try:
            users = {
                entry["username"]: UserAccount(
                    username=entry["username"],
                    password_hash=entry["password_hash"],
                    password_salt=entry["password_salt"],
                )
                for entry in raw.get("users", [])
            }
        except KeyError as exc:
            raise ValueError(f"Malformed user entry in {path}: missing {exc}") from exc

        return cls(
            users=users,
            max_sessions_per_user=int(
                raw.get("max_sessions_per_user", DEFAULT_MAX_SESSIONS_PER_USER)
            ),
            session_idle_timeout_seconds=float(
                raw.get("session_idle_timeout_seconds", DEFAULT_SESSION_IDLE_TIMEOUT_S)
            ),
        )

    def verify_credentials(self, username: str, password: str) -> bool:
        """Returns True if *username*/*password* match a known account.

        Runs a dummy hash even when *username* is unknown, so failing on a bad
        username takes roughly the same time as failing on a bad password —
        a pilot-grade mitigation (not a complete fix) against
        username-enumeration via response-timing.
        """
        account = self.users.get(username)
        if account is None:
            hash_password(password)
            return False
        return verify_password(password, account.password_hash, account.password_salt)


@dataclass
class AuthSession:
    """One issued bearer token: who it belongs to and when it expires."""

    token: str
    username: str
    expires_at: float


class TokenManager:
    """In-memory opaque bearer-token issuance, validation and revocation.

    Not persisted: every token is invalidated by an ``eng_serve`` process
    restart, which is acceptable for a pilot-grade deployment (users simply
    log in again).
    """

    def __init__(self, ttl_seconds: float = DEFAULT_TOKEN_TTL_S) -> None:
        self._ttl_seconds: float = ttl_seconds
        self._sessions: dict[str, AuthSession] = {}

    def issue(self, username: str) -> AuthSession:
        """Creates, stores and returns a new random token for *username*."""
        token = secrets.token_urlsafe(32)
        session = AuthSession(
            token=token, username=username, expires_at=time.time() + self._ttl_seconds
        )
        self._sessions[token] = session
        return session

    def resolve(self, token: str) -> str:
        """Returns the username owning *token*.

        Raises:
            InvalidTokenError: If *token* is unknown or has expired (an
                expired entry is evicted from the store here).
        """
        session = self._sessions.get(token)
        if session is None:
            raise InvalidTokenError("Token sconosciuto")
        if session.expires_at < time.time():
            del self._sessions[token]
            raise InvalidTokenError("Token scaduto")
        return session.username

    def revoke(self, token: str) -> None:
        """Removes *token* from the store, if present (idempotent)."""
        self._sessions.pop(token, None)


class AuthService:
    """Combines :class:`AuthConfig` (who can log in) with :class:`TokenManager`
    (what a logged-in caller presents on every later request).

    Every ``eng_serve`` app is expected to create exactly one instance at
    startup and publish it as ``app.state.auth_service`` — see
    :mod:`gmWebServe.fastapi_deps` and :mod:`gmWebServe.auth_router`, which
    both read it from there.
    """

    def __init__(self, config: AuthConfig, token_ttl_seconds: float = DEFAULT_TOKEN_TTL_S) -> None:
        self._config: AuthConfig = config
        self._tokens: TokenManager = TokenManager(token_ttl_seconds)

    @property
    def max_sessions_per_user(self) -> int:
        """Per-user cap on concurrent game sessions, from the loaded config."""
        return self._config.max_sessions_per_user

    @property
    def session_idle_timeout_seconds(self) -> float:
        """Idle-eviction timeout for game sessions, from the loaded config."""
        return self._config.session_idle_timeout_seconds

    def login(self, username: str, password: str) -> AuthSession:
        """Verifies credentials and issues a new bearer token.

        Raises:
            InvalidCredentialsError: If the username/password do not match.
        """
        if not self._config.verify_credentials(username, password):
            raise InvalidCredentialsError("Credenziali non valide")
        return self._tokens.issue(username)

    def logout(self, token: str) -> None:
        """Revokes *token*, if present (idempotent)."""
        self._tokens.revoke(token)

    def authenticate(self, token: str) -> str:
        """Returns the username owning *token*.

        Raises:
            InvalidTokenError: If *token* is missing, unknown, or expired.
        """
        return self._tokens.resolve(token)
