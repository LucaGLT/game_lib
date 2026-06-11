# gmActor — AI Implementation Instructions

**Version:** 0.1.0  
**Status:** Planning / Implementation Brief  
**Language:** C++17 Standard  
**Library name:** `gmActor`  
**Namespace:** `gmActor`  
**Goal:** provide reusable actor-state, statistics, status, item, equipment, and actor-registry primitives for a mission-based tabletop engine.

---

## 0. Mandatory constraints for the implementing AI

Follow the style and development discipline already used in the other `gmXxx` libraries, especially `gmFlow`, `gmDispatch`, `gmSave`, `gmDeck`, and `gmCompDeck`.

The implementation must:

- Use **C++17 standard library only**, unless explicitly integrating with another local `gmXxx` library.
- Keep the library **generic**. Do not hardcode rules from a specific game title.
- Use **string IDs** for external/debuggable references, following the style used by `gmFlow`.
- Separate immutable definitions from mutable runtime state where appropriate.
- Keep flow logic out of this library. `gmActor` does not decide whose turn it is.
- Keep card/deck lifecycle out of this library. Store deck IDs and card IDs only.
- Keep map topology out of this library. Store `AreaId` / location IDs only.
- Keep effect resolution out of this library. Store active status/modifier data and provide safe mutation helpers.
- Add Doxygen-style comments to public API symbols.
- Implement in phases: headers and stubs first, then bodies, then tests.
- Preserve compatibility with `gmSave` by providing `to_json` / `from_json` helpers or clearly documented serialization hooks.
- Prefer explicit types, enums, and value objects over raw maps of strings where the domain is known.

This library must be usable by a tactical dungeon-crawler mission engine where actors may include:

- player-controlled heroes;
- allied non-player actors;
- individual monster miniatures;
- monster groups that act as a single timeline actor;
- bosses;
- mission/system actors for scripted events.

Never mention any specific game title in code, comments, docs, tests, or examples.

---

## 1. Design philosophy

`gmActor` manages **who exists in the game state** and **what mutable actor-related data they currently have**.

It must not manage:

- turn order;
- action windows;
- card play validation;
- deck shuffling/drawing;
- map graph traversal;
- combat rules;
- AI behavior resolution;
- scenario objectives.

Those responsibilities belong to `gmFlow`, `gmDeck` / `gmCompDeck`, `gmMap`, and the game-specific engine.

The conceptual layering should be:

```text
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

`gmFlow::Actor` should remain an immutable flow participant. `gmActor` provides the mutable game-state side of that actor.

---

## 2. Main design decision: actor descriptor vs actor state

The existing `gmFlow` library treats actors as flow participants. This is correct and must not be replaced.

`gmActor` should provide runtime actor state structures that can be referenced by `gmFlow::ActorId`.

Recommended distinction:

```text
gmFlow::Actor
- immutable flow descriptor
- id
- actor type
- display name
- no HP, no position, no cards, no inventory

gmActor::ActorStateCommon
- mutable actor-related game state
- HP
- timeline position
- area/location reference
- front/back position
- statuses
- modifiers
- tags
```

This separation is mandatory.

---

## 3. File structure

Create the following structure:

```text
gmActor/
├── PLAN.md                         ← optional plan generated from this file
├── gmActor_API.md                  ← API manual generated after stubs
│
├── core/
│   ├── Ids.hpp                     ← ActorId, FactionId, AreaId, ItemId, etc.
│   ├── Enums.hpp                   ← ActorKind, AreaPosition, ItemKind, etc.
│   ├── Result.hpp                  ← ValidationResult / MutationResult if needed
│   └── Tags.hpp                    ← lightweight tag helpers if useful
│
├── stats/
│   ├── StatBlock.hpp               ← generic stat map/value object
│   ├── Health.hpp                  ← current/max HP helpers
│   └── TimelineStats.hpp           ← timeline position, tie-break rank
│
├── modifiers/
│   ├── Modifier.hpp                ← ModifierDefinition / ModifierInstance
│   ├── ModifierOperation.hpp       ← ADD, SUBTRACT, MULTIPLY, SET, etc.
│   └── ModifierSource.hpp          ← source references
│
├── statuses/
│   ├── StatusDefinition.hpp
│   ├── StatusInstance.hpp
│   └── StatusContainer.hpp
│
├── items/
│   ├── ItemDefinition.hpp
│   ├── ItemState.hpp
│   ├── InventoryState.hpp
│   ├── EquipmentState.hpp
│   └── EquipmentSlot.hpp
│
├── actors/
│   ├── ActorStateCommon.hpp
│   ├── HeroState.hpp
│   ├── AllyState.hpp
│   ├── MonsterInstanceState.hpp
│   ├── MonsterGroupState.hpp
│   ├── BossState.hpp
│   ├── MissionSystemState.hpp
│   ├── ActorStore.hpp
│   └── ActorQueries.hpp
│
├── serialization/
│   ├── ActorJson.hpp               ← to_json/from_json declarations
│   └── ActorSaveEnvelope.hpp       ← versioned state envelope if needed
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

