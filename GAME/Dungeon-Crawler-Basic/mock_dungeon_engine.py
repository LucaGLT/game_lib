"""Mock Dungeon Engine per Dungeon Crawler Basic.

Simula il CoreEngine C++ per il testing visuale della GUI:

- Stream eventi: client TCP verso EngineReceiver della GUI (porta 9200).
- Ricezione comandi: server TCP per EngineSender della GUI (porta 9201).

Attori simulati:
  hero       — Eroe del giocatore (PV 20/20, stanza room_1)
  goblin_1   — Mostro standard (PV 8/8, stanza room_1)
  troll_boss — Boss (PV 15/15, stanza room_3)

Carte:
  Caricate da data/cards_dungeon.json con regole in data/dungeon_rules.json
  e gruppi-regola in data/rule_groups_dungeon.json.

Effetti supportati:
  DEAL_DAMAGE  → target auto (primo nemico vivo stessa stanza)
  HEAL         → target eroe
  APPLY_STATUS → target dipende da effect.target
  MOVE_ACTOR   → sposta eroe nella prima stanza adiacente libera
  ADD_TAG      → aggiunge tag all'eroe

Mapping eventi:
  Dungeon (per HeroPanelWidget): dungeon.actor.hp_changed / status_changed / moved / equipped
  gmActor (per GmCompDeckModule): gmActor.snapshot / gmActor.actor.resource_changed
  gmAlea  (per GmCompDeckModule): gmAlea.deck.zone_changed / gmAlea.deck.card_moved
"""
from __future__ import annotations

import copy
import json
import os
import socket
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

# ── path bootstrap ─────────────────────────────────────────────────────────────
_PYLIB_DIR = Path(__file__).resolve().parents[2] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))

from gmGui.engine_bridge.framing import recv_frame, send_frame  # noqa: E402

# ── porta defaults ─────────────────────────────────────────────────────────────
HOST         = "127.0.0.1"
EVENT_PORT   = 9200
COMMAND_PORT = 9201

# Zone che contano come "carta in gioco attivo" — specchio di CardRuleBridge.
_ACTIVE_ZONES: frozenset[str] = frozenset({"PlayArea", "Memory"})

# Percorsi dati relativi a questo script.
_DATA_DIR  = Path(__file__).resolve().parent / "data"
_CARDS_PATH  = str(_DATA_DIR / "cards_dungeon.json")
_RGROUPS_PATH = str(_DATA_DIR / "rule_groups_dungeon.json")
_RULES_PATH  = str(_DATA_DIR / "dungeon_rules.json")

# ── stato attori ──────────────────────────────────────────────────────────────
_INITIAL_ACTORS: dict[str, dict[str, Any]] = {
    "hero": {
        "kind":     "HERO",
        "hp":       20,
        "max_hp":   20,
        "location": "room_1",
        "tags":     [],
        "statuses": {},
    },
    "goblin_1": {
        "kind":     "MONSTER",
        "hp":       8,
        "max_hp":   8,
        "location": "room_1",
        "tags":     [],
        "statuses": {},
    },
    "troll_boss": {
        "kind":     "BOSS_MONSTER",
        "hp":       15,
        "max_hp":   15,
        "location": "room_3",
        "tags":     [],
        "statuses": {},
    },
}

# Adiacenza stanze (room_1 ↔ room_2 ↔ room_3).
_ADJACENCY: dict[str, list[str]] = {
    "room_1": ["room_2"],
    "room_2": ["room_1", "room_3"],
    "room_3": ["room_2"],
}

# ── azioni eroe per turno ─────────────────────────────────────────────────────
_HERO_ACTIONS_PER_TURN: int = 2

# ── Actor id usato negli eventi gmActor (per GmCompDeckModule) ────────────────
# GmCompDeckModule cerca "Player_X" per il tracking risorse.
_GACTOR_HERO_ID: str = "Player_X"


