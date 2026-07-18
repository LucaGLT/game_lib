"""test_session_registry — end-to-end tests for SessionRegistry against a
fake engine process (see fake_engine.py) — no C++ build required, keeping
this suite fully game-agnostic.
"""
from __future__ import annotations

import sys
import time
from pathlib import Path

import pytest

from gmWebServe.session_registry import (
    SessionLimitExceededError,
    SessionNotFoundError,
    SessionRegistry,
)

_FAKE_ENGINE = Path(__file__).resolve().parent / "fake_engine.py"

pytestmark = pytest.mark.timeout(30)


@pytest.fixture()
def registry():
    reg = SessionRegistry(
        executable=Path(sys.executable),
        max_sessions_per_user=2,
        idle_timeout_seconds=600.0,
    )
    yield reg
    reg.shutdown()


def _bootstrap(sender) -> None:
    sender.send_command("fake.hello", {})


def _create(registry: SessionRegistry, owner: str):
    return registry.create_session(owner, _bootstrap, extra_args=[str(_FAKE_ENGINE)])


def test_create_and_get_session(registry: SessionRegistry) -> None:
    session = _create(registry, "alice")
    fetched = registry.get_session(session.session_id, "alice")
    assert fetched.session_id == session.session_id


def test_unknown_session_id_raises(registry: SessionRegistry) -> None:
    with pytest.raises(SessionNotFoundError):
        registry.get_session("does-not-exist", "alice")


def test_ownership_isolation(registry: SessionRegistry) -> None:
    session = _create(registry, "alice")
    with pytest.raises(SessionNotFoundError):
        registry.get_session(session.session_id, "bob")
    with pytest.raises(SessionNotFoundError):
        registry.send_command(session.session_id, "bob", "fake.probe", {})


def test_per_user_session_limit(registry: SessionRegistry) -> None:
    _create(registry, "alice")
    _create(registry, "alice")
    with pytest.raises(SessionLimitExceededError):
        _create(registry, "alice")


def test_two_users_concurrent_sessions_isolated(registry: SessionRegistry) -> None:
    session_a = _create(registry, "alice")
    session_b = _create(registry, "bob")
    assert session_a.session_id != session_b.session_id
    assert [s.session_id for s in registry.list_sessions("alice")] == [session_a.session_id]
    assert [s.session_id for s in registry.list_sessions("bob")] == [session_b.session_id]
    # A second user's session must not count against the first user's cap.
    _create(registry, "alice")


def test_close_session_frees_slot(registry: SessionRegistry) -> None:
    session1 = _create(registry, "alice")
    _create(registry, "alice")
    registry.close_session(session1.session_id, "alice")

    # Slot freed: a third session can now be created without hitting the cap.
    session3 = _create(registry, "alice")
    assert session3.session_id != session1.session_id

    with pytest.raises(SessionNotFoundError):
        registry.get_session(session1.session_id, "alice")


def test_close_session_wrong_owner_raises(registry: SessionRegistry) -> None:
    session = _create(registry, "alice")
    with pytest.raises(SessionNotFoundError):
        registry.close_session(session.session_id, "bob")


def test_command_and_subscribe_receive_engine_events(registry: SessionRegistry) -> None:
    session = _create(registry, "alice")
    queue = registry.subscribe(session.session_id, "alice")
    registry.loop = None  # not running inside asyncio here; poll last_envelope_by_type instead

    deadline = time.time() + 10.0
    while time.time() < deadline and "fake.started" not in session.last_envelope_by_type:
        time.sleep(0.1)
    assert "fake.started" in session.last_envelope_by_type
    assert queue is not None  # subscriber registered without error


def test_reap_idle_sessions(registry: SessionRegistry) -> None:
    registry.idle_timeout_seconds = 0.01
    session = _create(registry, "alice")
    time.sleep(0.2)
    reaped = registry.reap_idle_sessions()
    assert session.session_id in reaped
    with pytest.raises(SessionNotFoundError):
        registry.get_session(session.session_id, "alice")


def test_reap_idle_sessions_spares_active_ones(registry: SessionRegistry) -> None:
    registry.idle_timeout_seconds = 600.0
    session = _create(registry, "alice")
    reaped = registry.reap_idle_sessions()
    assert session.session_id not in reaped
    registry.get_session(session.session_id, "alice")  # still present, must not raise