Use `.cpp` files where needed, but simple value objects can remain header-only if consistent with the existing project style.

---

## 4. ID aliases

Create `gmActor/core/Ids.hpp`.

Use string aliases:

```cpp
namespace gmActor {

using ActorId           = std::string;
using FactionId         = std::string;
using AreaId            = std::string;
using ItemId            = std::string;
using ItemInstanceId    = std::string;
using CardId            = std::string;
using DeckInstanceId    = std::string;
using StatusId          = std::string;
using ModifierId        = std::string;
using TraitId           = std::string;
using AffiliationId     = std::string;
using MonsterTypeId     = std::string;
using MonsterGroupId    = std::string;
using MonsterInstanceId = std::string;
using ObjectiveId       = std::string;
using SourceId          = std::string;
using Tag               = std::string;

} // namespace gmActor
```

If integration with `gmFlow` is compiled in, `gmFlow::ActorId` and `gmActor::ActorId` should both be `std::string` and trivially convertible.

Do not introduce integer-only IDs in this library.

---

## 5. Enums

Create `gmActor/core/Enums.hpp`.

Required enums:

```cpp
namespace gmActor {

enum class ActorKind {
    HERO,
    ALLY_NPC,
    MONSTER_INSTANCE,
    MONSTER_GROUP,
    BOSS,
    MISSION_SYSTEM
};

enum class AreaPosition {
    FRONTLINE,
    BACKLINE,
    NONE
};

enum class ActorLifeState {
    ACTIVE,
    KO,
    DEAD,
    REMOVED
};

enum class ItemKind {
    WEAPON,
    ARMOR,
    TRINKET,
    CONSUMABLE,
    RELIC,
    MISSION_ITEM,
    MATERIAL,
    GENERIC
};

enum class EquipmentSlot {
    MAIN_HAND,
    OFF_HAND,
    ARMOR,
    TRINKET_1,
    TRINKET_2,
    RELIC,
    NONE
};

enum class ModifierOperation {
    ADD,
    SUBTRACT,
    MULTIPLY,
    SET
};

enum class ModifierDurationKind {
    PERMANENT,
    UNTIL_NEXT_ACTIVATION,
    UNTIL_TARGET_NEXT_ACTIVATION,
    UNTIL_TIME,
    WHILE_IN_AREA,
    WHILE_IN_POSITION,
    MANUAL_REMOVE
};

} // namespace gmActor
```

Add conversion helpers only if existing libraries already use that pattern. Otherwise keep enums simple.

---

## 6. Common actor state

Create `gmActor/actors/ActorStateCommon.hpp`.

This structure contains data common to all targetable or activatable actors.

```cpp
namespace gmActor {

struct ActorStateCommon {
    ActorId      actor_id;
    ActorKind    kind = ActorKind::HERO;
    std::string  display_name;

    FactionId    faction_id;

    bool enabled          = true;
    bool removed          = false;
    bool can_act          = true;
    bool can_be_targeted  = true;

    ActorLifeState life_state = ActorLifeState::ACTIVE;

    int timeline_position = 0;
    int tie_break_rank    = 0;

    AreaId       area_id;
    AreaPosition area_position = AreaPosition::NONE;

    int current_hp = 0;
    int max_hp     = 0;

    std::vector<StatusInstance>   statuses;
    std::vector<ModifierInstance> active_modifiers;
    std::vector<Tag>              tags;
};

} // namespace gmActor
```

Common fields are:

```text
- identity
- actor kind
- faction
- enabled/removed flags
- can_act flag
- can_be_targeted flag
- life state
- timeline position
- tie-break rank
- area/location ID
- internal area position: frontline/backline
- current/max HP
- statuses
- active modifiers
- tags
```