# ══════════════════════════════════════════════════════════════════════════════
# Caricamento JSON
# ══════════════════════════════════════════════════════════════════════════════

def _load_cards_json(
    path: str,
) -> tuple[dict[str, dict[str, Any]], dict[str, list[dict[str, Any]]]]:
    """Carica le definizioni carte da JSON.

    Returns:
        card_meta  — dict[card_id, entry completo]
        deck_state — distribuzione iniziale per zona
    """
    with open(path, encoding="utf-8") as fh:
        raw = json.load(fh)

    all_cards: list[dict[str, Any]] = raw.get("cards", [])
    hand_ids: list[str] = raw.get("initial_hand", [])
    hand_ids_set: set[str] = set(hand_ids)

    card_meta: dict[str, dict[str, Any]] = {}
    for card in all_cards:
        cid = str(card.get("card_id", ""))
        if cid:
            card_meta[cid] = card

    deck_state: dict[str, list[dict[str, Any]]] = {
        "MainDeck":   [],
        "CardHand":   [],
        "PlayArea":   [],
        "Memory":     [],
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
            "action_cost":   int(card.get("action_cost", 1)),
            "rule_group_id": str(card.get("rule_group_id", "")),
            "description":   str(card.get("description", "")),
            "rules":         list(card.get("rules", [])),
        }
        if cid in hand_ids_set:
            deck_state["CardHand"].append(entry)
        else:
            deck_state["MainDeck"].append(entry)

    return card_meta, deck_state


def _load_rules_json(path: str) -> dict[str, dict[str, Any]]:
    """Carica le definizioni regole da JSON.

    Returns:
        rule_book — dict[rule_id, rule_definition_dict]
    """
    if not os.path.isfile(path):
        print(f"[rules] AVVISO: file regole non trovato: {path}")
        return {}
    try:
        with open(path, encoding="utf-8") as fh:
            raw = json.load(fh)
        rule_book: dict[str, dict[str, Any]] = {}
        for rule in raw.get("rules", []):
            rid = str(rule.get("rule_id", ""))
            if rid:
                rule_book[rid] = rule
        print(f"[rules] {len(rule_book)} regole caricate da {os.path.basename(path)}")
        return rule_book
    except Exception as exc:  # noqa: BLE001
        print(f"[rules] AVVISO: impossibile caricare {path}: {exc}")
        return {}


# ══════════════════════════════════════════════════════════════════════════════
# Stato gruppi-regola (legge "rule_group_id" dal JSON dungeon)
# ══════════════════════════════════════════════════════════════════════════════

class _DungeonRuleGroupState:
    """Replica in-process di RuleGroupRegistry per il Dungeon Crawler.

    Legge il file rule_groups_dungeon.json il cui schema usa "rule_group_id"
    come chiave (diverso da "group_id" usato nel sandbox Dominion).
    """

    def __init__(self, rg_path: str | None = None) -> None:
        self._lifecycle: dict[str, str] = {}
        self._rule_ids:  dict[str, list[str]] = {}
        self._active:    set[str] = set()

        if rg_path and os.path.isfile(rg_path):
            try:
                with open(rg_path, encoding="utf-8") as fh:
                    raw = json.load(fh)
                for rg in raw.get("rule_groups", []):
                    gid = str(rg.get("rule_group_id", ""))
                    if gid:
                        self._lifecycle[gid] = str(rg.get("lifecycle", "TRANSIENT"))
                        self._rule_ids[gid]  = [str(r) for r in rg.get("rule_ids", [])]
                print(
                    f"[rule_groups] {len(self._lifecycle)} gruppi caricati da "
                    f"{os.path.basename(rg_path)}"
                )
            except Exception as exc:  # noqa: BLE001
                print(f"[rule_groups] AVVISO: {rg_path}: {exc}")

    def lifecycle(self, gid: str) -> str:
        return self._lifecycle.get(gid, "TRANSIENT")

    def is_active(self, gid: str) -> bool:
        return gid in self._active

    def activate(self, gid: str) -> bool:
        if gid in self._active:
            return False
        self._active.add(gid)
        return True

    def deactivate(self, gid: str) -> bool:
        if gid not in self._active:
            return False
        self._active.discard(gid)
        return True

    def rule_ids_of(self, gid: str) -> list[str]:
        return self._rule_ids.get(gid, [])


