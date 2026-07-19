"""test_session_manager — unit tests for `SessionManager._auto_resolve_monster_events`.

No engine subprocess needed: exercises the pure decision logic (given an
envelope + a session's `roles`, does it auto-answer on the monsters'
behalf?) with lightweight stand-ins instead of a real `GameSession`/engine
`sender` — see `test_session_e2e.py` for the real-engine round trip proof.
"""
from __future__ import annotations

from types import SimpleNamespace

from eng_serve.session_manager import SessionManager


def _session(roles: tuple[str, ...]):
    """A minimal stand-in exposing just what `_auto_resolve_monster_events` reads."""
    sender = SimpleNamespace(sent=[])
    sender.send_command = lambda type_id, data: sender.sent.append((type_id, data))
    return SimpleNamespace(roles=roles, sender=sender)


def test_reaction_window_for_monster_defender_is_auto_taken() -> None:
    session = _session(("thael", "velyr"))
    envelope = {
        "typeId": "eldhom.reaction.window_opened",
        "data": {"attacker_id": "thael", "defender_id": "brigante_A1", "incoming_damage": 1},
    }
    SessionManager._auto_resolve_monster_events(session, envelope)
    assert session.sender.sent == [
        ("eldhom.react_defense", {"defender_id": "brigante_A1", "reaction": "TAKE"})
    ]


def test_reaction_window_for_hero_defender_is_left_alone() -> None:
    """A monster attacking a PG must NOT be auto-resolved — a real user answers."""
    session = _session(("thael", "velyr"))
    envelope = {
        "typeId": "eldhom.reaction.window_opened",
        "data": {"attacker_id": "brigante_A1", "defender_id": "velyr", "incoming_damage": 1},
    }
    SessionManager._auto_resolve_monster_events(session, envelope)
    assert session.sender.sent == []


def test_formation_dialog_for_all_monster_actors_is_auto_resolved_empty_backline() -> None:
    session = _session(("thael", "velyr"))
    envelope = {
        "typeId": "eldhom.formation.dialog_needed",
        "data": {
            "location_id": "corridoio",
            "faction_id": "BRIGANTI",
            "source": "scompaginamento",
            "actors": [
                {"actor_id": "brigante_A1", "name": "Brigante", "in_backline": False},
                {"actor_id": "brigante_A2", "name": "Brigante", "in_backline": True},
            ],
        },
    }
    SessionManager._auto_resolve_monster_events(session, envelope)
    assert session.sender.sent == [
        (
            "eldhom.resolve_formation",
            {"faction_id": "BRIGANTI", "location_id": "corridoio", "backline": []},
        )
    ]


def test_formation_dialog_with_a_hero_actor_is_left_alone() -> None:
    """A formation dialog affecting the HERO faction must be left for a real user."""
    session = _session(("thael", "velyr"))
    envelope = {
        "typeId": "eldhom.formation.dialog_needed",
        "data": {
            "location_id": "ingresso",
            "faction_id": "HEROES",
            "source": "scompaginamento",
            "actors": [
                {"actor_id": "thael", "name": "Thael", "in_backline": False},
                {"actor_id": "velyr", "name": "Velyr", "in_backline": True},
            ],
        },
    }
    SessionManager._auto_resolve_monster_events(session, envelope)
    assert session.sender.sent == []


def test_unrelated_event_types_are_ignored() -> None:
    session = _session(("thael", "velyr"))
    SessionManager._auto_resolve_monster_events(
        session, {"typeId": "eldhom.state.full", "data": {}}
    )
    assert session.sender.sent == []
