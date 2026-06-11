# gmActor API Manual

**Version:** 0.1.0  
**Namespace:** `gmActor`  
**Language:** C++17  
**Status:** Phase 2 — stubs complete, Phase 4 implementations pending

---

## Overview

`gmActor` is a generic C++17 library that manages **actor state** in a
mission-based tabletop engine.

It owns:
- Who exists (heroes, allies, monster instances, monster groups, bosses, mission system)
- Their current stats, HP, statuses, modifiers, inventory, and equipment

It does **not** own:
- Turn order or action windows (`gmFlow`)
- Map topology or pathfinding (`gmMap`)
- Card zones or deck management (`gmDeck` / `gmCompDeck`)
- Combat resolution or AI behaviour (game engine layer above `gmActor`)
- Event publishing (payloads are in `ActorEvents.hpp`; the engine publishes them via `gmDispatch`)

---

## Architecture layer

```
Game-specific mission engine
   Commands, effects, combat, objectives, monster AI
        │ uses
gmActor
   Actor states, stats, statuses, modifiers, inventory
        │          │           │
    gmFlow      gmMap      gmDeck / gmCompDeck
   actor flow  locations   card zones / decks
```

---

## Integration boundaries

| Library | gmActor usage | Forbidden inside gmActor |
|---------|--------------|--------------------------|
| `gmFlow` | `adapters/GmFlowActorAdapter.hpp` builds `gmFlow::Actor` descriptors | Turn order, action windows, flow controller |
| `gmDeck/gmCompDeck` | Stores `DeckInstanceId`, `CardId` as string references only | Zone management, card draw |
| `gmMap` | Stores `AreaId`, `AreaPosition` only | Map graph, pathfinding, adjacency |
| `gmDispatch` | `ActorEvents.hpp` defines payloads; publishing is the engine's responsibility | Event bus creation, subscription |
| `gmSave` | `serialization/` uses `gmSave/json.hpp` + `gmSave/gmSave.hpp` | Save-file management inside core |
| `gmLog` | No core dependency; logging is the engine's responsibility | Hardcoded logging in setters |

---

## Build commands

```powershell
# Compile all gmActor TUs (no test, no gmSave)
$cl = "C:\AppPortable\clang+llvm-19.1.7-x86_64-pc-windows-msvc\bin\clang++.exe"
& $cl -std=c++17 -I. `
  gmActor/modifiers/Modifier.cpp `
  gmActor/statuses/StatusContainer.cpp `
  gmActor/stats/StatBlock.cpp `
  gmActor/stats/Health.cpp `
  gmActor/items/InventoryState.cpp `
  gmActor/items/EquipmentState.cpp `
  gmActor/actors/ActorStore.cpp `
  gmActor/actors/ActorQueries.cpp `
  gmActor/serialization/ActorJson.cpp -c

# Core test (no gmSave)
& $cl -std=c++17 -I. `
  gmActor/modifiers/Modifier.cpp `
  gmActor/statuses/StatusContainer.cpp `
  gmActor/stats/StatBlock.cpp `
  gmActor/stats/Health.cpp `
  gmActor/items/InventoryState.cpp `
  gmActor/items/EquipmentState.cpp `
  gmActor/actors/ActorStore.cpp `
  gmActor/actors/ActorQueries.cpp `
  gmActor/tests/test_health.cpp -o test_gmActor_health.exe

# Serialization test (needs gmSave)
& $cl -std=c++17 -I. `
  <all_actor_sources> `
  gmActor/serialization/ActorJson.cpp `
  gmActor/tests/test_serialization.cpp -o test_gmActor_serial.exe
