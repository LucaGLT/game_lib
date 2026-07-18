"""test_auth — unit tests for gmWebServe.auth (hashing, tokens, login flow)."""
from __future__ import annotations

import json
import time
from pathlib import Path

import pytest

from gmWebServe.auth import (
    AuthConfig,
    AuthService,
    InvalidCredentialsError,
    InvalidTokenError,
    UserAccount,
    hash_password,
    verify_password,
)

pytestmark = pytest.mark.timeout(10)


def test_hash_and_verify_password_roundtrip() -> None:
    password_hash, password_salt = hash_password("Sup3rSecret!")
    assert verify_password("Sup3rSecret!", password_hash, password_salt) is True
    assert verify_password("wrong-password", password_hash, password_salt) is False


def test_hash_password_uses_random_salt_by_default() -> None:
    hash1, salt1 = hash_password("same-password")
    hash2, salt2 = hash_password("same-password")
    assert salt1 != salt2
    assert hash1 != hash2


def test_hash_password_reuses_given_salt() -> None:
    _, salt_hex = hash_password("same-password")
    hash1, salt1 = hash_password("same-password", bytes.fromhex(salt_hex))
    hash2, salt2 = hash_password("same-password", bytes.fromhex(salt_hex))
    assert salt1 == salt2 == salt_hex
    assert hash1 == hash2


def test_auth_config_load_and_verify(tmp_path: Path) -> None:
    password_hash, password_salt = hash_password("demo-pass")
    config_path = tmp_path / "auth_config.json"
    config_path.write_text(
        json.dumps(
            {
                "users": [
                    {"username": "demo", "password_hash": password_hash, "password_salt": password_salt}
                ],
                "max_sessions_per_user": 3,
                "session_idle_timeout_seconds": 120,
            }
        ),
        encoding="utf-8",
    )

    config = AuthConfig.load(config_path)
    assert config.max_sessions_per_user == 3
    assert config.session_idle_timeout_seconds == 120
    assert config.verify_credentials("demo", "demo-pass") is True
    assert config.verify_credentials("demo", "wrong") is False
    assert config.verify_credentials("nobody", "anything") is False


def test_auth_service_login_logout_and_authenticate() -> None:
    service = AuthService(_config_with_one_user("demo", "demo-pass"))

    with pytest.raises(InvalidCredentialsError):
        service.login("demo", "wrong-password")

    session = service.login("demo", "demo-pass")
    assert service.authenticate(session.token) == "demo"

    service.logout(session.token)
    with pytest.raises(InvalidTokenError):
        service.authenticate(session.token)


def test_auth_service_unknown_username_rejected() -> None:
    service = AuthService(_config_with_one_user("demo", "demo-pass"))
    with pytest.raises(InvalidCredentialsError):
        service.login("nobody", "anything")


def test_auth_service_token_expiry() -> None:
    service = AuthService(_config_with_one_user("demo", "demo-pass"), token_ttl_seconds=0.05)
    session = service.login("demo", "demo-pass")
    time.sleep(0.2)
    with pytest.raises(InvalidTokenError):
        service.authenticate(session.token)


def test_auth_service_logout_is_idempotent() -> None:
    service = AuthService(_config_with_one_user("demo", "demo-pass"))
    session = service.login("demo", "demo-pass")
    service.logout(session.token)
    service.logout(session.token)  # must not raise


def _config_with_one_user(username: str, password: str) -> AuthConfig:
    password_hash, password_salt = hash_password(password)
    account = UserAccount(username=username, password_hash=password_hash, password_salt=password_salt)
    return AuthConfig(users={username: account}, max_sessions_per_user=2, session_idle_timeout_seconds=600)
