"""Manual mock engine for gmGui visual sandbox.

Roles mirrored from the real bridge:
- Event stream: TCP client to EngineReceiver (GUI listening socket).
- Command intake: TCP server for EngineSender (GUI command socket).

Default mode is manual: turns advance only when GUI sends
"gmFlow.turn.pass" from the Flow/Timeline panel.

Use --cards to load card definitions from a JSON file instead of the
built-in hard-coded deck.  The JSON format is documented in
``data/cards_dominion.json``.  When a card with a non-empty
``rule_group_id`` enters or leaves an active zone (PlayArea, Memory),
the mock engine simulates what ``CardRuleBridge`` + ``RuleGroupRegistry``
would do in C++: it emits ``gmRules.rule_group.activated`` /
``gmRules.rule_group.deactivated`` events and prints a coloured log line.
"""
from __future__ import annotations

import argparse
import json
import os
import random
import socket
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from gmGui.engine_bridge.framing import recv_frame, send_frame

HOST = "127.0.0.1"
EVENT_PORT = 9000
COMMAND_PORT = 9001

# Zones that count as "card in active play" — mirrors CardRuleBridge logic.
_ACTIVE_ZONES: frozenset[str] = frozenset({"PlayArea", "Memory"})


# ────────────────────────────────────────────────────────────────────────────────
# JSON card loading
# ────────────────────────────────────────────────────────────────────────────────


def _load_cards_json(
    path: str,
) -> tuple[dict[str, dict[str, Any]], dict[str, list[dict[str, Any]]]]:
    """Load card definitions from a JSON file.

    Returns:
        card_meta  — dict[card_id, {name, value, rule_group_id, ...}]
        deck_state — initial zone distribution
    """
    with open(path, encoding="utf-8") as fh:
        raw = json.load(fh)

    all_cards: list[dict[str, Any]] = raw.get("deck", [])
    hand_ids: list[str] = raw.get("initial_hand", [])

    card_meta: dict[str, dict[str, Any]] = {}
    for card in all_cards:
        cid = str(card.get("card_id", ""))
        if cid:
            card_meta[cid] = card

    hand_ids_set = set(hand_ids)
    deck_state: dict[str, list[dict[str, Any]]] = {
        "MainDeck": [],
        "CardHand": [],
        "PlayArea": [],
        "Memory": [],
        "DiscardPile": [],
        "BanishZone": [],
    }
    for card in all_cards:
        cid = str(card.get("card_id", ""))
        if not cid:
            continue
        entry: dict[str, Any] = {
            "card_id":       cid,
            "name":          str(card.get("name", cid)),
            "value":         card.get("value", 0),
            "rule_group_id": str(card.get("rule_group_id", "")),
            "description":   str(card.get("description", "")),
            "rules":         list(card.get("rules", [])),
        }
        if cid in hand_ids_set:
            deck_state["CardHand"].append(entry)
        else:
            deck_state["MainDeck"].append(entry)

    return card_meta, deck_state


# ────────────────────────────────────────────────────────────────────────────────
# Rule-group simulation (mirrors CardRuleBridge + RuleGroupRegistry)
# ────────────────────────────────────────────────────────────────────────────────


class _RuleGroupState:
    """Minimal in-process replica of gmRules::RuleGroupRegistry.

    Tracks which rule groups are active.  Lifecycle is read from the
    rule_groups.json file (if present next to cards.json), otherwise
    defaults to TRANSIENT for all groups.
    """

    def __init__(self, rule_groups_path: str | None = None) -> None:
        # group_id → lifecycle string (TRANSIENT / PERSISTENT / TRIGGER_BOUND)
        self._lifecycle: dict[str, str] = {}
        self._active: set[str] = set()

        if rule_groups_path and os.path.isfile(rule_groups_path):
            try:
                with open(rule_groups_path, encoding="utf-8") as fh:
                    raw = json.load(fh)
                for rg in raw.get("rule_groups", []):
                    gid = str(rg.get("group_id", ""))
                    if gid:
                        self._lifecycle[gid] = str(rg.get("lifecycle", "TRANSIENT"))
                print(
                    f"[rule_groups] {len(self._lifecycle)} gruppi caricati da "
                    f"{os.path.basename(rule_groups_path)}"
                )
            except Exception as exc:  # noqa: BLE001
                print(f"[rule_groups] AVVISO: impossibile caricare {rule_groups_path}: {exc}")

    def lifecycle(self, group_id: str) -> str:
        return self._lifecycle.get(group_id, "TRANSIENT")

    def is_active(self, group_id: str) -> bool:
        return group_id in self._active

    def activate(self, group_id: str) -> bool:
        """Returns True if state changed."""
        if group_id in self._active:
            return False
        self._active.add(group_id)
        return True

    def deactivate(self, group_id: str) -> bool:
        """Returns True if state changed."""
        if group_id not in self._active:
            return False
        self._active.discard(group_id)
        return True