```

---

## Module reference

---

### `core/Ids.hpp`

All identifiers in `gmActor` are `std::string` type aliases.

```cpp
#include "gmActor/core/Ids.hpp"
```

| Alias | Meaning |
|-------|---------|
| `ActorId` | Unique actor identifier |
| `FactionId` | Faction / team group |
| `AreaId` | Location reference (`gmMap`) |
| `ItemId` | Canonical item definition |
| `ItemInstanceId` | Unique runtime item instance |
| `CardId` | Card reference (`gmDeck`) |
| `DeckInstanceId` | Deck instance reference (`gmDeck`) |
| `StatusId` | Status effect |
| `ModifierId` | Modifier definition / instance |
| `TraitId` | Passive trait |
| `AffiliationId` | Sub-faction |
| `MonsterTypeId` | Monster archetype |
| `MonsterGroupId` | Monster group |
| `MonsterInstanceId` | Individual monster body |
| `ObjectiveId` | Mission objective |
| `SourceId` | Effect / modifier origin |
| `Tag` | Lightweight classification tag (`std::string`) |

---

### `core/Enums.hpp`

```cpp
#include "gmActor/core/Enums.hpp"
```

#### `ActorKind`

| Value | Description |
|-------|-------------|
| `HERO` | Player-controlled hero |
| `ALLY_NPC` | Allied non-player character |
| `MONSTER_INSTANCE` | Individual targetable monster body |
| `MONSTER_GROUP` | Group acting as a unit on the timeline |
| `BOSS` | Boss extension wrapping a group + body |
| `MISSION_SYSTEM` | Scripted environment / event source |

#### `AreaPosition`

| Value | Description |
|-------|-------------|
| `FRONTLINE` | Front rank |
| `BACKLINE` | Back rank |
| `NONE` | Position not applicable |

#### `ActorLifeState`

| Value | Description |
|-------|-------------|
| `ACTIVE` | Alive and able to participate |
| `KO` | Knocked out — alive but cannot act |
| `DEAD` | Permanently removed from combat |
| `REMOVED` | Removed from scenario (fled, captured …) |

#### `ItemKind`

`WEAPON`, `ARMOR`, `TRINKET`, `CONSUMABLE`, `RELIC`, `MISSION_ITEM`, `MATERIAL`, `GENERIC`

#### `EquipmentSlot`

`MAIN_HAND`, `OFF_HAND`, `ARMOR`, `TRINKET_1`, `TRINKET_2`, `RELIC`, `NONE`

#### `ModifierOperation`

| Value | Behaviour |
|-------|-----------|
| `ADD` | Adds to base value |
| `SUBTRACT` | Subtracts from base value |
| `MULTIPLY` | Multiplies the post-ADD result |
| `SET` | Overrides base (last SET wins) |

#### `ModifierDurationKind`

`PERMANENT`, `UNTIL_NEXT_ACTIVATION`, `UNTIL_TARGET_NEXT_ACTIVATION`, `UNTIL_TIME`, `WHILE_IN_AREA`, `WHILE_IN_POSITION`, `MANUAL_REMOVE`

---

### `core/Tags.hpp`

```cpp
#include "gmActor/core/Tags.hpp"
```

Free functions on `std::vector<Tag>`:

```cpp
bool has_tag(const std::vector<Tag>& tags, const Tag& tag);
void add_tag(std::vector<Tag>& tags, const Tag& tag);     // no-op if already present
void remove_tag(std::vector<Tag>& tags, const Tag& tag);  // no-op if absent
```

---

### `core/Errors.hpp`

```cpp
#include "gmActor/core/Errors.hpp"
```

Exception hierarchy (all inherit from `std::runtime_error` via `ActorError`):

| Exception | Thrown when |
|-----------|-------------|
| `ActorError` | Base class |
| `UnknownActorError` | `actor_id` not found in the store |
| `DuplicateActorError` | `actor_id` already registered |
| `InvalidActorKindError` | Operation is not valid for the actor's kind (e.g. `common()` on a group) |
| `UnknownItemError` | `ItemInstanceId` not found in an `InventoryState` |
| `InvalidEquipmentSlotError` | Slot is `NONE`, or attempting to equip to an occupied slot |

---

### `stats/StatBlock`

```cpp
#include "gmActor/stats/StatBlock.hpp"
```

Generic string-keyed map of base numeric stat values.  Stores **base** values only; compute effective values with `apply_modifiers()`.

```cpp
StatBlock sb;
sb.set("base_damage",   3.0);
sb.set("base_movement", 2.0);

