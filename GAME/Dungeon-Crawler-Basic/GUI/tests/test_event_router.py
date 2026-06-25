"""Unit tests for EventRouter."""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "GUI"))

from app.event_router import EventRouter  # noqa: E402


def test_register_and_dispatch():
    router = EventRouter()
    received = []
    router.register("dungeon.actor.moved", lambda msg: received.append(msg))
    msg = {"typeId": "dungeon.actor.moved", "data": {"actor_id": "hero", "to": "room_2"}}
    router.dispatch(msg)
    assert len(received) == 1
    assert received[0]["data"]["actor_id"] == "hero"
    print("  [OK] test_register_and_dispatch")


def test_unknown_typeid_ignored():
    router = EventRouter()
    received = []
    router.register("dungeon.actor.moved", lambda msg: received.append(msg))
    router.dispatch({"typeId": "unknown.event", "data": {}})
    assert len(received) == 0
    print("  [OK] test_unknown_typeid_ignored")


def test_multiple_handlers_same_typeid():
    router = EventRouter()
    log: list[int] = []
    router.register("ev", lambda _: log.append(1))
    router.register("ev", lambda _: log.append(2))
    router.dispatch({"typeId": "ev", "data": {}})
    assert log == [1, 2]
    print("  [OK] test_multiple_handlers_same_typeid")


def test_dispatch_missing_typeid():
    router = EventRouter()
    # Should not raise even if typeId is missing
    router.dispatch({})
    print("  [OK] test_dispatch_missing_typeid")


if __name__ == "__main__":
    print("=== EventRouter unit tests ===")
    test_register_and_dispatch()
    test_unknown_typeid_ignored()
    test_multiple_handlers_same_typeid()
    test_dispatch_missing_typeid()
    print("All EventRouter tests PASSED.")