Rules:

- `MONSTER_GROUP` may have no HP of its own. In that case set `max_hp = 0`, `current_hp = 0`, and `can_be_targeted = false` unless the specific game says otherwise.
- `MISSION_SYSTEM` usually has no HP, no area, and cannot be targeted.
- Do not infer death/KO automatically in raw setters unless the method explicitly says it applies damage rules.
- The game-specific engine decides how `KO`, `DEAD`, and `REMOVED` affect play.

---

## 7. Health helpers

Create `gmActor/stats/Health.hpp`.

Provide small helper functions/classes for safe HP changes.

Required operations:

```cpp
bool has_health(const ActorStateCommon& actor);
int  missing_hp(const ActorStateCommon& actor);
bool is_alive(const ActorStateCommon& actor);
bool is_ko(const ActorStateCommon& actor);

void set_hp(ActorStateCommon& actor, int value);
void damage_hp(ActorStateCommon& actor, int amount);
void heal_hp(ActorStateCommon& actor, int amount);
```

Rules:

- Clamp HP between `0` and `max_hp`.
- Reject or no-op negative amounts. Prefer returning a result object over throwing for common rule failures.
- Do not decide whether `0 HP` means KO or death unless the method name is explicitly game-rule-specific. Generic HP helpers only set numeric HP.

---

## 8. Status model

Create:

```text
gmActor/statuses/StatusDefinition.hpp
gmActor/statuses/StatusInstance.hpp
gmActor/statuses/StatusContainer.hpp
```

Definitions are immutable. Instances are mutable runtime applications of a status.

```cpp
struct StatusDefinition {
    StatusId id;
    std::string name;
    std::string description;
    std::vector<Tag> tags;
    bool stackable = false;
};

struct StatusInstance {
    StatusId id;
    SourceId source_id;
    int stacks = 1;
    ModifierDurationKind duration_kind = ModifierDurationKind::MANUAL_REMOVE;
    int expires_at_time = -1;
    std::vector<ModifierInstance> modifiers;
};
```

`StatusContainer` should provide:

```cpp
class StatusContainer {
public:
    bool has(const StatusId& id) const;
    std::optional<StatusInstance> get(const StatusId& id) const;
    void add(StatusInstance status, bool stackable);
    void remove(const StatusId& id);
    void clear();
    const std::vector<StatusInstance>& all() const;
};
```

Rules:

- If a status is not stackable, adding it again should refresh or replace it. Choose one behavior and document it. Recommended: replace instance and keep `stacks = 1`.
- If stackable, increment stacks or add according to explicit behavior. Recommended: increment stacks on same `StatusId`.
- Do not implement specific status effects such as burn damage or stun costs here. Store the data only.

---

## 9. Modifier model

Create:

```text
gmActor/modifiers/Modifier.hpp
gmActor/modifiers/ModifierOperation.hpp
gmActor/modifiers/ModifierSource.hpp
```

Recommended structures:

```cpp
struct ModifierDefinition {
    ModifierId id;
    std::string name;
    std::string stat_key;
    ModifierOperation operation = ModifierOperation::ADD;
    double value = 0.0;
    std::vector<Tag> tags;
};

struct ModifierInstance {
    ModifierId id;
    SourceId source_id;
    std::string stat_key;
    ModifierOperation operation = ModifierOperation::ADD;
    double value = 0.0;
    ModifierDurationKind duration_kind = ModifierDurationKind::MANUAL_REMOVE;
    int expires_at_time = -1;
};
```

Implement a generic evaluator:

```cpp
double apply_modifiers(double base_value,
                       const std::string& stat_key,
                       const std::vector<ModifierInstance>& modifiers);
```

Recommended order:

```text
1. SET, if present, last SET wins by vector order
2. ADD / SUBTRACT
3. MULTIPLY
```

Document this order.

Do not encode game-specific stat names as enum-only. Use `std::string stat_key` so games can define values such as:

```text
hp_max
base_damage
base_movement
hand_limit
memory_limit
time_cost
```

---

## 10. Hero state

Create `gmActor/actors/HeroState.hpp`.

A hero is a player-controlled or player-facing actor with deck references, equipment, inventory, and progression-related fields.