# ══════════════════════════════════════════════════════════════════════════════
# Utility eventi
# ══════════════════════════════════════════════════════════════════════════════

def _env(type_id: str, data: dict[str, Any]) -> dict[str, Any]:
    return {
        "typeId": type_id,
        "source": "MockDungeonEngine",
        "time":   datetime.now(timezone.utc).isoformat(),
        "data":   data,
    }


def _send_event(sock: socket.socket, type_id: str, data: dict[str, Any]) -> None:
    payload = json.dumps(_env(type_id, data), ensure_ascii=True)
    send_frame(sock, payload)


# ══════════════════════════════════════════════════════════════════════════════
# Risoluzione target per gli effetti dungeon
# ══════════════════════════════════════════════════════════════════════════════

def _resolve_targets(
    target_spec: str,
    actors: dict[str, dict[str, Any]],
    active_actor_id: str,
) -> list[str]:
    """Risolve la specifica target in una lista di actor_id."""
    actor = actors.get(active_actor_id, {})
    hero_loc = str(actor.get("location", ""))

    if target_spec == "SELF":
        return [active_actor_id]

    if target_spec == "SELECTED_ENEMY":
        # Auto-selezione: primo nemico vivo nella stessa locazione.
        enemies = [
            aid for aid, a in actors.items()
            if a.get("kind", "") != "HERO"
            and int(a.get("hp", 0)) > 0
            and str(a.get("location", "")) == hero_loc
        ]
        return enemies[:1]

    if target_spec == "ALL_ENEMIES_IN_LOCATION":
        return [
            aid for aid, a in actors.items()
            if a.get("kind", "") != "HERO"
            and int(a.get("hp", 0)) > 0
            and str(a.get("location", "")) == hero_loc
        ]

    if target_spec == "ALL_ALLIES_IN_LOCATION":
        return [
            aid for aid, a in actors.items()
            if a.get("kind", "") == "HERO"
            and int(a.get("hp", 0)) > 0
            and str(a.get("location", "")) == hero_loc
        ]

    return [active_actor_id]


# ══════════════════════════════════════════════════════════════════════════════
# Applicazione effetti dungeon
# ══════════════════════════════════════════════════════════════════════════════

