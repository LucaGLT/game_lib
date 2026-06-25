"""Minimal mock engine for gmGui visual sandbox.

Sends synthetic envelopes to gmGui EngineReceiver (TCP 127.0.0.1:9000)
using the same length-prefixed framing protocol used by gmDispatch.
"""
from __future__ import annotations

import argparse
import json
import socket
import struct
import time
from datetime import datetime, timezone
from typing import Any

HOST = "127.0.0.1"
EVENT_PORT = 9000


def _send_frame(sock: socket.socket, payload: dict[str, Any]) -> None:
    raw = json.dumps(payload, ensure_ascii=True).encode("utf-8")
    frame = struct.pack(">I", len(raw)) + raw
    sock.sendall(frame)


def _env(type_id: str, data: dict[str, Any]) -> dict[str, Any]:
    return {
        "typeId": type_id,
        "source": "MockEngine",
        "time": datetime.now(timezone.utc).isoformat(),
        "data": data,
    }


def _emit_initial_snapshot(sock: socket.socket) -> None:
    # Flow
    _send_frame(sock, _env("gmFlow.session.started", {"session_id": "sandbox_01"}))
    _send_frame(sock, _env("gmFlow.phase.entered", {"phase_id": "SETUP"}))
    _send_frame(sock, _env("gmFlow.round.started", {"index": 1}))
    _send_frame(
        sock,
        _env(
            "gmFlow.turn.started",
            {"turn_id": "TURN_1", "active_actors": ["Player_X"]},
        ),
    )

    # Actor
    _send_frame(
        sock,
        _env(
            "gmActor.snapshot",
            {
                "actors": [
                    {
                        "actor_id": "Player_X",
                        "name": "Player X",
                        "faction_id": "team_x",
                        "current_hp": 3,
                        "max_hp": 3,
                        "life_state": "ALIVE",
                        "statuses": {"ACTIVE_TURN": 1},
                        "equipment": {"hand": "fire_01"},
                        "area_id": "arena",
                    },
                    {
                        "actor_id": "Player_O",
                        "name": "Player O",
                        "faction_id": "team_o",
                        "current_hp": 3,
                        "max_hp": 3,
                        "life_state": "ALIVE",
                        "statuses": {},
                        "equipment": {},
                        "area_id": "arena",
                    },
                ]
            },
        ),
    )

    # CompDeck zones
    _send_frame(
        sock,
        _env(
            "gmAlea.deck.zone_changed",
            {
                "zone_name": "MainDeck",
                "cards": [
                    {"card_id": "fire_01", "name": "Fireball"},
                    {"card_id": "shield_01", "name": "Shield"},
                    {"card_id": "heal_01", "name": "Heal"},
                ],
            },
        ),
    )
    _send_frame(
        sock,
        _env(
            "gmAlea.deck.zone_changed",
            {
                "zone_name": "CardHand",
                "cards": [
                    {"card_id": "potion_01", "name": "Potion"},
                    {"card_id": "arrow_01", "name": "Arrow"},
                ],
            },
        ),
    )
    _send_frame(
        sock,
        _env("gmAlea.deck.zone_changed", {"zone_name": "PlayArea", "cards": []}),
    )
    _send_frame(
        sock,
        _env("gmAlea.deck.zone_changed", {"zone_name": "DiscardPile", "cards": []}),
    )
    _send_frame(
        sock,
        _env("gmAlea.deck.zone_changed", {"zone_name": "BanishZone", "cards": []}),
    )

    # Dice
    _send_frame(
        sock,
        _env(
            "gmAlea.dice.setup",
            {
                "mode": "standard",
                "count": 2,
                "faces": 6,
                "locked": False,
                "result": {"dice": [4, 2], "total": 6},
            },
        ),
    )


def _emit_loop(sock: socket.socket, interval_s: float) -> None:
    turn = 1
    while True:
        time.sleep(interval_s)
        turn += 1

        active = "Player_X" if turn % 2 == 1 else "Player_O"
        _send_frame(sock, _env("gmFlow.phase.entered", {"phase_id": "PLAYER_TURN"}))
        _send_frame(sock, _env("gmFlow.round.started", {"index": turn}))
        _send_frame(
            sock,
            _env("gmFlow.turn.started", {"turn_id": f"TURN_{turn}", "active_actors": [active]}),
        )

        _send_frame(
            sock,
            _env(
                "gmActor.actor.status_added",
                {"actor_id": active, "status_id": "ACTIVE_TURN", "stacks": 1},
            ),
        )

        # Move one card MainDeck -> CardHand every loop when available.
        if turn <= 3:
            card_id = ["fire_01", "shield_01", "heal_01"][turn - 1]
            _send_frame(
                sock,
                _env(
                    "gmAlea.deck.card_moved",
                    {
                        "card_id": card_id,
                        "from_zone": "MainDeck",
                        "to_zone": "CardHand",
                    },
                ),
            )

        dice_a = (turn % 6) + 1
        dice_b = ((turn + 2) % 6) + 1
        _send_frame(
            sock,
            _env(
                "gmAlea.dice.roll_result",
                {"dice": [dice_a, dice_b], "total": dice_a + dice_b},
            ),
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Mock event producer for gmGui sandbox")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=EVENT_PORT)
    parser.add_argument("--interval", type=float, default=1.8)
    args = parser.parse_args()

    print(f"[mock_engine] Connecting to {args.host}:{args.port} ...")
    with socket.create_connection((args.host, args.port), timeout=15.0) as sock:
        print("[mock_engine] Connected. Sending initial snapshot...")
        _emit_initial_snapshot(sock)
        print("[mock_engine] Streaming periodic updates. Press Ctrl+C to stop.")
        _emit_loop(sock, args.interval)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
