# Wire Contract v1 — Dungeon Crawler Basic

**Version:** 1.0.0
**Status:** Frozen ✅ (FASE A)
**Namespace:** `gmDungeonBasic`

> This document is the **single source of truth** for the communication protocol
> between the CoreEngine (C++17) and the GUI (PySide6).
> Any modification requires explicit approval and a version bump to v2.

---

## Transport

| Parameter | Value |
|---|---|
| Protocol | TCP / loopback (127.0.0.1) |
| Frame format | 4-byte big-endian length + UTF-8 JSON payload |
| Events port | **9200** (GUI is server, Engine is client) |
| Commands port | **9201** (Engine is server, GUI is client) |
| Encoding | UTF-8 JSON |

The framing is identical to the `gmDispatch::IpSocketChannel` wire format so
that `pyLib/gmGui/engine_bridge` can be reused without modification.

---

## Event envelope (CoreEngine → GUI)

```json
{
  "typeId":  "<event_id>",
  "source":  "DungeonCore",
  "headers": { "data": "<payload JSON as string>" }
}
```

The GUI normalises `headers.data` (JSON string) into `msg["data"]` (dict)
using the existing `EngineReceiver` normalisation step.

---

## Command envelope (GUI → CoreEngine)

```json
{
  "typeId":  "<command_id>",
  "source":  "GUI",
  "data":    { ... }
}
```

---

## Commands (GUI → CoreEngine)

### dungeon.new_game

Starts a new dungeon session.

```json
{
  "typeId": "dungeon.new_game",
  "data":   { "map_file": "maps/dungeon_01.json" }
}
```

| Field | Type | Required | Description |
|---|---|---|---|
| `map_file` | string | no | Path to JSON map file. Defaults to built-in map. |

---

### dungeon.move

Moves the hero to an adjacent room.

```json
{
  "typeId": "dungeon.move",
  "data":   { "hero_id": "hero", "destination": "room_2" }
}
```

| Field | Type | Required | Description |
|---|---|---|---|
| `hero_id` | string | yes | Actor id of the moving hero. |
| `destination` | string | yes | Target room id (must be adjacent). |

---

### dungeon.heal

Uses a healing potion (requires tag `has_potion`).

```json
{
  "typeId": "dungeon.heal",
  "data":   { "hero_id": "hero", "target_id": "hero" }
}
```

| Field | Type | Required | Description |
|---|---|---|---|
| `hero_id` | string | yes | Actor consuming the potion. |
| `target_id` | string | no | Actor receiving the heal. Defaults to `hero_id` (self-heal). |

---

### dungeon.equip

Equips a weapon (requires tag `bigword_available`, no `equipped_weapon`).

```json
{
  "typeId": "dungeon.equip",
  "data":   { "hero_id": "hero", "item_tag": "bigword_available" }
}
```

| Field | Type | Required | Description |
|---|---|---|---|
| `hero_id` | string | yes | Actor equipping the weapon. |
| `item_tag` | string | yes | Item tag to activate (e.g. `bigword_available`). |

---

### dungeon.end_turn

Signals that the hero has finished their actions for this turn.

```json
{
  "typeId": "dungeon.end_turn",
  "data":   { "hero_id": "hero" }
}
```

---

## Events (CoreEngine → GUI)

### dungeon.session.started

Emitted once at the start of a new session.

```json
{
  "session_id": "s_20260618_001",
  "round": 1
}
```

---

### dungeon.map.snapshot

Full map snapshot. Emitted once after session start and on request.

```json
{
  "map_id": "dungeon_01",
  "rooms": [
    {
      "id": "room_1",
      "tags": ["start"],
      "adjacent": ["room_2", "room_3"]
    }
  ]
}
```

---

### dungeon.actor.snapshot

Full roster snapshot. Emitted once after map snapshot.

```json
{
  "actors": [
    {
      "id":       "hero",
      "kind":     "HERO",
      "hp":       10,
      "max_hp":   10,
      "location": "room_1",
      "tags":     [],
      "statuses": []
    },
    {
      "id":       "monster_1",
      "kind":     "MONSTER",
      "hp":       5,
      "max_hp":   5,
      "location": "room_2",
      "tags":     [],
      "statuses": []
    }
  ]
}
```

---

### dungeon.actor.moved

An actor changed room.

```json
{ "actor_id": "hero", "from": "room_1", "to": "room_2" }
```

---

### dungeon.actor.healed

A heal action was completed.

```json
{ "actor_id": "hero", "target_id": "hero", "amount": 3, "hp_after": 8 }
```

---

### dungeon.actor.equipped

A weapon was equipped.

```json
{ "actor_id": "hero", "item_tag": "bigword_available" }
```

---

### dungeon.actor.status_changed

A status was added or removed.

```json
{ "actor_id": "hero", "status_id": "defended", "added": true }
```

---

### dungeon.actor.hp_changed

HP changed by damage or healing (incremental).

```json
{ "actor_id": "monster_1", "delta": -2, "hp_after": 3 }
```

---

### dungeon.turn.started

A new actor turn begins.

```json
{ "actor_id": "hero", "round": 2 }
```

---

### dungeon.turn.ended

The current actor's turn has ended.

```json
{ "actor_id": "hero" }
```

---

### dungeon.action.rejected

The engine rejected an action.

```json
{ "reason": "No potion available.", "command": "dungeon.heal" }
```

---

### dungeon.game.over

The session ended.

```json
{ "outcome": "DUNGEON_CLEARED" }
```

`outcome` is one of: `HERO_DEFEATED`, `DUNGEON_CLEARED`.

---

## Map JSON format (dungeon definition file)

```json
{
  "map_id": "dungeon_01",
  "rooms": [
    {
      "id":       "room_1",
      "tags":     ["start"],
      "adjacent": ["room_2", "room_3"]
    },
    {
      "id":       "room_2",
      "tags":     [],
      "adjacent": ["room_1", "room_4"]
    }
  ],
  "actors": [
    {
      "id":     "hero",
      "kind":   "HERO",
      "hp":     10,
      "max_hp": 10,
      "room":   "room_1",
      "tags":   ["has_potion", "bigword_available"]
    },
    {
      "id":     "monster_1",
      "kind":   "MONSTER",
      "hp":     5,
      "max_hp": 5,
      "room":   "room_2",
      "tags":   []
    },
    {
      "id":     "boss",
      "kind":   "BOSS_MONSTER",
      "hp":     20,
      "max_hp": 20,
      "room":   "room_5",
      "tags":   []
    }
  ]
}
```

---

## Out of scope in v1

The following actions are **not** part of this contract and must not appear
in any command or event:

- `dungeon.attack` / Attack rule
- `dungeon.defend` / Defend rule

These will be defined in a future **contract v2** after explicit approval.