double dmg = sb.get("base_damage");       // 3.0
double def = sb.get("defense", 1.0);      // 1.0 (default)
bool   has  = sb.has("base_damage");      // true

sb.remove("base_damage");
const std::unordered_map<std::string, double>& raw = sb.data();
```

| Method | Description |
|--------|-------------|
| `void set(key, value)` | Set or overwrite |
| `double get(key, default=0.0) const` | Retrieve or return default |
| `bool has(key) const` | Key presence check |
| `void remove(key)` | Remove entry (no-op if absent) |
| `const unordered_map<string,double>& data() const` | Raw map access |

---

### `stats/Health.hpp`

```cpp
#include "gmActor/stats/Health.hpp"
```

Free functions on `ActorStateCommon&`:

```cpp
bool has_health(const ActorStateCommon& actor);    // true if max_hp > 0
int  missing_hp(const ActorStateCommon& actor);    // max_hp - current_hp (≥ 0)
bool is_alive(const ActorStateCommon& actor);      // life_state == ACTIVE
bool is_ko(const ActorStateCommon& actor);         // life_state == KO

void set_hp(ActorStateCommon& actor, int value);          // clamp to [0, max_hp]
void damage_hp(ActorStateCommon& actor, int amount);      // negative → no-op
void heal_hp(ActorStateCommon& actor, int amount);        // negative → no-op
```

> **Important:** HP helpers **do not** transition `life_state`.  
> The game engine decides whether 0 HP means KO, DEAD, or something else.

---

### `modifiers/Modifier.hpp`

```cpp
#include "gmActor/modifiers/Modifier.hpp"
```

#### `ModifierDefinition`

Immutable shared template (loaded from data files):

| Field | Type | Description |
|-------|------|-------------|
| `id` | `ModifierId` | Unique identifier |
| `name` | `string` | Human-readable label |
| `stat_key` | `string` | Target stat (e.g. `"base_damage"`) |
| `operation` | `ModifierOperation` | Mathematical operation |
| `value` | `double` | Magnitude |
| `tags` | `vector<Tag>` | Classification tags |

#### `ModifierInstance`

Mutable runtime application of a modifier to one actor:

| Field | Type | Description |
|-------|------|-------------|
| `id` | `ModifierId` | Matches a `ModifierDefinition.id` |
| `source_id` | `SourceId` | Who applied this modifier |
| `stat_key` | `string` | Target stat |
| `operation` | `ModifierOperation` | Operation |
| `value` | `double` | Magnitude (may differ from definition) |
| `duration_kind` | `ModifierDurationKind` | Expiry rule |
| `expires_at_time` | `int` | Tick for `UNTIL_TIME`, else `-1` |

#### `apply_modifiers()`

```cpp
double apply_modifiers(
    double base_value,
    const std::string& stat_key,
    const std::vector<ModifierInstance>& modifiers);
```

Evaluation order: **SET** (last wins) → **ADD / SUBTRACT** → **MULTIPLY**.  
Only modifiers whose `stat_key` matches the argument are applied.

---

### `statuses/StatusDefinition.hpp` & `StatusInstance.hpp`

```cpp
#include "gmActor/statuses/StatusDefinition.hpp"
#include "gmActor/statuses/StatusInstance.hpp"
```

#### `StatusDefinition` (immutable, from data)

| Field | Type | Default |
|-------|------|---------|
| `id` | `StatusId` | |
| `name` | `string` | |
| `description` | `string` | |
| `tags` | `vector<Tag>` | `{}` |
| `stackable` | `bool` | `false` |

#### `StatusInstance` (mutable, runtime)

| Field | Type | Default |
|-------|------|---------|
| `id` | `StatusId` | |
| `source_id` | `SourceId` | |
| `stacks` | `int` | `1` |
| `duration_kind` | `ModifierDurationKind` | |
| `expires_at_time` | `int` | `-1` |
| `modifiers` | `vector<ModifierInstance>` | `{}` |

---

### `statuses/StatusContainer`

```cpp
#include "gmActor/statuses/StatusContainer.hpp"
```

Manages active statuses on a single actor.

```cpp
StatusContainer sc;