def _apply_dungeon_effects(
    rule_ids: list[str],
    rule_book: dict[str, dict[str, Any]],
    active_actor_id: str,
    event_sock: socket.socket,
    actors: dict[str, dict[str, Any]],
    resources: dict[str, int],
) -> None:
    """Applica gli effetti delle regole e aggiorna lo stato attori + risorse."""
    for rule_id in rule_ids:
        rule_def = rule_book.get(rule_id)
        if rule_def is None:
            print(f"  [effect] {rule_id}: definizione non trovata in rule_book")
            continue
        effects: list[dict[str, Any]] = rule_def.get("effects", [])
        if not effects:
            print(f"  [effect] {rule_id}: nessun effetto definito")
            continue

        for eff in effects:
            eff_type  = str(eff.get("type",      "")).upper()
            target_sp = str(eff.get("target",    "SELF"))
            amount    = int(eff.get("amount",    0))
            status_id = str(eff.get("status_id", ""))
            tag       = str(eff.get("tag",       ""))

            targets = _resolve_targets(target_sp, actors, active_actor_id)

            # ── DEAL_DAMAGE ────────────────────────────────────────────────
            if eff_type == "DEAL_DAMAGE":
                for tid in targets:
                    a = actors.get(tid)
                    if not a:
                        continue
                    old_hp: int = int(a["hp"])
                    new_hp: int = max(0, old_hp - amount)
                    a["hp"] = new_hp
                    print(f"  [effect] DANNO  {tid}: PV {old_hp} → {new_hp} ({-amount})")
                    _send_event(event_sock, "dungeon.actor.hp_changed", {
                        "actor_id": tid,
                        "hp_after": new_hp,
                        "max_hp":   int(a["max_hp"]),
                        "delta":    new_hp - old_hp,
                    })
                    if new_hp == 0:
                        print(f"  [effect] {tid} è stato sconfitto!")
                        _send_event(event_sock, "dungeon.actor.defeated", {
                            "actor_id": tid,
                        })

            # ── HEAL ───────────────────────────────────────────────────────
            elif eff_type == "HEAL":
                for tid in targets:
                    a = actors.get(tid)
                    if not a:
                        continue
                    old_hp = int(a["hp"])
                    new_hp = min(int(a["max_hp"]), old_hp + amount)
                    a["hp"] = new_hp
                    print(f"  [effect] CURA   {tid}: PV {old_hp} → {new_hp} (+{amount})")
                    _send_event(event_sock, "dungeon.actor.hp_changed", {
                        "actor_id": tid,
                        "hp_after": new_hp,
                        "max_hp":   int(a["max_hp"]),
                        "delta":    new_hp - old_hp,
                    })

            # ── APPLY_STATUS ───────────────────────────────────────────────
            elif eff_type == "APPLY_STATUS" and status_id:
                for tid in targets:
                    a = actors.get(tid)
                    if not a:
                        continue
                    a.setdefault("statuses", {})[status_id] = 1
                    print(f"  [effect] STATUS {tid}: +{status_id}")
                    _send_event(event_sock, "dungeon.actor.status_changed", {
                        "actor_id":  tid,
                        "status_id": status_id,
                        "added":     True,
                    })

            # ── ADD_TAG ────────────────────────────────────────────────────
            elif eff_type == "ADD_TAG" and tag:
                for tid in targets:
                    a = actors.get(tid)
                    if not a:
                        continue
                    if tag not in a.get("tags", []):
                        a.setdefault("tags", []).append(tag)
                    print(f"  [effect] TAG    {tid}: +{tag}")
                    _send_event(event_sock, "dungeon.actor.equipped", {
                        "actor_id": tid,
                        "item_tag": tag,
                    })

            # ── MOVE_ACTOR ─────────────────────────────────────────────────
            elif eff_type == "MOVE_ACTOR":
                for tid in targets:
                    a = actors.get(tid)
                    if not a:
                        continue
                    current_loc = str(a.get("location", ""))
                    adj = _ADJACENCY.get(current_loc, [])
                    if adj:
                        new_loc = adj[0]
                        a["location"] = new_loc
                        print(f"  [effect] MUOVI  {tid}: {current_loc} → {new_loc}")
                        _send_event(event_sock, "dungeon.actor.moved", {
                            "actor_id": tid,
                            "from":     current_loc,
                            "to":       new_loc,
                        })
                    else:
                        print(f"  [effect] MUOVI  {tid}: nessuna stanza adiacente a {current_loc}")

            else:
                print(f"  [effect] {eff_type}: non gestito (target={target_sp})")


# ══════════════════════════════════════════════════════════════════════════════
# Snapshot iniziale
# ══════════════════════════════════════════════════════════════════════════════