```cpp
struct HeroState {
    ActorStateCommon common;

    int level = 1;

    int hand_limit         = 0;
    int memory_limit       = 0;
    int mission_deck_limit = 0;

    DeckInstanceId total_deck_id;
    DeckInstanceId mission_deck_id;

    EquipmentState equipment;
    InventoryState inventory;

    std::vector<AffiliationId> affiliations;

    bool is_ko = false;
    std::vector<ItemInstanceId> carried_mission_items;
};
```

Hero-specific fields:

```text
- level
- hand limit
- memory limit
- mission deck limit
- total deck ID
- mission deck ID
- equipment
- inventory
- affiliations
- KO flag
- carried mission items
```

Do not store actual card zones here. Actual card zones belong to `gmCompDeck` or a game-specific deck manager. Store deck instance IDs only.

---

## 11. Allied non-player actor state

Create `gmActor/actors/AllyState.hpp`.

This can be small for V1:

```cpp
struct AllyState {
    ActorStateCommon common;
    std::vector<TraitId> traits;
    std::vector<ItemInstanceId> carried_items;
};
```

Allies may later receive deck references or behavior references, but do not add them until required.

---

## 12. Monster instance state

Create `gmActor/actors/MonsterInstanceState.hpp`.

A monster instance is a physical miniature or targetable enemy on the map.

```cpp
struct MonsterInstanceState {
    ActorStateCommon common;

    MonsterTypeId     monster_type_id;
    MonsterGroupId    group_id;

    bool elite     = false;
    bool boss_part = false;

    int base_damage   = 1;
    int base_movement = 2;

    std::vector<TraitId> traits;
    std::string loot_ref;
};
```

Monster-instance-specific fields:

```text
- monster type ID
- group ID
- elite flag
- boss-part flag
- base damage
- base movement
- traits
- optional loot reference
```

A monster instance may be targetable and damageable. Its group may be the actor that receives turns.

---

## 13. Monster group state

Create `gmActor/actors/MonsterGroupState.hpp`.

A monster group is often the actor that acts on the timeline. It controls one or more monster instances.

```cpp
struct MonsterGroupState {
    ActorId        actor_id;
    MonsterGroupId group_id;
    MonsterTypeId  monster_type_id;
    std::string    display_name;

    bool enabled = true;
    bool removed = false;

    int timeline_position = 0;
    int tie_break_rank    = 0;

    std::vector<MonsterInstanceId> members;

    DeckInstanceId behavior_deck_id;
    CardId         active_behavior_card_id;
    DeckInstanceId behavior_discard_id;

    std::vector<ModifierInstance> active_group_modifiers;
    std::vector<Tag> tags;
};
```

Monster-group-specific fields:

```text
- group ID
- monster type ID
- member monster instance IDs
- timeline position
- tie-break rank
- behavior deck ID
- active behavior card ID
- behavior discard ID
- group modifiers
- tags
```

Important model rule:

```text
MonsterGroup = actor that acts
MonsterInstance = physical target on the map
```

A group usually has no HP. HP belongs to monster instances.

---

## 14. Boss state

Create `gmActor/actors/BossState.hpp`.

For V1, a boss should be modelled as:

```text
Boss = MonsterGroup with one MonsterInstance
```

Still provide a light optional structure for future extensions:

```cpp
struct BossState {
    MonsterInstanceId body_instance_id;
    MonsterGroupId    controller_group_id;

    int phase_index = 0;
    int rage        = 0;

    std::vector<ObjectiveId> linked_objectives;
    std::vector<Tag> tags;
};
```

Do not implement boss phases in this library. Store state only.

---

## 15. Mission/system actor state

Create `gmActor/actors/MissionSystemState.hpp`.

A system actor represents scenario logic, events, traps, environment, or scripted sources.

```cpp
struct MissionSystemState {
    ActorId actor_id = "system_mission";
    std::string display_name = "Mission System";

    bool enabled = true;
    std::vector<Tag> tags;
};
```

A mission/system actor normally has:

```text
- no HP
- no area
- no inventory
- no deck
- no equipment
```

It may appear as `source_id` for statuses, modifiers, damage, events, and scripted effects.

---

## 16. Item definition and state

Create:

```text
gmActor/items/ItemDefinition.hpp
gmActor/items/ItemState.hpp
```

Definitions are immutable. States track ownership, charges, and equipped/exhausted state.