// Add / stack
sc.add(instance, /*stackable=*/false);  // replaces if same id exists
sc.add(instance, /*stackable=*/true);   // increments stacks if same id exists

// Query
bool present = sc.has("poisoned");
std::optional<StatusInstance> s = sc.get("poisoned");
const std::vector<StatusInstance>& all = sc.all();

// Remove
sc.remove("poisoned");
sc.clear();
```

Stackability rules:

| `stackable` | Duplicate behaviour |
|-------------|---------------------|
| `false` | Existing instance **replaced** (`stacks = 1`) |
| `true` | `stacks` incremented by `incoming.stacks` |

---

### `items/ItemDefinition.hpp` & `ItemState.hpp`

```cpp
#include "gmActor/items/ItemDefinition.hpp"
#include "gmActor/items/ItemState.hpp"
```

#### `ItemDefinition` (immutable, from data)

| Field | Type |
|-------|------|
| `id` | `ItemId` |
| `name` | `string` |
| `kind` | `ItemKind` |
| `tags` | `vector<Tag>` |
| `granted_cards` | `vector<CardId>` |
| `passive_modifiers` | `vector<ModifierDefinition>` |
| `use_effect_refs` | `vector<string>` |
| `consumable` | `bool` |
| `max_charges` | `int` |

#### `ItemState` (mutable, runtime)

| Field | Type | Default |
|-------|------|---------|
| `instance_id` | `ItemInstanceId` | |
| `item_id` | `ItemId` | |
| `owner_id` | `ActorId` | |
| `equipped` | `bool` | `false` |
| `slot` | `EquipmentSlot` | `NONE` |
| `charges` | `int` | `0` |
| `exhausted` | `bool` | `false` |

---

### `items/InventoryState`

```cpp
#include "gmActor/items/InventoryState.hpp"
```

Ordered list of item instance IDs carried by one actor.

```cpp
InventoryState inv;

inv.add("sword_01");           // append if not present
inv.remove("sword_01");        // throws UnknownItemError if not found
bool has  = inv.contains("sword_01");
int  n    = inv.count();
const std::vector<ItemInstanceId>& ids = inv.items();
```

---

### `items/EquipmentState`

```cpp
#include "gmActor/items/EquipmentState.hpp"
```

Slot → instance mapping for equipped items.

```cpp
EquipmentState eq;

eq.equip(EquipmentSlot::MAIN_HAND, "sword_01");   // throws InvalidEquipmentSlotError if slot == NONE or occupied
eq.unequip(EquipmentSlot::MAIN_HAND);             // no-op if empty