def _emit_initial_snapshot(
    event_sock: socket.socket,
    actors: dict[str, dict[str, Any]],
    deck_state: dict[str, list[dict[str, Any]]],
    resources: dict[str, int],
) -> None:
    """Emette tutti gli snapshot iniziali verso la GUI."""
    # ── Session / Flow ────────────────────────────────────────────────────────
    _send_event(event_sock, "dungeon.session.started", {
        "session_id": "dungeon_mock_01",
        "round": 1,
    })

    # ── Mappa ─────────────────────────────────────────────────────────────────
    _send_event(event_sock, "dungeon.map.snapshot", {
        "map_id": "dungeon_mock",
        "rooms": [
            {"id": "room_1", "tags": ["start"], "adjacent": ["room_2"]},
            {"id": "room_2", "tags": [],         "adjacent": ["room_1", "room_3"]},
            {"id": "room_3", "tags": ["boss"],   "adjacent": ["room_2"]},
        ],
    })

    # ── Attori (per HeroPanelWidget) ──────────────────────────────────────────
    actors_payload: list[dict[str, Any]] = []
    for aid, a in actors.items():
        actors_payload.append({
            "id":       aid,
            "kind":     a["kind"],
            "hp":       int(a["hp"]),
            "max_hp":   int(a["max_hp"]),
            "location": a["location"],
            "tags":     list(a.get("tags", [])),
            "statuses": list(a.get("statuses", {}).keys()),
        })
    _send_event(event_sock, "dungeon.actor.snapshot", {"actors": actors_payload})

    # ── Turno iniziale ────────────────────────────────────────────────────────
    _send_event(event_sock, "dungeon.turn.started", {
        "actor_id":          "hero",
        "round":             1,
        "actions_remaining": resources.get("actions", _HERO_ACTIONS_PER_TURN),
        "available_actions": ["move", "heal", "equip"],
    })

    # ── gmActor snapshot (per GmCompDeckModule: action tracking) ─────────────
    _send_event(event_sock, "gmActor.snapshot", {
        "actors": [
            {
                "actor_id":    _GACTOR_HERO_ID,
                "name":        "Hero",
                "faction_id":  "heroes",
                "current_hp":  int(actors["hero"]["hp"]),
                "max_hp":      int(actors["hero"]["max_hp"]),
                "life_state":  "ALIVE",
                "statuses":    {},
                "equipment":   {},
                "area_id":     actors["hero"]["location"],
                "resources":   {"actions": resources.get("actions", _HERO_ACTIONS_PER_TURN)},
            },
        ],
    })

    # ── Stato mazzo (per GmCompDeckModule) ───────────────────────────────────
    for zone_name, cards in deck_state.items():
        _send_event(event_sock, "gmAlea.deck.zone_changed", {
            "zone_name": zone_name,
            "cards":     cards,
        })


# ══════════════════════════════════════════════════════════════════════════════
# Gestione spostamento carte
# ══════════════════════════════════════════════════════════════════════════════

def _find_and_remove_card(
    deck_state: dict[str, list[dict[str, Any]]],
    from_zone:  str,
    card_id:    str,
) -> dict[str, Any] | None:
    cards = deck_state.get(from_zone)
    if cards is None:
        return None
    for idx, card in enumerate(cards):
        if str(card.get("card_id", "")) == card_id:
            return cards.pop(idx)
    return None