```cpp
struct ItemDefinition {
    ItemId id;
    std::string name;
    ItemKind kind = ItemKind::GENERIC;
    std::vector<Tag> tags;

    std::vector<CardId> granted_cards;
    std::vector<ModifierDefinition> passive_modifiers;

    // Store effect IDs or opaque effect payload references only.
    // Actual effect resolution belongs to the game-specific engine.
    std::vector<std::string> use_effect_refs;

    bool consumable = false;
    int max_charges = 0;
};

struct ItemState {
    ItemInstanceId instance_id;
    ItemId item_id;
    ActorId owner_id;

    bool equipped = false;
    EquipmentSlot slot = EquipmentSlot::NONE;

    int charges = 0;
    bool exhausted = false;
};
```

Rules:

- `ItemDefinition` may grant cards by ID, but does not create or manage those cards.
- `ItemDefinition` may reference effect payloads by string or opaque reference, but does not resolve them.
- `ItemState` tracks runtime values such as charges and exhaustion.
- Mission items should use `ItemKind::MISSION_ITEM`.

---

## 17. Inventory and equipment

Create:

```text
gmActor/items/InventoryState.hpp
gmActor/items/EquipmentState.hpp
gmActor/items/EquipmentSlot.hpp
```

Recommended API:

```cpp
class InventoryState {
public:
    void add(ItemInstanceId id);
    void remove(const ItemInstanceId& id);
    bool contains(const ItemInstanceId& id) const;
    const std::vector<ItemInstanceId>& items() const;
};

class EquipmentState {
public:
    bool has_equipped(EquipmentSlot slot) const;
    std::optional<ItemInstanceId> equipped_at(EquipmentSlot slot) const;

    void equip(EquipmentSlot slot, ItemInstanceId item);
    void unequip(EquipmentSlot slot);
    std::vector<ItemInstanceId> all_equipped() const;
};
```

Rules:

- Do not validate whether an item is allowed in a slot unless item definitions are provided to the method. Prefer leaving slot legality to the game-specific engine.
- Equipment should store item instance IDs, not item definitions.
- Inventory should store item instance IDs.

---

## 18. ActorStore

Create `gmActor/actors/ActorStore.hpp`.

This is the central container for all actor-related state.

Recommended internal storage:

```cpp
class ActorStore {
public:
    void add_hero(HeroState hero);
    void add_ally(AllyState ally);
    void add_monster_instance(MonsterInstanceState monster);
    void add_monster_group(MonsterGroupState group);
    void add_boss(BossState boss);
    void set_mission_system(MissionSystemState system);

    bool has_actor(const ActorId& id) const;
    ActorKind actor_kind(const ActorId& id) const;

    ActorStateCommon&       common(const ActorId& id);
    const ActorStateCommon& common(const ActorId& id) const;

    HeroState&       hero(const ActorId& id);
    const HeroState& hero(const ActorId& id) const;

    MonsterGroupState&       monster_group(const ActorId& id);
    const MonsterGroupState& monster_group(const ActorId& id) const;

    MonsterInstanceState&       monster_instance(const MonsterInstanceId& id);
    const MonsterInstanceState& monster_instance(const MonsterInstanceId& id) const;

    std::vector<ActorId> timeline_actor_ids() const;
    std::vector<ActorId> actors_in_area(const AreaId& area) const;
    std::vector<ActorId> targetable_actors_in_area(const AreaId& area) const;
};
```

Implementation detail:

- Since not every actor type has `ActorStateCommon` in the same shape, implement helper methods carefully.
- For `MonsterGroupState`, either provide a lightweight projected `ActorStateCommon` or provide separate group accessors. Do not pretend a group has HP if it does not.
- A group may be an actor for timeline purposes without being targetable.

Recommended storage:

```cpp
std::unordered_map<ActorId, HeroState> heroes_;
std::unordered_map<ActorId, AllyState> allies_;
std::unordered_map<MonsterInstanceId, MonsterInstanceState> monsters_;
std::unordered_map<ActorId, MonsterGroupState> monster_groups_;
std::unordered_map<ActorId, BossState> bosses_;
std::optional<MissionSystemState> mission_system_;
```

Add indexes only after needed.

---

## 19. Actor queries

Create `gmActor/actors/ActorQueries.hpp`.

Implement free functions or a query helper class:

```cpp
bool is_hero(const ActorStore& store, const ActorId& id);
bool is_monster_group(const ActorStore& store, const ActorId& id);
bool is_targetable(const ActorStore& store, const ActorId& id);
bool can_act(const ActorStore& store, const ActorId& id);

std::vector<ActorId> actors_by_faction(const ActorStore& store, const FactionId& faction);
std::vector<ActorId> actors_in_area(const ActorStore& store, const AreaId& area);
std::vector<ActorId> living_heroes(const ActorStore& store);
std::vector<ActorId> enabled_timeline_actors(const ActorStore& store);
```

These are pure queries and must not mutate state.

---

## 20. Integration with gmFlow

`gmActor` must not include heavy `gmFlow` dependencies unless necessary.

Recommended approach:

- Keep `gmActor` independent.
- Provide optional adapter helpers in a separate header if needed:

```text
gmActor/adapters/GmFlowActorAdapter.hpp
```

Adapter responsibilities:

```text
- Convert gmActor actor IDs to gmFlow actors.
- Build gmFlow actor descriptors from gmActor state.
- Expose timeline actors to a timeline flow controller.
```

A monster group should be eligible as a `gmFlow::Actor` because it acts as a unit on the timeline.

A monster instance may or may not be a `gmFlow::Actor`, depending on the game. In the target mission engine, monster instances are usually targetable bodies, while monster groups are timeline actors.

---

## 21. Integration with gmDeck / gmCompDeck

`gmActor` must not implement deck zones.

Store only:

```text
- DeckInstanceId
- CardId references
```

Examples:

```cpp
DeckInstanceId mission_deck_id;
DeckInstanceId behavior_deck_id;
CardId active_behavior_card_id;
```

The actual card lifecycle belongs to `gmCompDeck`.

A hero can reference:

```text
- total deck
- mission deck
```

A monster group can reference:

```text
- behavior deck
- behavior discard
- active behavior card
```

Do not duplicate hand, discard, play, memory, or banish zones inside `gmActor`.

---

## 22. Integration with gmMap

`gmActor` must not implement map topology.

Store only:

```text
- AreaId
- AreaPosition
```

The actual graph of areas/locations belongs to `gmMap` or a game-specific map module.

`AreaPosition` supports games that divide a location internally into abstract tactical positions such as front/back.

---

## 23. Integration with gmDispatch and gmLog

Do not implement a custom event bus in `gmActor`.

If actor changes need to be published, the game-specific engine should use `gmDispatch`.

Optional: define simple event payload structs in an adapter header:

```text
gmActor/events/ActorEvents.hpp
```

Possible events:

```text
gmActor.actor.hp_changed
gmActor.actor.status_added
gmActor.actor.status_removed
gmActor.actor.moved_area
gmActor.actor.position_changed
gmActor.actor.item_equipped
gmActor.actor.item_unequipped
gmActor.actor.life_state_changed
```

But keep event publishing outside core state containers unless the surrounding engine passes an event bus explicitly.

Logging should follow the existing `gmLog` approach from the game engine layer, not be hardcoded in every setter.

---

## 24. Serialization

Provide serialization support compatible with the local `gmSave` style.

The target pattern is:

```text
free function to_json(...)
free function from_json(...)
```

Implement serialization for:

```text
ActorStateCommon
HeroState
AllyState
MonsterInstanceState
MonsterGroupState
BossState
MissionSystemState
ItemDefinition
ItemState
InventoryState
EquipmentState
StatusDefinition
StatusInstance
ModifierDefinition
ModifierInstance
ActorStore
```

Add a versioned envelope if useful:

```cpp
struct ActorSaveEnvelope {
    std::string version = "0.1.0";
    ActorStore actor_store;
};
```

Never serialize raw pointers.

---

## 25. Error handling

Follow the explicit exception/result style of the existing libraries.

Recommended exceptions:

```cpp
class ActorError : public std::runtime_error { ... };
class UnknownActorError : public ActorError { ... };
class DuplicateActorError : public ActorError { ... };
class InvalidActorKindError : public ActorError { ... };
class UnknownItemError : public ActorError { ... };
class InvalidEquipmentSlotError : public ActorError { ... };
```

For common gameplay mutation failures, prefer result objects where the caller may recover.

For programming/data consistency errors, exceptions are acceptable.

---

## 26. Public API comments

Add Doxygen-style comments to all public classes and public methods.

Example:

```cpp
/**
 * @brief Mutable runtime state shared by all actor-like entities.
 *
 * This structure stores only actor-related state. It does not own flow,
 * deck, map, or rule-resolution logic.
 */
struct ActorStateCommon { ... };
```