bool present                            = eq.has_equipped(EquipmentSlot::MAIN_HAND);
std::optional<ItemInstanceId> item      = eq.equipped_at(EquipmentSlot::MAIN_HAND);
std::vector<ItemInstanceId>   all_items = eq.all_equipped();
```

---

### `actors/ActorStateCommon`

```cpp
#include "gmActor/actors/ActorStateCommon.hpp"
```

Shared mutable state present on all actor kinds **except `MonsterGroupState`**.

| Field | Type | Default |
|-------|------|---------|
| `actor_id` | `ActorId` | |
| `kind` | `ActorKind` | |
| `display_name` | `string` | |
| `faction_id` | `FactionId` | |
| `enabled` | `bool` | `true` |
| `removed` | `bool` | `false` |
| `can_act` | `bool` | `true` |
| `can_be_targeted` | `bool` | `true` |
| `life_state` | `ActorLifeState` | `ACTIVE` |
| `timeline_position` | `int` | `0` |
| `tie_break_rank` | `int` | `0` |
| `area_id` | `AreaId` | |
| `area_position` | `AreaPosition` | `NONE` |
| `current_hp` | `int` | `0` |
| `max_hp` | `int` | `0` |
| `statuses` | `vector<StatusInstance>` | `{}` |
| `active_modifiers` | `vector<ModifierInstance>` | `{}` |
| `tags` | `vector<Tag>` | `{}` |

---

### Actor state structs

```cpp
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/AllyState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/actors/BossState.hpp"
#include "gmActor/actors/MissionSystemState.hpp"
```

#### `HeroState`

Embeds `ActorStateCommon common`.

| Field | Type | Default |
|-------|------|---------|
| `common` | `ActorStateCommon` | |
| `level` | `int` | `1` |
| `hand_limit` | `int` | `0` |
| `memory_limit` | `int` | `0` |
| `mission_deck_limit` | `int` | `0` |
| `total_deck_id` | `DeckInstanceId` | |
| `mission_deck_id` | `DeckInstanceId` | |
| `equipment` | `EquipmentState` | |
| `inventory` | `InventoryState` | |
| `affiliations` | `vector<AffiliationId>` | `{}` |
| `is_ko` | `bool` | `false` |
| `carried_mission_items` | `vector<ItemInstanceId>` | `{}` |

#### `AllyState`

| Notable field | Type |
|---------------|------|
| `common` | `ActorStateCommon` |
| `traits` | `vector<TraitId>` |
| `carried_items` | `vector<ItemInstanceId>` |

#### `MonsterInstanceState`

| Notable field | Type | Default |
|---------------|------|---------|
| `common` | `ActorStateCommon` | |
| `monster_type_id` | `MonsterTypeId` | |
| `group_id` | `MonsterGroupId` | |
| `elite` | `bool` | `false` |
| `boss_part` | `bool` | `false` |
| `base_damage` | `int` | `1` |
| `base_movement` | `int` | `2` |
| `traits` | `vector<TraitId>` | `{}` |
| `loot_ref` | `string` | |

#### `MonsterGroupState`

> **Design decision D1:** `MonsterGroupState` does **not** embed `ActorStateCommon`.  
> Calling `ActorStore::common(group_id)` throws `InvalidActorKindError`.

| Field | Type | Default |
|-------|------|---------|
| `actor_id` | `ActorId` | |
| `group_id` | `MonsterGroupId` | |
| `monster_type_id` | `MonsterTypeId` | |
| `display_name` | `string` | |
| `enabled` | `bool` | `true` |
| `removed` | `bool` | `false` |
| `timeline_position` | `int` | `0` |
| `tie_break_rank` | `int` | `0` |
| `members` | `vector<MonsterInstanceId>` | `{}` |
| `behavior_deck_id` | `DeckInstanceId` | |
| `active_behavior_card_id` | `CardId` | |
| `behavior_discard_id` | `DeckInstanceId` | |
| `active_group_modifiers` | `vector<ModifierInstance>` | `{}` |
| `tags` | `vector<Tag>` | `{}` |

#### `BossState`

Keyed in `ActorStore` by `controller_group_id` (design decision D2).

| Field | Type | Default |
|-------|------|---------|
| `body_instance_id` | `MonsterInstanceId` | |
| `controller_group_id` | `MonsterGroupId` | |
| `phase_index` | `int` | `0` |
| `rage` | `int` | `0` |
| `linked_objectives` | `vector<ObjectiveId>` | `{}` |
| `tags` | `vector<Tag>` | `{}` |

#### `MissionSystemState`

At most one per `ActorStore`.

| Field | Type | Default |
|-------|------|---------|
| `actor_id` | `ActorId` | `"system_mission"` |
| `display_name` | `string` | `"Mission System"` |
| `enabled` | `bool` | `true` |
| `tags` | `vector<Tag>` | `{}` |

---

### `actors/ActorStore`

```cpp
#include "gmActor/actors/ActorStore.hpp"
```

Central owning registry for all actor states.  All states are accessed by ID.

#### Registration

```cpp
ActorStore store;

