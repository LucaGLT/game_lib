# gmActor — Development Plan

**Version:** 0.1.0  
**Status:** Phase 1 — Planning ✅  
**Language:** C++17 Standard  
**Namespace:** `gmActor`

---

## Goal

Generic, reusable C++17 library providing actor-state, statistics, status,
item, equipment, and actor-registry primitives for a mission-based tabletop
engine.

`gmActor` manages **who exists in the game state** and **what mutable
actor-related data they currently have**. It does not manage turn order,
action windows, card play, deck zones, map topology, combat resolution, or
AI behaviour.

---

## Architecture layer diagram

```
┌──────────────────────────────────────────────────────────────┐
│ Game-specific mission engine                                 │
│ Commands, effects, combat rules, objectives, monster AI       │
└───────────────────────────┬──────────────────────────────────┘
                            │ uses
┌───────────────────────────▼──────────────────────────────────┐
│ gmActor                                                       │
│ Actor states, stats, statuses, modifiers, inventory metadata  │
└───────┬───────────────┬───────────────┬──────────────────────┘
        │               │               │
        ▼               ▼               ▼
   gmFlow           gmMap           gmDeck/gmCompDeck
   actor flow       locations       card zones/decks
```

---

## Design decisions (locked)

| # | Decision | Choice |
|---|----------|--------|
| D1 | `MonsterGroupState` and `common(id)` | `common(group_id)` throws `InvalidActorKindError`. Groups accessed only via `monster_group(id)`. `MonsterGroupState` has its own `timeline_position`, `tie_break_rank`, `enabled`, `removed` fields — NOT an embedded `ActorStateCommon`. |
| D2 | `BossState` key in `ActorStore` | Keyed by `controller_group_id`. |
| D3 | `AllyState` on timeline | Appears in `timeline_actor_ids()` if `can_act == true` on its `ActorStateCommon`. |
| D4 | `ActorEvents.hpp` in V1 | Yes — struct-only payload, zero gmDispatch dependency. |
| D5 | `GmFlowActorAdapter` in V1 | Yes — real implementation; includes gmFlow headers. |
| D6 | Serialization | Uses `gmSave/json.hpp` (nlohmann) + `gmSave/gmSave.hpp`. Provides versioned `ActorSaveEnvelope`. |
| D7 | Header-only split | `StatusContainer`, `InventoryState`, `EquipmentState`, `ActorStore` get `.cpp` files. All others are header-only. |
| D8 | `ModifierOperation` / `EquipmentSlot` | Defined once in `core/Enums.hpp`. Thin redirect headers in subdirectories include `Enums.hpp`. |
| D9 | `common()` on unknown ID | Throws `UnknownActorError`. |
| D10 | HP setters auto-kill | Generic helpers clamp only. `life_state` transitions are not automatic. |
| D11 | Status non-stackable re-add | Replaces the existing instance (`stacks = 1`). |
| D12 | Status stackable re-add | Increments `stacks` on the existing instance. |
| D13 | Modifier evaluation order | SET (last wins) → ADD/SUBTRACT → MULTIPLY. |
| D14 | negative damage/heal | No-op (ignored). Documented. |
| D15 | String IDs everywhere | All IDs are `std::string` aliases. No integer-only IDs in this library. |
| D16 | Integer stat keys | `stat_key` is `std::string`; game defines values like `"hp_max"`, `"base_damage"`. |

---

## File structure

```
gmActor/
├── PLAN.md                             ← this file
├── ai-instructions.md                  ← library coding conventions
├── gmActor_API.md                      ← API manual (Phase 3)
│
├── core/
│   ├── Ids.hpp                         ← all string ID aliases
│   ├── Enums.hpp                       ← all enums (ActorKind, AreaPosition,
│   │                                      ActorLifeState, ItemKind,
│   │                                      EquipmentSlot, ModifierOperation,
│   │                                      ModifierDurationKind)
│   └── Tags.hpp                        ← tag helpers (using Tag = std::string)
│
├── stats/
│   ├── StatBlock.hpp                   ← generic stat map / value object
│   ├── Health.hpp                      ← HP helpers (set/damage/heal)
│   └── TimelineStats.hpp               ← timeline_position, tie_break_rank
│
├── modifiers/
│   ├── ModifierOperation.hpp           ← thin include → core/Enums.hpp
│   ├── ModifierSource.hpp              ← ModifierSource value struct
│   └── Modifier.hpp                    ← ModifierDefinition, ModifierInstance,
│                                          apply_modifiers()
│
├── statuses/
│   ├── StatusDefinition.hpp            ← immutable definition
│   ├── StatusInstance.hpp              ← mutable runtime instance
│   ├── StatusContainer.hpp             ← class declaration
│   └── StatusContainer.cpp             ← class implementation
│
├── items/
│   ├── EquipmentSlot.hpp               ← thin include → core/Enums.hpp
│   ├── ItemDefinition.hpp              ← immutable item definition
│   ├── ItemState.hpp                   ← mutable runtime item state
│   ├── InventoryState.hpp              ← class declaration
│   ├── InventoryState.cpp              ← class implementation
│   ├── EquipmentState.hpp              ← class declaration
│   └── EquipmentState.cpp              ← class implementation
│
├── actors/
│   ├── ActorStateCommon.hpp            ← shared mutable state for all actors
│   ├── HeroState.hpp                   ← hero-specific state
│   ├── AllyState.hpp                   ← ally NPC state
│   ├── MonsterInstanceState.hpp        ← individual targetable monster
│   ├── MonsterGroupState.hpp           ← group acting on timeline
│   ├── BossState.hpp                   ← optional boss extension
│   ├── MissionSystemState.hpp          ← scripted/environment actor
│   ├── ActorStore.hpp                  ← class declaration
│   ├── ActorStore.cpp                  ← class implementation
│   └── ActorQueries.hpp                ← pure free-function queries
│
├── adapters/
│   └── GmFlowActorAdapter.hpp          ← builds gmFlow::Actor from gmActor state
│
├── events/
│   └── ActorEvents.hpp                 ← struct-only event payloads
│
├── serialization/
│   ├── ActorJson.hpp                   ← to_json/from_json declarations
│   ├── ActorJson.cpp                   ← to_json/from_json implementations
│   └── ActorSaveEnvelope.hpp           ← versioned save envelope
│
└── tests/
    ├── test_actor_common.cpp
    ├── test_health.cpp
    ├── test_status_container.cpp
    ├── test_modifier_container.cpp
    ├── test_inventory_equipment.cpp
    ├── test_actor_store.cpp
    └── test_serialization.cpp
```