---

## 27. Development phases

Follow the same ordered workflow used for `gmFlow`.

### Phase 1 — Planning

Deliverables:

- Confirm file tree.
- Confirm enum list.
- Confirm state structures.
- Confirm integration boundaries with `gmFlow`, `gmDeck`, `gmMap`, `gmDispatch`, `gmSave`, and `gmLog`.

### Phase 2 — Headers and stubs

Deliverables:

- Create all headers.
- Create `.cpp` files with stub bodies where needed.
- Add Doxygen comments.
- Ensure everything compiles with no implementation logic beyond safe placeholders.

### Phase 3 — API documentation

Deliverables:

- Generate or write `gmActor_API.md`.
- Include overview, architecture, class reference, examples, integration notes.

### Phase 4 — Implementation

Recommended order:

1. IDs and enums.
2. Health helpers.
3. Status model and container.
4. Modifier model and evaluator.
5. Item definition/state.
6. Inventory and equipment.
7. Actor state structs.
8. ActorStore.
9. ActorQueries.
10. Serialization helpers.

### Phase 5 — Tests

Add tests after each completed implementation block.

---

## 28. Required tests

Minimum tests:

### Actor common

```text
- create ActorStateCommon
- set location and area position
- set enabled/removed/can_act/can_be_targeted flags
- tags can be added and queried
```

### Health

```text
- damage clamps at 0
- healing clamps at max_hp
- negative damage/healing rejected or ignored as documented
- actor with max_hp 0 is treated as having no health
```

### Statuses

```text
- add non-stackable status
- add same non-stackable status replaces/refreshes as documented
- add stackable status increments stacks
- remove status
- clear statuses
```

### Modifiers

```text
- ADD modifier applies correctly
- SUBTRACT modifier applies correctly
- MULTIPLY modifier applies correctly
- SET modifier applies correctly
- documented operation order is respected
- modifiers for other stat_key are ignored
```

### Inventory and equipment

```text
- add item to inventory
- remove item from inventory
- equip item in slot
- unequip slot
- query all equipped items
```

### ActorStore

```text
- add hero
- add monster instance
- add monster group
- duplicate actor ID fails
- unknown actor access fails
- timeline_actor_ids returns heroes/groups/system as appropriate
- actors_in_area returns targetable bodies, not group controllers unless configured
```

### Serialization

```text
- serialize and deserialize HeroState
- serialize and deserialize MonsterGroupState
- serialize and deserialize ActorStore
- round-trip preserves IDs, HP, statuses, modifiers, inventory, equipment
```

---

## 29. Build commands

Use the project’s normal C++17 build style. Example command from repository root:

```powershell
clang++ -std=c++17 -I. `
    gmActor/tests/test_actor_common.cpp `
    gmActor/tests/test_health.cpp `
    gmActor/tests/test_status_container.cpp `
    gmActor/tests/test_modifier_container.cpp `
    gmActor/tests/test_inventory_equipment.cpp `
    gmActor/tests/test_actor_store.cpp `
    -o test_gmActor.exe ; ./test_gmActor.exe
```

If serialization tests require `gmSave`, include the relevant `gmSave` source files as done in the existing project.

---

## 30. Non-goals for V1

Do not implement these in `gmActor` V1:

```text
- turn order;
- timeline controller;
- action validation;
- card play rules;
- deck zones;
- map adjacency;
- pathfinding;
- combat resolution;
- monster AI;
- objective resolution;
- campaign progression;
- UI bindings;
- Python bindings;
- undo/redo;
- threaded event delivery.
```

These belong elsewhere.

---

## 31. Final expected result

After implementation, `gmActor` should provide a clean reusable C++17 layer for:

```text
- common actor runtime state;
- hero-specific state;
- ally state;
- monster instance state;
- monster group state;
- optional boss state;
- mission/system actor state;
- health helpers;
- statuses;
- modifiers;
- item definitions;
- item runtime state;
- inventory;
- equipment;
- actor storage and queries;
- serialization hooks.
```

It should integrate naturally with:

```text
- gmFlow for who can act and when;
- gmMap for where actors are;
- gmDeck/gmCompDeck for card and deck lifecycle;
- gmDispatch for event publication;
- gmSave for persistence;
- gmLog for structured logs.
```

The library must remain generic, testable, and independent from any single game title.