def _handle_deck_move_command(
    event_sock: socket.socket,
    deck_state: dict[str, list[dict[str, Any]]],
    cmd:        dict[str, Any],
    card_meta:  dict[str, dict[str, Any]],
    rg_state:   _DungeonRuleGroupState,
    rule_book:  dict[str, dict[str, Any]],
    actors:     dict[str, dict[str, Any]],
    resources:  dict[str, int],
) -> None:
    """Gestisce gmAlea.deck.move_card: sposta la carta e applica gli effetti."""
    data = cmd.get("data", {})
    if not isinstance(data, dict):
        return

    card_id   = str(data.get("card_id",   ""))
    from_zone = str(data.get("from",      ""))
    to_zone   = str(data.get("to",        ""))

    if not card_id or not from_zone or not to_zone or from_zone == to_zone:
        return
    if from_zone not in deck_state or to_zone not in deck_state:
        return

    card = _find_and_remove_card(deck_state, from_zone, card_id)
    if card is None:
        print(f"[deck] carta {card_id!r} non trovata in {from_zone}")
        return

    deck_state[to_zone].insert(0, card)

    # Notifica il movimento alla GUI (aggiorna zone nel deck module).
    _send_event(event_sock, "gmAlea.deck.card_moved", {
        "card_id":   card_id,
        "from_zone": from_zone,
        "to_zone":   to_zone,
    })

    # ── Costo azioni ──────────────────────────────────────────────────────────
    if from_zone == "CardHand" and to_zone in _ACTIVE_ZONES:
        action_cost: int = int(card_meta.get(card_id, {}).get("action_cost", 1))
        if action_cost > 0:
            current_actions = resources.get("actions", 0)
            new_actions     = max(0, current_actions - action_cost)
            resources["actions"] = new_actions
            delta = new_actions - current_actions
            print(
                f"  [cost] azioni {current_actions} → {new_actions} "
                f"(giocata {card_id}, costo {action_cost})"
            )
            _send_event(event_sock, "gmActor.actor.resource_changed", {
                "actor_id":    _GACTOR_HERO_ID,
                "resource_id": "actions",
                "delta":       delta,
                "new_value":   new_actions,
            })

    # ── Effetti regola ────────────────────────────────────────────────────────
    rule_group_id = str(card_meta.get(card_id, {}).get("rule_group_id", ""))
    _fire_rule_group(
        event_sock, card_id, rule_group_id,
        from_zone, to_zone,
        rg_state, rule_book, actors, resources,
    )


def _fire_rule_group(
    event_sock:    socket.socket,
    card_id:       str,
    rule_group_id: str,
    from_zone:     str,
    to_zone:       str,
    rg_state:      _DungeonRuleGroupState,
    rule_book:     dict[str, dict[str, Any]],
    actors:        dict[str, dict[str, Any]],
    resources:     dict[str, int],
) -> None:
    """Simula CardRuleBridge: attiva/disattiva rule group e applica effetti."""
    if not rule_group_id:
        return

    entering_active = to_zone in _ACTIVE_ZONES
    leaving_active  = from_zone in _ACTIVE_ZONES and to_zone not in _ACTIVE_ZONES

    if entering_active:
        changed = rg_state.activate(rule_group_id)
        if changed:
            print(
                f"[rule_group] ✔ ATTIVATO  {rule_group_id!r:<28}  "
                f"({card_id} → {to_zone})"
            )
            _send_event(event_sock, "gmRules.rule_group.activated", {
                "group_id": rule_group_id,
                "card_id":  card_id,
                "zone":     to_zone,
            })
        else:
            print(
                f"[rule_group] ✔ GIA ATTIVO {rule_group_id!r:<28}  "
                f"({card_id} → {to_zone}) — riapplico effetti"
            )

        # Gli effetti si applicano sempre, anche se il gruppo era già attivo.
        rule_ids = rg_state.rule_ids_of(rule_group_id)
        if rule_ids:
            _apply_dungeon_effects(
                rule_ids, rule_book,
                "hero", event_sock, actors, resources,
            )

    elif leaving_active and rg_state.lifecycle(rule_group_id) == "TRANSIENT":
        changed = rg_state.deactivate(rule_group_id)
        if changed:
            print(
                f"[rule_group] ✘ DISATTIV. {rule_group_id!r:<28}  "
                f"({card_id}: {from_zone} → {to_zone})"
            )
            _send_event(event_sock, "gmRules.rule_group.deactivated", {
                "group_id": rule_group_id,
                "card_id":  card_id,
                "zone":     to_zone,
            })


# ══════════════════════════════════════════════════════════════════════════════
# Fine turno / turno mostro
# ══════════════════════════════════════════════════════════════════════════════