def _fire_rule_group_event(
    event_sock: socket.socket,
    card_id: str,
    rule_group_id: str,
    from_zone: str,
    to_zone: str,
    rg_state: _RuleGroupState,
) -> None:
    """Simulate CardRuleBridge zone-change callback.

    Fires ``gmRules.rule_group.activated`` or
    ``gmRules.rule_group.deactivated`` when appropriate and updates
    ``rg_state``.
    """
    if not rule_group_id:
        return

    entering_active = to_zone in _ACTIVE_ZONES
    leaving_active = (from_zone in _ACTIVE_ZONES) and (to_zone not in _ACTIVE_ZONES)

    if entering_active:
        changed = rg_state.activate(rule_group_id)
        if changed:
            _send_event(
                event_sock,
                "gmRules.rule_group.activated",
                {"group_id": rule_group_id, "card_id": card_id, "zone": to_zone},
            )
            print(
                f"[rule_group] ✔ ATTIVATO  {rule_group_id!r:<24}  "
                f"({card_id} → {to_zone})"
            )
        else:
            print(
                f"[rule_group]   gia attivo {rule_group_id!r:<24}  "
                f"({card_id} → {to_zone})"
            )

    elif leaving_active and rg_state.lifecycle(rule_group_id) == "TRANSIENT":
        changed = rg_state.deactivate(rule_group_id)
        if changed:
            _send_event(
                event_sock,
                "gmRules.rule_group.deactivated",
                {"group_id": rule_group_id, "card_id": card_id, "zone": to_zone},
            )
            print(
                f"[rule_group] ✘ DISATTIV. {rule_group_id!r:<24}  "
                f"({card_id}: {from_zone} → {to_zone})"
            )


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


def _emit_initial_snapshot(
    event_sock: socket.socket,
    deck_state: dict[str, list[dict[str, Any]]],
) -> None:
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
                    "equipment": {},
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

    for zone_name, cards in deck_state.items():
        _send_event(
            event_sock,
            "gmAlea.deck.zone_changed",
            {"zone_name": zone_name, "cards": cards},
        )

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


def _initial_deck_state() -> dict[str, list[dict[str, str]]]:
    return {
        "MainDeck": [
            {"card_id": "fire_01", "name": "Fireball"},
            {"card_id": "shield_01", "name": "Shield"},
            {"card_id": "heal_01", "name": "Heal"},
        ],
        "CardHand": [
            {"card_id": "potion_01", "name": "Potion"},
            {"card_id": "arrow_01", "name": "Arrow"},
        ],
        "PlayArea": [],
        "Memory": [],
        "DiscardPile": [],
        "BanishZone": [],
    }


def _find_and_remove_card(
    deck_state: dict[str, list[dict[str, str]]],
    from_zone: str,
    card_id: str,
) -> dict[str, str] | None:
    cards = deck_state.get(from_zone)
    if cards is None:
        return None
    for idx, card in enumerate(cards):
        if str(card.get("card_id", "")) == card_id:
            return cards.pop(idx)
    return None


def _handle_deck_move_command(
    event_sock: socket.socket,
    deck_state: dict,
    cmd: dict,
    card_meta: dict[str, dict[str, Any]],
    rg_state: _RuleGroupState,
) -> None:
    data = cmd.get("data", {})
    if not isinstance(data, dict):
        return

    card_id = str(data.get("card_id", ""))
    from_zone = str(data.get("from", ""))
    to_zone = str(data.get("to", ""))
    if not card_id or not from_zone or not to_zone or from_zone == to_zone:
        return
    if from_zone not in deck_state or to_zone not in deck_state:
        return

    card = _find_and_remove_card(deck_state, from_zone, card_id)
    if card is None:
        return
    deck_state[to_zone].insert(0, card)

    _send_event(
        event_sock,
        "gmAlea.deck.card_moved",
        {
            "card_id": card_id,
            "from_zone": from_zone,
            "to_zone": to_zone,
        },
    )

    rule_group_id = str(card_meta.get(card_id, {}).get("rule_group_id", ""))
    _fire_rule_group_event(event_sock, card_id, rule_group_id, from_zone, to_zone, rg_state)