store.add_hero(hero);                     // throws DuplicateActorError if id exists
store.add_ally(ally);
store.add_monster_instance(monster);
store.add_monster_group(group);
store.add_boss(boss);                     // keyed by boss.controller_group_id
store.set_mission_system(system);         // replaces any previous instance
```

#### Existence and kind

```cpp
bool present = store.has_actor("hero_alice");
ActorKind k  = store.actor_kind("hero_alice"); // throws UnknownActorError if absent
```

#### Common state access

```cpp
ActorStateCommon& c = store.common("hero_alice");  // mutable
// throws InvalidActorKindError for MONSTER_GROUP (D1)
// throws UnknownActorError if not found
```

#### Typed accessors

```cpp
HeroState&            h  = store.hero("hero_alice");
AllyState&            a  = store.ally("ally_bob");
MonsterInstanceState& mi = store.monster_instance("goblin_01");
MonsterGroupState&    mg = store.monster_group("goblin_group_A");
BossState&            b  = store.boss("dragon_group");       // keyed by controller_group_id
MissionSystemState&   ms = store.mission_system();
```

All accessors have `const` overloads and throw:
- `UnknownActorError` — ID not found
- `InvalidActorKindError` — Wrong type for the requested accessor

#### Bulk queries

```cpp
std::vector<ActorId> ids  = store.timeline_actor_ids();     // all enabled non-removed actors
std::vector<ActorId> area = store.actors_in_area("room_1"); // all actors in an area
std::vector<ActorId> tgt  = store.targetable_actors_in_area("room_1");
```

---

### `actors/ActorQueries.hpp`

```cpp
#include "gmActor/actors/ActorQueries.hpp"
```

Pure const free functions — no mutation.

```cpp
// Classification
bool is_hero(store, id);
bool is_monster_group(store, id);
bool is_targetable(store, id);  // false for MONSTER_GROUP and MISSION_SYSTEM
bool can_act(store, id);        // common.can_act==true, or group.enabled && !removed

// Collections
std::vector<ActorId> actors_by_faction(store, faction_id);
std::vector<ActorId> actors_in_area(store, area_id);
std::vector<ActorId> living_heroes(store);              // life_state == ACTIVE
std::vector<ActorId> enabled_timeline_actors(store);    // enabled && !removed
```

---

### `events/ActorEvents.hpp`

```cpp
#include "gmActor/events/ActorEvents.hpp"
```

Struct-only payloads; zero dependency on `gmDispatch`.  The game engine
publishes events using these structs.

#### Event type constants

| Constant | Value | Trigger |
|----------|-------|---------|
| `EVT_HP_CHANGED` | `"gmActor.actor.hp_changed"` | `current_hp` changed |
| `EVT_STATUS_ADDED` | `"gmActor.actor.status_added"` | Status added to actor |
| `EVT_STATUS_REMOVED` | `"gmActor.actor.status_removed"` | Status removed from actor |
| `EVT_MOVED_AREA` | `"gmActor.actor.moved_area"` | `area_id` changed |
| `EVT_POSITION_CHANGED` | `"gmActor.actor.position_changed"` | `area_position` changed |
| `EVT_ITEM_EQUIPPED` | `"gmActor.actor.item_equipped"` | Item equipped |
| `EVT_ITEM_UNEQUIPPED` | `"gmActor.actor.item_unequipped"` | Item unequipped |
| `EVT_LIFE_STATE_CHANGED` | `"gmActor.actor.life_state_changed"` | `life_state` changed |

#### Payload structs

```cpp
struct HpChangedEvent           { ActorId actor_id; int old_hp; int new_hp; int max_hp; SourceId source_id; };
struct StatusAddedEvent         { ActorId actor_id; StatusId status_id; int stacks; SourceId source_id; };
struct StatusRemovedEvent       { ActorId actor_id; StatusId status_id; };
struct MovedAreaEvent           { ActorId actor_id; AreaId old_area; AreaId new_area; };
struct PositionChangedEvent     { ActorId actor_id; AreaPosition old_position; AreaPosition new_position; };
struct ItemEquippedEvent        { ActorId actor_id; ItemInstanceId item_instance_id; EquipmentSlot slot; };
struct ItemUnequippedEvent      { ActorId actor_id; ItemInstanceId item_instance_id; EquipmentSlot slot; };
struct LifeStateChangedEvent    { ActorId actor_id; ActorLifeState old_state; ActorLifeState new_state; };
```

---

### `adapters/GmFlowActorAdapter.hpp`

```cpp
#include "gmActor/adapters/GmFlowActorAdapter.hpp"
```

Bridges `gmActor` state to `gmFlow` flow descriptors.

```cpp
// Map ActorKind → gmFlow::ActorType
// HERO → PLAYER | ALLY_NPC/MONSTER_INSTANCE/MONSTER_GROUP/BOSS → BOT | MISSION_SYSTEM → SYSTEM
gmFlow::ActorType t = to_flow_actor_type(ActorKind::HERO);