def _end_hero_turn(
    event_sock: socket.socket,
    actors:     dict[str, dict[str, Any]],
    resources:  dict[str, int],
    round_no:   int,
) -> int:
    """Esegue il turno mostro automatico e inizia il nuovo turno eroe."""
    _send_event(event_sock, "dungeon.turn.ended", {"actor_id": "hero"})
    print(f"[turn] Fine turno eroe (round {round_no}). Turno mostri...")

    # ── Turno mostri: ogni mostro vivo nella stessa stanza attacca l'eroe ────
    hero = actors.get("hero", {})
    hero_loc = str(hero.get("location", ""))
    for aid, a in actors.items():
        if a.get("kind", "") == "HERO":
            continue
        if int(a.get("hp", 0)) <= 0:
            continue
        if str(a.get("location", "")) != hero_loc:
            continue
        # Danno base mostro: 2 PV.
        dmg = 2
        old_hp = int(hero.get("hp", 0))
        new_hp = max(0, old_hp - dmg)
        actors["hero"]["hp"] = new_hp
        print(f"  [mostro] {aid} attacca hero: PV {old_hp} → {new_hp}")
        _send_event(event_sock, "dungeon.actor.hp_changed", {
            "actor_id": "hero",
            "hp_after": new_hp,
            "max_hp":   int(actors["hero"]["max_hp"]),
            "delta":    new_hp - old_hp,
        })
        if new_hp == 0:
            print("  [game_over] Eroe sconfitto!")
            _send_event(event_sock, "dungeon.game.over", {
                "outcome": "HERO_DEFEATED",
            })
            return round_no

    # ── Nuovo turno eroe ──────────────────────────────────────────────────────
    round_no += 1
    resources["actions"] = _HERO_ACTIONS_PER_TURN
    print(f"[turn] Nuovo turno eroe (round {round_no}). Azioni: {_HERO_ACTIONS_PER_TURN}.")

    # Ripristina azioni nel deck module.
    _send_event(event_sock, "gmActor.actor.resource_changed", {
        "actor_id":    _GACTOR_HERO_ID,
        "resource_id": "actions",
        "delta":       _HERO_ACTIONS_PER_TURN - 0,
        "new_value":   _HERO_ACTIONS_PER_TURN,
    })

    _send_event(event_sock, "dungeon.turn.started", {
        "actor_id":          "hero",
        "round":             round_no,
        "actions_remaining": _HERO_ACTIONS_PER_TURN,
        "available_actions": ["move", "heal", "equip"],
    })
    return round_no


# ══════════════════════════════════════════════════════════════════════════════
# Server comandi
# ══════════════════════════════════════════════════════════════════════════════

def _serve_commands(
    host:       str,
    cmd_port:   int,
    event_sock: socket.socket,
    deck_state: dict[str, list[dict[str, Any]]],
    card_meta:  dict[str, dict[str, Any]],
    rg_state:   _DungeonRuleGroupState,
    rule_book:  dict[str, dict[str, Any]],
    actors:     dict[str, dict[str, Any]],
    resources:  dict[str, int],
) -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, cmd_port))
    server.listen(1)
    print(f"[mock_dungeon] CmdServer in ascolto su {host}:{cmd_port}")

    round_no = 1

    try:
        while True:
            client, addr = server.accept()
            client.settimeout(1.0)
            print(f"[mock_dungeon] GUI command channel connesso da {addr[0]}:{addr[1]}")
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
                    data    = cmd.get("data", {}) or {}

                    # ── Carte ─────────────────────────────────────────────────
                    if type_id == "gmAlea.deck.move_card":
                        _handle_deck_move_command(
                            event_sock, deck_state, cmd, card_meta,
                            rg_state, rule_book, actors, resources,
                        )

                    # ── Fine turno ────────────────────────────────────────────
                    elif type_id in ("dungeon.end_turn", "gmFlow.turn.pass"):
                        round_no = _end_hero_turn(
                            event_sock, actors, resources, round_no,
                        )

                    # ── Comandi dungeon classici (v1) ─────────────────────────
                    elif type_id == "dungeon.move":
                        hero_id = str(data.get("hero_id", "hero"))
                        dest    = str(data.get("destination", ""))
                        actor   = actors.get(hero_id, {})
                        current = str(actor.get("location", ""))
                        if dest in _ADJACENCY.get(current, []):
                            actor["location"] = dest
                            print(f"[dungeon] {hero_id} si sposta: {current} → {dest}")
                            _send_event(event_sock, "dungeon.actor.moved", {
                                "actor_id": hero_id,
                                "from": current,
                                "to":   dest,
                            })
                        else:
                            _send_event(event_sock, "dungeon.action.rejected", {
                                "reason":  f"Stanza {dest!r} non adiacente a {current!r}.",
                                "command": "dungeon.move",
                            })

                    elif type_id == "dungeon.heal":
                        hero_id   = str(data.get("hero_id",   "hero"))
                        target_id = str(data.get("target_id", hero_id))
                        a = actors.get(target_id, {})
                        old_hp = int(a.get("hp", 0))
                        new_hp = min(int(a.get("max_hp", 20)), old_hp + 3)
                        a["hp"] = new_hp
                        _send_event(event_sock, "dungeon.actor.hp_changed", {
                            "actor_id": target_id,
                            "hp_after": new_hp,
                            "max_hp":   int(a.get("max_hp", 20)),
                            "delta":    new_hp - old_hp,
                        })
                        _send_event(event_sock, "dungeon.actor.healed", {
                            "actor_id": target_id,
                            "amount":   new_hp - old_hp,
                        })

                    elif type_id == "dungeon.end_turn":
                        round_no = _end_hero_turn(
                            event_sock, actors, resources, round_no,
                        )

            except (ConnectionError, OSError):
                print("[mock_dungeon] GUI command channel disconnesso")
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