Total: **39 files** (32 headers/sources + 7 tests).

---

## Integration boundaries

| Library | gmActor use | Forbidden in gmActor |
|---------|-------------|----------------------|
| `gmFlow` | `adapters/GmFlowActorAdapter.hpp` builds `gmFlow::Actor` from state. `gmFlow::ActorId` and `gmActor::ActorId` are both `std::string`, trivially compatible. | Turn order, action windows, flow controller. |
| `gmDeck/gmCompDeck` | Store `DeckInstanceId`, `CardId` only as string references. | Zone management, card draw, shuffle. |
| `gmMap` | Store `AreaId`, `AreaPosition` only. | Map graph, pathfinding, adjacency. |
| `gmDispatch` | `ActorEvents.hpp` defines payloads; publishing is game-engine responsibility. | Event bus creation, subscription. |
| `gmSave` | `serialization/` uses `gmSave/json.hpp` + `gmSave/gmSave.hpp`. | Save file management inside gmActor core. |
| `gmLog` | No dependency in core. Logging is game-engine responsibility. | Hardcoded logging in setters. |

---

## Exceptions

```cpp
class ActorError              : public std::runtime_error {};
class UnknownActorError       : public ActorError {};
class DuplicateActorError     : public ActorError {};
class InvalidActorKindError   : public ActorError {};
class UnknownItemError        : public ActorError {};
class InvalidEquipmentSlotError : public ActorError {};
```

---

## Development phases

### Phase 1 — Planning ✅
- [x] Confirm file tree
- [x] Confirm design decisions (D1–D16)
- [x] Confirm integration boundaries
- [x] Confirm exceptions

### Phase 2 — Headers and stubs
- [ ] Create all headers (Ids, Enums, Tags, stats, modifiers, statuses, items, actors, adapters, events, serialization)
- [ ] Create `.cpp` stubs for StatusContainer, InventoryState, EquipmentState, ActorStore, ActorJson
- [ ] Add Doxygen comments on all public symbols
- [ ] Compile cleanly with no implementation logic (safe placeholders only)

### Phase 3 — API documentation
- [ ] Write `gmActor_API.md` covering overview, architecture, class reference, examples, integration notes

### Phase 4 — Implementation (ordered)
1. [ ] `core/` — Ids, Enums, Tags (trivial)
2. [ ] `stats/` — StatBlock, Health helpers, TimelineStats
3. [ ] `modifiers/` — Modifier structs + apply_modifiers evaluator
4. [ ] `statuses/` — StatusDefinition, StatusInstance, StatusContainer
5. [ ] `items/` — ItemDefinition, ItemState, InventoryState, EquipmentState
6. [ ] `actors/` — ActorStateCommon, all concrete state structs
7. [ ] `actors/ActorStore` — storage, accessors, timeline/area queries
8. [ ] `actors/ActorQueries` — pure free-function query helpers
9. [ ] `adapters/GmFlowActorAdapter` — gmFlow bridge
10. [ ] `events/ActorEvents` — event payloads
11. [ ] `serialization/` — to_json/from_json for all types + ActorSaveEnvelope

### Phase 5 — Tests (one file per subsystem)
- [ ] test_actor_common.cpp
- [ ] test_health.cpp
- [ ] test_status_container.cpp
- [ ] test_modifier_container.cpp
- [ ] test_inventory_equipment.cpp
- [ ] test_actor_store.cpp
- [ ] test_serialization.cpp

---

## Build commands (from game_lib root)

```powershell
# Subsystem tests (no gmSave required):
clang++ -std=c++17 -I. `
    gmActor/statuses/StatusContainer.cpp `
    gmActor/items/InventoryState.cpp `
    gmActor/items/EquipmentState.cpp `
    gmActor/actors/ActorStore.cpp `
    gmActor/tests/test_actor_common.cpp `
    -o test_gmActor_common.exe

# Serialization test (requires gmSave):
clang++ -std=c++17 -I. `
    gmActor/statuses/StatusContainer.cpp `
    gmActor/items/InventoryState.cpp `
    gmActor/items/EquipmentState.cpp `
    gmActor/actors/ActorStore.cpp `
    gmActor/serialization/ActorJson.cpp `
    gmActor/tests/test_serialization.cpp `
    -o test_gmActor_serial.exe
```

---

## Non-goals for V1

Turn order, timeline controller, action validation, card play rules, deck zones,
map adjacency, pathfinding, combat resolution, monster AI, objective resolution,
campaign progression, UI bindings, undo/redo, threaded event delivery.