// Build a gmFlow::Actor for any common-bearing actor
// (throws InvalidActorKindError for MONSTER_GROUP)
gmFlow::Actor fa = make_flow_actor(store, "hero_alice");

// Build a gmFlow::Actor for a MonsterGroup (no ActorStateCommon)
gmFlow::Actor ga = make_flow_actor_from_group(group_state);

// Populate a gmFlow::ActorRegistry from the full store (Phase 4)
populate_flow_registry(store, registry);
```

---

### `serialization/ActorJson.hpp`

```cpp
#include "gmActor/serialization/ActorJson.hpp"
```

Declares `to_json` / `from_json` for all gmActor types (nlohmann/json ADL
convention).  Include this header to enable `nlohmann::json j = my_hero;`
syntax.

Covered types: all enums, `ModifierDefinition`, `ModifierInstance`,
`StatusDefinition`, `StatusInstance`, `ItemDefinition`, `ItemState`,
`InventoryState`, `EquipmentState`, `ActorStateCommon`, `HeroState`,
`AllyState`, `MonsterInstanceState`, `MonsterGroupState`, `BossState`,
`MissionSystemState`, `ActorStore`.

---

### `serialization/ActorSaveEnvelope.hpp`

```cpp
#include "gmActor/serialization/ActorSaveEnvelope.hpp"
```

Versioned envelope for file persistence:

```cpp
struct ActorSaveEnvelope {
    std::string gmactor_version = "0.1.0";
    ActorStore  store;
};
```

Supports `to_json` / `from_json`.  Use with `gmSave::save_versioned` /
`gmSave::load_versioned` for forward-compatible save files.

---

## Design decisions summary

| # | Decision |
|---|----------|
| D1 | `common(group_id)` throws `InvalidActorKindError` — groups accessed only via `monster_group()` |
| D2 | `BossState` keyed by `controller_group_id` in `ActorStore` |
| D3 | `AllyState` appears in `timeline_actor_ids()` only if `common.can_act == true` |
| D4 | `ActorEvents.hpp` is struct-only — zero `gmDispatch` dependency |
| D5 | `GmFlowActorAdapter.hpp` ships in V1 with real implementation |
| D6 | Serialization uses `gmSave/json.hpp` + `gmSave/gmSave.hpp`; versioned envelope provided |
| D7 | `.cpp` files for: `StatusContainer`, `InventoryState`, `EquipmentState`, `ActorStore`, `ActorQueries`, `ActorJson`, `Modifier`, `StatBlock`, `Health` |
| D8 | `ModifierOperation` and `EquipmentSlot` defined once in `core/Enums.hpp` |
| D9 | `common()` on unknown ID throws `UnknownActorError` |
| D10 | HP helpers clamp only; `life_state` transitions are the engine's responsibility |
| D11 | Non-stackable status re-add → replaces existing instance (`stacks = 1`) |
| D12 | Stackable status re-add → increments `stacks` |
| D13 | Modifier evaluation order: SET (last wins) → ADD/SUBTRACT → MULTIPLY |
| D14 | Negative damage / heal amounts → no-op |
| D15 | All IDs are `std::string` aliases |
| D16 | `stat_key` is `std::string` (e.g. `"hp_max"`, `"base_damage"`) |
