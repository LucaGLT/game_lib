"""Manual mock engine for gmGui visual sandbox.

Roles mirrored from the real bridge:
- Event stream: TCP client to EngineReceiver (GUI listening socket).
- Command intake: TCP server for EngineSender (GUI command socket).

Default mode is manual: turns advance only when GUI sends
"gmFlow.turn.pass" from the Flow/Timeline panel.
"""
from __future__ import annotations

import argparse
import json
import socket
import time
from datetime import datetime, timezone
from typing import Any

from gmGui.engine_bridge.framing import recv_frame, send_frame

HOST = "127.0.0.1"
EVENT_PORT = 9000
COMMAND_PORT = 9001


def _env(type_id: str, data: dict[str, Any]) -> dict[str, Any]:
    return {
        "typeId": type_id,
        "source": "MockEngine",
        "time": datetime.now(timezone.utc).isoformat(),
        "data": data,
    }


def _send_event(sock: socket.socket, type_id: str, data: dict[str, Any]) -> None:
    payload = json.dumps(_env(type_id, data), ensure_ascii=True)
    send_frame(sock, payload)


def _connect_event_stream(host: str, port: int, timeout_s: float = 20.0) -> socket.socket:
    deadline = time.time() + timeout_s
    last_err: Exception | None = None
    while time.time() < deadline:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((host, port))
            return sock
        except OSError as exc:
            last_err = exc
            try:
                sock.close()
            except OSError:
                pass
            time.sleep(0.2)
    raise ConnectionError(
        f"[mock_engine] impossibile connettersi al receiver {host}:{port} entro {timeout_s:.1f}s ({last_err})"
    )


def _emit_initial_snapshot(event_sock: socket.socket) -> None:
    _send_event(event_sock, "gmFlow.session.started", {"session_id": "sandbox_01"})
    _send_event(event_sock, "gmFlow.phase.entered", {"phase_id": "SETUP"})
    _send_event(event_sock, "gmFlow.round.started", {"index": 1})
    _send_event(
        event_sock,
        "gmFlow.turn.started",
        {"turn_id": "TURN_1", "active_actors": ["Player_X"]},
    )

    _send_event(
        event_sock,
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
    )

    _send_event(
        event_sock,
        "gmAlea.deck.zone_changed",
        {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "fire_01", "name": "Fireball"},
                {"card_id": "shield_01", "name": "Shield"},
                {"card_id": "heal_01", "name": "Heal"},
            ],
        },
    )
    _send_event(
        event_sock,
        "gmAlea.deck.zone_changed",
        {
            "zone_name": "CardHand",
            "cards": [
                {"card_id": "potion_01", "name": "Potion"},
                {"card_id": "arrow_01", "name": "Arrow"},
            ],
        },
    )
    _send_event(event_sock, "gmAlea.deck.zone_changed", {"zone_name": "PlayArea", "cards": []})
    _send_event(event_sock, "gmAlea.deck.zone_changed", {"zone_name": "DiscardPile", "cards": []})
    _send_event(event_sock, "gmAlea.deck.zone_changed", {"zone_name": "BanishZone", "cards": []})

    _send_event(
        event_sock,
        "gmAlea.dice.setup",
        {
            "mode": "standard",
            "count": 2,
            "faces": 6,
            "locked": False,
            "result": {"dice": [4, 2], "total": 6},
        },
    )


def _advance_turn(event_sock: socket.socket, turn_index: int) -> None:
    active = "Player_X" if turn_index % 2 == 1 else "Player_O"

    _send_event(event_sock, "gmFlow.phase.entered", {"phase_id": "PLAYER_TURN"})
    _send_event(event_sock, "gmFlow.round.started", {"index": turn_index})
    _send_event(
        event_sock,
        "gmFlow.turn.started",
        {"turn_id": f"TURN_{turn_index}", "active_actors": [active]},
    )
    _send_event(
        event_sock,
        "gmActor.actor.status_added",
        {"actor_id": active, "status_id": "ACTIVE_TURN", "stacks": 1},
    )

    if turn_index <= 3:
        card_id = ["fire_01", "shield_01", "heal_01"][turn_index - 1]
        _send_event(
            event_sock,
            "gmAlea.deck.card_moved",
            {
                "card_id": card_id,
                "from_zone": "MainDeck",
                "to_zone": "CardHand",
            },
        )

    dice_a = (turn_index % 6) + 1
    dice_b = ((turn_index + 2) % 6) + 1
    _send_event(
        event_sock,
        "gmAlea.dice.roll_result",
        {"dice": [dice_a, dice_b], "total": dice_a + dice_b},
    )


def _serve_commands(host: str, cmd_port: int, event_sock: socket.socket) -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, cmd_port))
    server.listen(1)
    print(f"[mock_engine] CmdServer in ascolto su {host}:{cmd_port}")

    turn_index = 1
    paused = False

    try:
        while True:
            client, addr = server.accept()
            client.settimeout(1.0)
            print(f"[mock_engine] GUI command channel connesso da {addr[0]}:{addr[1]}")
            try:
                while True:
                    raw = recv_frame(client)
                    try:
                        cmd = json.loads(raw)
                    except json.JSONDecodeError:
                        continue

                    type_id = str(cmd.get("typeId", ""))
                    if type_id == "gmFlow.turn.pass":
                        if paused:
                            print("[mock_engine] turn.pass ignorato: sessione in pausa")
                            continue
                        turn_index += 1
                        _advance_turn(event_sock, turn_index)
                        print(f"[mock_engine] avanzato a TURN_{turn_index}")
                    elif type_id == "gmFlow.session.pause":
                        paused = True
                        _send_event(event_sock, "gmFlow.session.paused", {})
                    elif type_id == "gmFlow.session.resume":
                        paused = False
                        _send_event(event_sock, "gmFlow.session.resumed", {})
                    elif type_id == "gmFlow.session.stop":
                        _send_event(event_sock, "gmFlow.session.completed", {})
            except (ConnectionError, OSError):
                print("[mock_engine] GUI command channel disconnesso")
            finally:
                try:
                    client.close()
                except OSError:
                    pass
    finally:
        try:
            server.close()
        except OSError:
            pass


def main() -> int:
    parser = argparse.ArgumentParser(description="Mock event producer for gmGui sandbox")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=EVENT_PORT, help="Porta eventi GUI (receiver)")
    parser.add_argument(
        "--cmd-port",
        type=int,
        default=COMMAND_PORT,
        help="Porta comandi in ingresso dalla GUI (sender)",
    )
    args = parser.parse_args()

    print(f"[mock_engine] Connessione stream eventi a {args.host}:{args.port} ...")
    with _connect_event_stream(args.host, args.port, timeout_s=20.0) as event_sock:
        print("[mock_engine] Connesso. Invio snapshot iniziale...")
        _emit_initial_snapshot(event_sock)
        print("[mock_engine] Modalita manuale: usa 'Passa Turno' in Flow/Timeline.")
        _serve_commands(args.host, args.cmd_port, event_sock)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