# ══════════════════════════════════════════════════════════════════════════════
# Connessione stream eventi
# ══════════════════════════════════════════════════════════════════════════════

def _connect_event_stream(host: str, port: int, timeout_s: float = 20.0) -> socket.socket:
    deadline  = time.time() + timeout_s
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
        f"[mock_dungeon] impossibile connettersi a {host}:{port} "
        f"entro {timeout_s:.1f}s ({last_err})"
    )


# ══════════════════════════════════════════════════════════════════════════════
# Entry point
# ══════════════════════════════════════════════════════════════════════════════

def main() -> int:
    # ── Carica carte ──────────────────────────────────────────────────────────
    if not os.path.isfile(_CARDS_PATH):
        print(f"[mock_dungeon] ERRORE: carte non trovate: {_CARDS_PATH}")
        return 1

    card_meta, deck_state = _load_cards_json(_CARDS_PATH)
    total = sum(len(v) for v in deck_state.values())
    print(f"[mock_dungeon] {total} carte caricate da {os.path.basename(_CARDS_PATH)}")

    # ── Carica regole ─────────────────────────────────────────────────────────
    rg_state  = _DungeonRuleGroupState(_RGROUPS_PATH)
    rule_book = _load_rules_json(_RULES_PATH)

    # ── Stato attori + risorse ────────────────────────────────────────────────
    actors: dict[str, dict[str, Any]] = copy.deepcopy(_INITIAL_ACTORS)
    resources: dict[str, int] = {"actions": _HERO_ACTIONS_PER_TURN}

    # ── Connetti e lancia ─────────────────────────────────────────────────────
    print(f"[mock_dungeon] Connessione stream eventi a {HOST}:{EVENT_PORT} ...")
    with _connect_event_stream(HOST, EVENT_PORT, timeout_s=20.0) as event_sock:
        print("[mock_dungeon] Connesso. Invio snapshot iniziale...")
        _emit_initial_snapshot(event_sock, actors, deck_state, resources)
        print("[mock_dungeon] Pronto. Trascina le carte dalla Mano verso Giocate.")
        print("[mock_dungeon] Effetti attivi: DEAL_DAMAGE, HEAL, APPLY_STATUS, ADD_TAG, MOVE_ACTOR")
        _serve_commands(
            HOST, COMMAND_PORT, event_sock,
            deck_state, card_meta, rg_state, rule_book,
            actors, resources,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
