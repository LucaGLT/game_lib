"""manage_users — CLI to create/update pilot-grade users in an auth_config.json.

Never writes a plaintext password to disk — only its PBKDF2 hash + salt (see
``gmWebServe.auth.hash_password``). Creates the config file (with default
session limits) if it does not exist yet; otherwise updates the named user's
password in place, leaving every other user and the session limits untouched.

Usage (run from the ``pyLib`` directory, so ``gmWebServe`` resolves as a
package)::

    cd pyLib
    python -m gmWebServe.tools.manage_users \\
        --config "../GAME/Tic-Tac-Toe/WebApp/eng_serve/auth_config.json" \\
        --username demo --password "NewPassword123!"
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from ..auth import DEFAULT_MAX_SESSIONS_PER_USER, DEFAULT_SESSION_IDLE_TIMEOUT_S, hash_password

__all__ = ["upsert_user"]


def upsert_user(config_path: Path, username: str, password: str) -> None:
    """Adds *username* to the config at *config_path*, or updates its password.

    Args:
        config_path: Path to the JSON auth config (created if missing).
        username: Account to add/update.
        password: New plaintext password (hashed before writing, never stored).
    """
    if config_path.exists():
        raw = json.loads(config_path.read_text(encoding="utf-8"))
    else:
        raw = {
            "users": [],
            "max_sessions_per_user": DEFAULT_MAX_SESSIONS_PER_USER,
            "session_idle_timeout_seconds": DEFAULT_SESSION_IDLE_TIMEOUT_S,
        }

    password_hash, password_salt = hash_password(password)
    remaining_users = [u for u in raw.get("users", []) if u.get("username") != username]
    remaining_users.append(
        {"username": username, "password_hash": password_hash, "password_salt": password_salt}
    )
    raw["users"] = remaining_users

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(json.dumps(raw, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    """CLI entry point: parses argv and calls :func:`upsert_user`."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True, type=Path, help="Path to auth_config.json")
    parser.add_argument("--username", required=True, help="Account to add/update")
    parser.add_argument("--password", required=True, help="New plaintext password")
    args = parser.parse_args()

    upsert_user(args.config, args.username, args.password)
    print(f"User '{args.username}' saved to {args.config}")


if __name__ == "__main__":
    main()