def _handle_draw_command(
    event_sock: socket.socket,
    deck_state: dict,
    cmd: dict,
    card_meta: dict[str, dict[str, Any]],
    rg_state: _RuleGroupState,
) -> None:
    data = cmd.get("data", {})
    count = 1
    if isinstance(data, dict):
        try:
            count = int(data.get("count", 1))
        except (TypeError, ValueError):
            count = 1
    if count <= 0:
        return

    for _ in range(count):
        if len(deck_state["MainDeck"]) == 0:
            return
        card = deck_state["MainDeck"].pop(0)
        deck_state["CardHand"].insert(0, card)
        cid = str(card.get("card_id", ""))
        _send_event(
            event_sock,
            "gmAlea.deck.card_moved",
            {
                "card_id": cid,
                "from_zone": "MainDeck",
                "to_zone": "CardHand",
            },
        )
        rule_group_id = str(card_meta.get(cid, {}).get("rule_group_id", ""))
        _fire_rule_group_event(event_sock, cid, rule_group_id, "MainDeck", "CardHand", rg_state)


def _handle_recycle_discard(event_sock: socket.socket, deck_state: dict) -> None:
    discard_cards = deck_state.get("DiscardPile", [])
    if len(discard_cards) == 0:
        return

    random.shuffle(discard_cards)
    # Place shuffled discard cards on top of MainDeck as a pile.
    main_cards = deck_state.get("MainDeck", [])
    deck_state["MainDeck"] = discard_cards + main_cards
    deck_state["DiscardPile"] = []

    _send_event(
        event_sock,
        "gmAlea.deck.zone_changed",
        {
            "zone_name": "MainDeck",
            "cards": deck_state["MainDeck"],
        },
    )
    _send_event(
        event_sock,
        "gmAlea.deck.zone_changed",
        {
            "zone_name": "DiscardPile",
            "cards": [],
        },
    )
    _send_event(
        event_sock,
        "gmAlea.deck.shuffled",
        {
            "zone_name": "MainDeck",
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


def _serve_commands(
    host: str,
    cmd_port: int,
    event_sock: socket.socket,
    deck_state: dict,
    card_meta: dict[str, dict[str, Any]],
    rg_state: _RuleGroupState,
) -> None:
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
                    try:
                        raw = recv_frame(client)
                    except socket.timeout:
                        continue
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
                    elif type_id == "gmAlea.deck.move_card":
                        _handle_deck_move_command(event_sock, deck_state, cmd, card_meta, rg_state)
                    elif type_id == "gmAlea.deck.draw":
                        _handle_draw_command(event_sock, deck_state, cmd, card_meta, rg_state)
                    elif type_id == "gmAlea.deck.recycle_discard":
                        _handle_recycle_discard(event_sock, deck_state)
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
    parser.add_argument(
        "--cards",
        default=None,
        metavar="PATH",
        help=(
            "Percorso a un file JSON di carte (es. data/cards_dominion.json). "
            "Se non specificato usa il deck hard-coded. "
            "Il file rule_groups.json deve trovarsi nella stessa cartella."
        ),
    )
    args = parser.parse_args()

    # ── Load deck ──────────────────────────────────────────────────────────────
    card_meta: dict[str, dict[str, Any]] = {}
    rg_path: str | None = None

    if args.cards:
        cards_path = os.path.abspath(args.cards)
        if not os.path.isfile(cards_path):
            print(f"[mock_engine] ERRORE: file carte non trovato: {cards_path}")
            return 1
        card_meta, deck_state = _load_cards_json(cards_path)
        rg_path = str(Path(cards_path).parent / "rule_groups.json")
        print(
            f"[mock_engine] Carte caricate da {os.path.basename(cards_path)}: "
            f"{sum(len(v) for v in deck_state.values())} token"
        )
    else:
        deck_state = _initial_deck_state()

    rg_state = _RuleGroupState(rg_path)

    # ── Connect and run ────────────────────────────────────────────────────────
    print(f"[mock_engine] Connessione stream eventi a {args.host}:{args.port} ...")
    with _connect_event_stream(args.host, args.port, timeout_s=20.0) as event_sock:
        print("[mock_engine] Connesso. Invio snapshot iniziale...")
        _emit_initial_snapshot(event_sock, deck_state)
        print("[mock_engine] Modalita manuale: usa 'Passa Turno' in Flow/Timeline.")
        print("[mock_engine] Usa la GUI per spostare carte tra zone.")
        print("[mock_engine] I movimenti verso PlayArea/Memory attivano i rule group.")
        _serve_commands(args.host, args.cmd_port, event_sock, deck_state, card_meta, rg_state)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
