# gmRules — AI Implementation Instructions

**Library name:** `gmRules`  
**Language:** C++17 Standard  
**Namespace:** `gmRules`  
**Status:** New library to implement  
**Purpose:** Generic tabletop rule primitive library  

---

## 1. Goal

Implement a new generic C++17 library named `gmRules`.

`gmRules` must provide reusable building blocks for tabletop game rules:

```text
Target    → who or what a rule affects
Condition → whether a rule can happen or trigger
Effect    → what a rule does
Status    → persistent effects, modifiers, durations, and stacking
```

The library must be generic. It must not contain game-specific concepts, names, story data, scenario-specific rules, UI code, or hardcoded gameplay assumptions.

The library must be designed to work with the existing `gmXxx` ecosystem, especially:

```text
gmFlow      → action/flow/session control
gmActor     → actors, health, stats, tags, status containers
gmMap       → locations, adjacency, topology
gmDeck      → deterministic decks
gmCompDeck  → multi-zone card lifecycle, including MEMORY if available
gmDispatch  → event dispatch
gmLog       → structured logging
gmSave      → JSON persistence/versioning
```

Follow the architectural style already used in libraries such as `gmFlow`, `gmDispatch`, `gmDeck`, `gmSave`, and `gmMap`:

- C++17 only.
- Standard library first.
- Clear namespace.
- Small focused headers.
- Public API documented with Doxygen-style comments.
- Explicit result types instead of silent failure.
- No UI coupling.
- No scripting language.
- No hidden global state.
- ID-based references.
- Deterministic behavior where possible.

---

## 2. Design Philosophy

`gmRules` is not a full game engine.

It does not decide turn order.  
It does not own the game session.  
It does not own the board.  
It does not own actors.  
It does not own decks.  
It does not own the UI.  
It does not implement AI.  

It provides a generic rule grammar usable by game-specific code.

```text
┌─────────────────────────────────────────────────────────────┐
│ Game-specific code                                          │
│ PlayCardAction, MoveAction, ScenarioEvent, TrapInteraction  │
└────────────────────┬────────────────────────────────────────┘
                     │ uses
┌────────────────────▼────────────────────────────────────────┐
│ gmRules                                                     │
│ TargetSpec · ConditionSpec · EffectSpec · StatusEngine      │
└────────────────────┬────────────────────────────────────────┘
                     │ accesses through adapters/context
┌────────────────────▼────────────────────────────────────────┐
│ gmActor · gmMap · gmDeck · gmCompDeck · gmDispatch · gmLog  │
└─────────────────────────────────────────────────────────────┘
```

A typical game action should work like this:

```cpp
ActionResult PlayCardAction::execute(gmFlow::GameContext& ctx) {
    auto& state = static_cast<MyGameState&>(ctx.state());

    MyRuleContext rules_ctx(state, ctx.event_bus());

    gmRules::gmRulesEngine rules;
    auto result = rules.resolve_effects(card.effects, rules_ctx);

    if (!result.succeeded()) {
        return gmFlow::ActionResult::failure(result.message());
    }

    return gmFlow::ActionResult::success();
}
```

---

## 3. Priority

`gmRules` has high priority because it becomes the shared grammar for:

```text
cards
abilities
items
status effects
scenario events
traps
objectives
reactions
instant effects
AI behavior cards
```

Without `gmRules`, every concrete action would reimplement target selection, condition checking, effect resolution, and status logic independently.

---

## 4. Scope

### 4.1 Must implement

`gmRules` must include internal modules for:

```text
Target
Condition
Effect
Status
```

The library should expose a façade named:

```cpp
gmRules::gmRulesEngine
```

### 4.2 Must not implement

Do not implement:

```text
turn order
round system
timeline system
scenario loading
campaign progression
GUI integration
networking
AI decision making
text parser
Lua/Python scripting
full expression language
undo/redo
event sourcing
```

Those responsibilities belong to other libraries or game-specific code.

---

## 5. Dependency Rules

### 5.1 Allowed dependencies

The library may depend on:

```text
C++17 standard library
gmActor
gmMap
gmDeck / gmCompDeck
gmDispatch, optional
gmLog, optional
gmSave compatibility through to_json/from_json patterns
```

### 5.2 Forbidden dependencies

The library must not depend on:

```text
gmFlow
scenario-specific game code
campaign-specific code
GUI frameworks
Python bindings
Qt
```

### 5.3 Dependency direction

Correct dependency direction:

```text
gmFlow action.execute()
    ↓
gmRules
    ↓
gmActor / gmMap / gmDeck / gmCompDeck
```

Incorrect dependency direction:

```text
gmRules
    ↓
gmFlow
```

`gmRules` must not include headers from `gmFlow`.

---

## 6. Naming Conventions

Follow the conventions used by existing `gmXxx` libraries.

| Element | Convention | Example |
| ------- | ---------- | ------- |
| Namespace | `gmRules` | `gmRules::EffectSpec` |
| Façade class | `gm` + PascalCase | `gmRulesEngine` |
| Interface | `I` + PascalCase | `IRuleContext` |
| Supporting class | PascalCase | `TargetResolver` |
| Enums | PascalCase enum class | `EffectType` |
| Enum values | SCREAMING_SNAKE_CASE | `DEAL_DAMAGE` |
| Private members | snake_case + `_` | `context_` |
| Methods | snake_case | `resolve_effect()` |
| Include guards | `GMRULES_CLASSNAME_HPP` | `GMRULES_EFFECTSPEC_HPP` |

Avoid redundant class names such as:

```cpp
gmRules::GmCondition
```

Prefer:

```cpp
gmRules::ConditionSpec
gmRules::ConditionEvaluator
gmRules::EffectResolver
gmRules::StatusEngine
```

---

## 7. Proposed File Structure

```text
gmRules/
├── PLAN.md
├── gmRules_API.md
├── ai-instructions.md
│
├── core/
│   ├── Ids.hpp
│   ├── RuleError.hpp
│   ├── RuleResult.hpp
│   ├── RuleContext.hpp
│   ├── RuleEvent.hpp
│   └── RuleTypes.hpp
│
├── target/
│   ├── TargetSpec.hpp
│   ├── TargetRef.hpp
│   ├── TargetSelector.hpp
│   ├── TargetFilter.hpp
│   ├── TargetResolver.hpp
│   └── TargetResult.hpp
│
├── condition/
│   ├── ConditionSpec.hpp
│   ├── ConditionType.hpp
│   ├── ConditionEvaluator.hpp
│   ├── CompositeCondition.hpp
│   └── TriggerSpec.hpp
│
├── effect/
│   ├── EffectSpec.hpp
│   ├── EffectType.hpp
│   ├── EffectResolver.hpp
│   ├── EffectResult.hpp
│   └── EffectRegistry.hpp
│
├── status/
│   ├── StatusDefinition.hpp
│   ├── StatusInstance.hpp
│   ├── Modifier.hpp
│   ├── Duration.hpp
│   ├── StatusEngine.hpp
│   └── StackingPolicy.hpp
│
├── facade/
│   └── gmRulesEngine.hpp
│
└── tests/
    ├── test_target_resolver.cpp
    ├── test_condition_evaluator.cpp
    ├── test_effect_resolver.cpp
    ├── test_status_engine.cpp
    └── test_rules_integration.cpp
```

---

## 8. Core Module

### 8.1 Ids.hpp

Use string identifiers for serializability and debug readability.

```cpp
namespace gmRules {

using RuleId         = std::string;
using ActorId        = std::string;
using LocationId     = std::string;
using CardId         = std::string;
using DeckId         = std::string;
using ItemId         = std::string;
using StatusId       = std::string;
using StatusInstanceId = std::string;
using EffectId       = std::string;
using ConditionId    = std::string;
using EventType      = std::string;

} // namespace gmRules
```

Do not assume numeric IDs in public APIs.

---

### 8.2 RuleError.hpp

```cpp
namespace gmRules {

enum class RuleError {
    NONE,
    UNKNOWN_ACTOR,
    UNKNOWN_LOCATION,
    UNKNOWN_CARD,
    UNKNOWN_DECK,
    UNKNOWN_ITEM,
    UNKNOWN_STATUS,
    INVALID_TARGET,
    CONDITION_FAILED,
    EFFECT_FAILED,
    UNSUPPORTED_EFFECT,
    UNSUPPORTED_CONDITION,
    RULE_VIOLATION,
    CONTEXT_ERROR,
    CUSTOM_ERROR
};

} // namespace gmRules
```

---

### 8.3 RuleResult.hpp

Generic result type for non-effect operations.

```cpp
namespace gmRules {

class RuleResult {
public:
    static RuleResult ok();
    static RuleResult fail(RuleError error, std::string message);
    static RuleResult warning(std::string message);

    bool valid() const;
    bool has_warning() const;

    RuleError error() const;
    const std::string& message() const;
};

} // namespace gmRules
```

---

### 8.4 RuleEvent.hpp

A lightweight event generated by rules.

```cpp
namespace gmRules {

struct RuleEvent {
    EventType type;
    std::string source_id;
    std::string target_id;
    std::string payload_json;
};

} // namespace gmRules
```

`payload_json` may be an empty string in V1. Avoid introducing a JSON dependency unless already used elsewhere in the project.

---

### 8.5 RuleContext.hpp

`RuleContext` is the adapter between `gmRules` and the actual game state.

It must be abstract. `gmRules` must not impose a concrete game state class.

```cpp
namespace gmRules {

class RuleContext {
public:
    virtual ~RuleContext() = default;

    // Actor access.
    virtual bool has_actor(const ActorId& actor_id) const = 0;
    virtual bool actor_has_tag(const ActorId& actor_id,
                               const std::string& tag) const = 0;
    virtual int actor_current_hp(const ActorId& actor_id) const = 0;
    virtual int actor_max_hp(const ActorId& actor_id) const = 0;
    virtual void modify_actor_hp(const ActorId& actor_id, int delta) = 0;

    // Actor status access.
    virtual bool actor_has_status(const ActorId& actor_id,
                                  const StatusId& status_id) const = 0;
    virtual void add_status_instance(const StatusInstance& status) = 0;
    virtual void remove_status_instance(const StatusInstanceId& instance_id) = 0;
    virtual std::vector<StatusInstanceId>
        statuses_on_actor(const ActorId& actor_id) const = 0;

    // Location / map access.
    virtual bool has_location(const LocationId& location_id) const = 0;
    virtual LocationId actor_location(const ActorId& actor_id) const = 0;
    virtual bool are_locations_adjacent(const LocationId& a,
                                        const LocationId& b) const = 0;
    virtual int distance_between_locations(const LocationId& a,
                                           const LocationId& b) const = 0;
    virtual bool location_has_tag(const LocationId& location_id,
                                  const std::string& tag) const = 0;
    virtual void move_actor_to_location(const ActorId& actor_id,
                                        const LocationId& location_id) = 0;

    // Faction / relation access.
    virtual bool are_allies(const ActorId& a, const ActorId& b) const = 0;
    virtual bool are_enemies(const ActorId& a, const ActorId& b) const = 0;

    // Deck / card access.
    virtual bool has_deck(const DeckId& deck_id) const = 0;
    virtual std::vector<CardId> draw_cards(const DeckId& deck_id,
                                           int amount) = 0;
    virtual RuleResult move_card_to_zone(const DeckId& deck_id,
                                         const CardId& card_id,
                                         const std::string& zone_name) = 0;

    // Events.
    virtual void emit_event(const RuleEvent& event) = 0;
};

} // namespace gmRules
```

Game-specific code may implement this interface by delegating to `gmActor`, `gmMap`, `gmCompDeck`, and its own state container.

---

## 9. Target Module

### 9.1 Purpose

The target module resolves and validates what a rule affects.

Examples:

```text
self
selected actor
selected enemy
selected ally
all actors in current location
all enemies in adjacent location
selected card
selected item
selected location
```

---

### 9.2 TargetSpec.hpp

```cpp
namespace gmRules {

enum class TargetKind {
    ACTOR,
    ACTOR_GROUP,
    LOCATION,
    CARD,
    DECK,
    ITEM,
    INTERACTABLE,
    NONE
};

enum class TargetSelector {
    SELF,
    SOURCE,
    SELECTED_ACTOR,
    SELECTED_ALLY,
    SELECTED_ENEMY,
    ALL_ACTORS_IN_LOCATION,
    ALL_ALLIES_IN_LOCATION,
    ALL_ENEMIES_IN_LOCATION,
    ACTORS_WITH_STATUS,
    LOCATION,
    SELECTED_CARD,
    SELECTED_ITEM,
    MANUAL
};

enum class RangeType {
    NONE,
    SAME_LOCATION,
    ADJACENT_LOCATION,
    WITHIN_N_LOCATIONS,
    ANY_VISIBLE_LOCATION,
    GLOBAL
};

struct TargetSpec {
    TargetKind kind = TargetKind::NONE;
    TargetSelector selector = TargetSelector::MANUAL;
    RangeType range_type = RangeType::NONE;
    int range_value = 0;

    std::vector<std::string> required_tags;
    std::vector<std::string> forbidden_tags;

    bool allow_self = true;
    bool required = true;
};

} // namespace gmRules
```

---

### 9.3 TargetRef.hpp

```cpp
namespace gmRules {

struct TargetRef {
    TargetKind kind = TargetKind::NONE;
    std::string id;
};

} // namespace gmRules
```

---

### 9.4 TargetResult.hpp

```cpp
namespace gmRules {

class TargetResult {
public:
    static TargetResult success(std::vector<TargetRef> targets);
    static TargetResult failure(std::string message);

    bool valid() const;
    const std::vector<TargetRef>& targets() const;
    const std::string& message() const;
};

} // namespace gmRules
```

---

### 9.5 TargetResolver.hpp

```cpp
namespace gmRules {

class TargetResolver {
public:
    TargetResult resolve(const TargetSpec& spec,
                         const ActorId& source_actor_id,
                         const std::vector<TargetRef>& selected_targets,
                         RuleContext& ctx) const;
};

} // namespace gmRules
```

`selected_targets` contains targets selected by the UI or by game-specific AI before calling `gmRules`.

Do not implement UI selection logic in `gmRules`.

---

## 10. Condition Module

### 10.1 Purpose

Conditions answer: “Is this rule currently true?”

They are used for:

```text
action validation
card play requirements
trigger requirements
status duration checks
objective checks
interaction requirements
```

---

### 10.2 ConditionType.hpp

```cpp
namespace gmRules {

enum class ConditionType {
    ALWAYS,
    NEVER,

    ACTOR_EXISTS,
    ACTOR_HAS_STATUS,
    ACTOR_HAS_TAG,
    ACTOR_HP_AT_OR_BELOW,
    ACTOR_HP_AT_OR_ABOVE,
    ACTOR_IN_LOCATION,
    ACTOR_IN_POSITION,

    TARGET_EXISTS,
    TARGET_HAS_STATUS,
    TARGET_HAS_TAG,

    LOCATION_EXISTS,
    LOCATION_HAS_TAG,
    LOCATION_IS_ADJACENT,

    DECK_HAS_AT_LEAST,
    CARD_IN_ZONE,

    RESOURCE_AT_LEAST,

    CUSTOM
};

enum class CompositeOperator {
    ALL_OF,
    ANY_OF,
    NONE_OF,
    NOT
};

} // namespace gmRules
```

---

### 10.3 ConditionSpec.hpp

```cpp
namespace gmRules {

struct ConditionSpec {
    ConditionType type = ConditionType::ALWAYS;

    std::string subject_id;
    std::string target_id;
    std::string value;
    int amount = 0;

    std::vector<ConditionSpec> children;
    CompositeOperator op = CompositeOperator::ALL_OF;
};

} // namespace gmRules
```

If `children` is not empty, evaluate as a composite condition using `op`.

---

### 10.4 TriggerSpec.hpp

Triggers describe event-based activation requirements.

```cpp
namespace gmRules {

enum class TriggerType {
    ON_ACTION_SUBMITTED,
    ON_ACTION_COMPLETED,
    ON_CARD_PLAYED,
    ON_ACTOR_DAMAGED,
    ON_ACTOR_MOVED,
    ON_STATUS_APPLIED,
    ON_TIME_REACHED,
    ON_LOCATION_ENTERED,
    CUSTOM
};

struct TriggerSpec {
    TriggerType type = TriggerType::CUSTOM;
    std::vector<ConditionSpec> conditions;
};

} // namespace gmRules
```

`TriggerSpec` must remain event-system agnostic. A game-specific adapter should translate `gmFlow`, `gmDispatch`, or custom events into trigger checks.

---

### 10.5 ConditionEvaluator.hpp

```cpp
namespace gmRules {

class ConditionEvaluator {
public:
    RuleResult evaluate(const ConditionSpec& condition,
                        RuleContext& ctx) const;

    RuleResult evaluate_all(const std::vector<ConditionSpec>& conditions,
                            RuleContext& ctx) const;
};

} // namespace gmRules
```

`evaluate()` must be side-effect-free.

---

## 11. Effect Module

### 11.1 Purpose

Effects answer: “What happens?”

Effects mutate game state through `RuleContext` and may emit `RuleEvent`s.

---

### 11.2 EffectType.hpp

```cpp
namespace gmRules {

enum class EffectType {
    DEAL_DAMAGE,
    HEAL,
    MOVE_ACTOR,
    SHIFT_POSITION,

    DRAW_CARDS,
    DISCARD_CARDS,
    MOVE_CARD_TO_ZONE,

    APPLY_STATUS,
    REMOVE_STATUS,
    ADD_MODIFIER,
    REMOVE_MODIFIER,

    ADD_TAG,
    REMOVE_TAG,

    SET_STATE,
    SPAWN_ACTOR,
    DESPAWN_ACTOR,

    EMIT_EVENT,
    MANUAL_EFFECT,
    CUSTOM
};

} // namespace gmRules
```

---

### 11.3 EffectSpec.hpp

```cpp
namespace gmRules {

struct EffectSpec {
    EffectType type = EffectType::CUSTOM;

    std::string source_id;
    TargetSpec target;

    int amount = 0;
    std::string value;

    std::vector<ConditionSpec> conditions;

    bool optional = false;
    bool stop_on_failure = true;
};

} // namespace gmRules
```

`value` is intentionally generic. It may represent:

```text
status_id
zone_name
tag name
state value
custom payload
```

---

### 11.4 EffectResult.hpp

```cpp
namespace gmRules {

class EffectResult {
public:
    static EffectResult success();
    static EffectResult partial(std::vector<std::string> warnings);
    static EffectResult failure(std::string message);

    bool succeeded() const;
    bool partial_success() const;

    const std::vector<RuleEvent>& events() const;
    const std::vector<std::string>& warnings() const;
    const std::string& message() const;

    void add_event(RuleEvent event);
    void add_warning(std::string warning);
};

} // namespace gmRules
```

---

### 11.5 EffectResolver.hpp

```cpp
namespace gmRules {

class EffectResolver {
public:
    EffectResult resolve(const EffectSpec& effect,
                         const ActorId& source_actor_id,
                         const std::vector<TargetRef>& selected_targets,
                         RuleContext& ctx);

    EffectResult resolve_many(const std::vector<EffectSpec>& effects,
                              const ActorId& source_actor_id,
                              const std::vector<TargetRef>& selected_targets,
                              RuleContext& ctx);
};

} // namespace gmRules
```

Resolution order:

```text
1. Evaluate effect conditions.
2. Resolve targets.
3. Apply effect through RuleContext.
4. Emit RuleEvents.
5. Return EffectResult.
```

If an effect fails and `stop_on_failure == true`, `resolve_many()` must stop and return failure.

If `optional == true`, failure should become a warning unless it is a structural/context error.

---

## 12. Status Module

### 12.1 Purpose

Statuses are persistent rule objects attached to actors or other entities.

They may contain:

```text
on_apply effects
on_remove effects
on_activation_start effects
on_activation_end effects
continuous modifiers
duration
stacking policy
```

---

### 12.2 Duration.hpp

```cpp
namespace gmRules {

enum class DurationType {
    PERMANENT,
    UNTIL_REMOVED,
    FOR_N_ACTIVATIONS,
    UNTIL_NEXT_ACTIVATION,
    UNTIL_TIME_REACHED,
    WHILE_IN_LOCATION,
    WHILE_CONDITION_TRUE,
    CUSTOM
};

struct DurationSpec {
    DurationType type = DurationType::UNTIL_REMOVED;
    int amount = 0;
    std::string value;
    std::vector<ConditionSpec> conditions;
};

struct DurationState {
    DurationSpec spec;
    int remaining = 0;
    bool expired = false;
};

} // namespace gmRules
```

Do not assume games use rounds. Use activation/time/condition-based durations.

---

### 12.3 StackingPolicy.hpp

```cpp
namespace gmRules {

enum class StackingMode {
    REFRESH_DURATION,
    ADD_STACK,
    IGNORE_NEW,
    REPLACE,
    UNIQUE_BY_SOURCE
};

struct StackingPolicy {
    StackingMode mode = StackingMode::REFRESH_DURATION;
    int max_stacks = 1;
};

} // namespace gmRules
```

---

### 12.4 Modifier.hpp

Modifiers represent persistent changes to stats or rule values.

```cpp
namespace gmRules {

enum class ModifierOp {
    ADD,
    SUBTRACT,
    MULTIPLY,
    SET,
    MIN,
    MAX
};

struct Modifier {
    std::string id;
    std::string stat_id;
    ModifierOp op = ModifierOp::ADD;
    int amount = 0;
    std::vector<ConditionSpec> conditions;
};

} // namespace gmRules
```

`gmRules` should define modifier data, but actual stat recalculation may be delegated to `gmActor` or game-specific code.

---

### 12.5 StatusDefinition.hpp

```cpp
namespace gmRules {

struct StatusDefinition {
    StatusId id;
    std::string name;
    std::vector<std::string> tags;

    std::vector<EffectSpec> on_apply;
    std::vector<EffectSpec> on_remove;
    std::vector<EffectSpec> on_activation_start;
    std::vector<EffectSpec> on_activation_end;
    std::vector<EffectSpec> continuous_effects;

    std::vector<Modifier> modifiers;

    StackingPolicy stacking_policy;
    DurationSpec default_duration;
};

} // namespace gmRules
```

---

### 12.6 StatusInstance.hpp

```cpp
namespace gmRules {

struct StatusInstance {
    StatusInstanceId instance_id;
    StatusId status_id;
    ActorId owner_actor_id;
    std::string source_id;

    int stacks = 1;
    DurationState duration;
};

} // namespace gmRules
```

---

### 12.7 StatusEngine.hpp

```cpp
namespace gmRules {

class StatusEngine {
public:
    RuleResult apply_status(const StatusDefinition& def,
                            const ActorId& owner_actor_id,
                            const std::string& source_id,
                            RuleContext& ctx);

    RuleResult remove_status(const StatusInstanceId& status_instance_id,
                             RuleContext& ctx);

    RuleResult on_activation_start(const ActorId& actor_id,
                                   RuleContext& ctx);

    RuleResult on_activation_end(const ActorId& actor_id,
                                 RuleContext& ctx);

    RuleResult update_durations(RuleContext& ctx);
};

} // namespace gmRules
```

`StatusEngine` may internally use `EffectResolver` for status effects.

---

## 13. Facade Module

### 13.1 gmRulesEngine.hpp

Provide a simple façade for game-specific code.

```cpp
namespace gmRules {

class gmRulesEngine {
public:
    TargetResult resolve_target(const TargetSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                                RuleContext& ctx);

    RuleResult evaluate_condition(const ConditionSpec& spec,
                                  RuleContext& ctx);

    RuleResult evaluate_conditions(const std::vector<ConditionSpec>& specs,
                                   RuleContext& ctx);

    EffectResult resolve_effect(const EffectSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                                RuleContext& ctx);

    EffectResult resolve_effects(const std::vector<EffectSpec>& specs,
                                 const ActorId& source_actor_id,
                                 const std::vector<TargetRef>& selected_targets,
                                 RuleContext& ctx);

    RuleResult apply_status(const StatusDefinition& status,
                            const ActorId& owner_actor_id,
                            const std::string& source_id,
                            RuleContext& ctx);
};

} // namespace gmRules
```

The façade should compose:

```text
TargetResolver
ConditionEvaluator
EffectResolver
StatusEngine
```

Do not put all logic directly in the façade.

---

## 14. Serialization Requirements

The library must be compatible with `gmSave` conventions.

Provide `to_json` / `from_json` free functions where appropriate, following the patterns already used in existing libraries.

Types to serialize:

```text
TargetSpec
TargetRef
ConditionSpec
TriggerSpec
EffectSpec
StatusDefinition
StatusInstance
DurationSpec
DurationState
StackingPolicy
Modifier
```

Do not serialize `RuleContext`, `TargetResolver`, `ConditionEvaluator`, `EffectResolver`, `StatusEngine`, or `gmRulesEngine`.

---

## 15. Manual Effect

`EffectType::MANUAL_EFFECT` must exist.

It is required as an escape hatch for effects that cannot yet be automated.

Behavior:

```text
- It does not mutate state automatically.
- It emits a RuleEvent describing the manual effect.
- It returns success unless required fields are malformed.
```

This allows early game-design testing without implementing every complex effect immediately.

---

## 16. Minimal V1 Feature Set

The first implementation should prioritize the following subset.

### 16.1 Target V1

```text
SELF
SELECTED_ACTOR
SELECTED_ALLY
SELECTED_ENEMY
ALL_ALLIES_IN_LOCATION
ALL_ENEMIES_IN_LOCATION
LOCATION
```

### 16.2 Condition V1

```text
ALWAYS
ACTOR_EXISTS
ACTOR_HAS_STATUS
ACTOR_HAS_TAG
ACTOR_HP_AT_OR_BELOW
ACTOR_IN_LOCATION
LOCATION_HAS_TAG
```

### 16.3 Effect V1

```text
DEAL_DAMAGE
HEAL
MOVE_ACTOR
DRAW_CARDS
DISCARD_CARDS
MOVE_CARD_TO_ZONE
APPLY_STATUS
REMOVE_STATUS
MANUAL_EFFECT
```

### 16.4 Status V1

```text
StatusDefinition
StatusInstance
DurationSpec
DurationState
StackingPolicy
on_apply
on_remove
on_activation_start
on_activation_end
```

Implement V1 completely before expanding.

---

## 17. Implementation Phases

Follow the development approach used by `gmFlow` and other `gmXxx` libraries.

### Phase 1 — Planning

Deliverables:

```text
- PLAN.md approved
- file tree approved
- dependency direction approved
- V1 subset approved
```

### Phase 2 — Headers, interfaces, signatures, stubs

Deliverables:

```text
- all public headers created
- all classes and methods declared
- stub .cpp bodies compile
- no business logic yet
- Doxygen comments on public API
```

Do not implement heavy logic in Phase 2.

### Phase 3 — API documentation

Deliverables:

```text
- gmRules_API.md
- architecture overview
- examples
- all public types documented
- integration section with gmActor/gmMap/gmDeck/gmFlow
```

### Phase 4 — Implementation

Recommended order:

```text
4.1 Core result/context/event types
4.2 TargetResolver
4.3 ConditionEvaluator
4.4 EffectResolver
4.5 StatusEngine
4.6 gmRulesEngine façade
4.7 Serialization helpers
```

### Phase 5 — Tests

Add tests after each implementation step.

Test files:

```text
tests/test_target_resolver.cpp
tests/test_condition_evaluator.cpp
tests/test_effect_resolver.cpp
tests/test_status_engine.cpp
tests/test_rules_integration.cpp
```

---

## 18. Required Tests

### 18.1 TargetResolver tests

```text
- SELF resolves to source actor
- SELECTED_ACTOR validates selected target exists
- SELECTED_ALLY rejects enemy target
- SELECTED_ENEMY rejects ally target
- ALL_ALLIES_IN_LOCATION returns correct actors
- ALL_ENEMIES_IN_LOCATION returns correct actors
- range SAME_LOCATION works
- range ADJACENT_LOCATION works
- required_tags filters targets
- forbidden_tags filters targets
```

### 18.2 ConditionEvaluator tests

```text
- ALWAYS passes
- NEVER fails
- ACTOR_EXISTS passes/fails correctly
- ACTOR_HAS_STATUS passes/fails correctly
- ACTOR_HP_AT_OR_BELOW passes/fails correctly
- ACTOR_IN_LOCATION passes/fails correctly
- LOCATION_HAS_TAG passes/fails correctly
- ALL_OF composite works
- ANY_OF composite works
- NOT composite works
```

### 18.3 EffectResolver tests

```text
- DEAL_DAMAGE reduces HP
- HEAL increases HP but does not exceed max if context enforces max
- MOVE_ACTOR changes location
- DRAW_CARDS delegates to context
- MOVE_CARD_TO_ZONE delegates to context
- APPLY_STATUS creates status instance
- REMOVE_STATUS removes status instance
- MANUAL_EFFECT emits event and does not mutate state
- stop_on_failure stops effect chain
- optional failure becomes warning
```

### 18.4 StatusEngine tests

```text
- apply_status adds a StatusInstance
- REFRESH_DURATION refreshes existing status
- ADD_STACK increments stacks up to max_stacks
- IGNORE_NEW ignores duplicate
- REPLACE removes old and applies new
- on_apply effects are resolved
- on_remove effects are resolved
- on_activation_start effects are resolved
- on_activation_end effects are resolved
- duration UNTIL_NEXT_ACTIVATION expires correctly
- duration FOR_N_ACTIVATIONS decrements correctly
```

### 18.5 Integration tests

```text
- card-like effect with target + condition + damage
- status applies damage on activation start
- move effect validates location via map adapter
- draw effect validates deck adapter
- manual effect produces event
```

---

## 19. Integration Example

A game-specific card definition may contain:

```cpp
std::vector<gmRules::EffectSpec> effects;
```

Example effect:

```cpp
gmRules::EffectSpec effect;
effect.type = gmRules::EffectType::DEAL_DAMAGE;
effect.amount = 2;
effect.target.kind = gmRules::TargetKind::ACTOR;
effect.target.selector = gmRules::TargetSelector::SELECTED_ENEMY;
effect.target.range_type = gmRules::RangeType::SAME_LOCATION;
```

A concrete action resolves it:

```cpp
gmRules::gmRulesEngine rules;
auto result = rules.resolve_effects(card.effects,
                                    source_actor_id,
                                    selected_targets,
                                    rule_context);
```

---

## 20. Architectural Invariants

The following must always remain true:

1. `gmRules` does not know turn order.
2. `gmRules` does not include `gmFlow` headers.
3. `ConditionEvaluator` is side-effect-free.
4. `TargetResolver` is side-effect-free.
5. `EffectResolver` is the main mutation point.
6. `StatusEngine` mutates only through `RuleContext`.
7. `RuleContext` is abstract.
8. All public references use string IDs.
9. Manual effects are supported.
10. No UI code is allowed.
11. No game-specific names or hardcoded setting details are allowed.
12. Serialization must be compatible with the existing `gmSave` style.

---

## 21. Suggested Build Commands

Adapt paths to the repository root.

```powershell
# Phase 2: compile stubs
clang++ -std=c++17 -I. ^
    gmRules/target/*.cpp ^
    gmRules/condition/*.cpp ^
    gmRules/effect/*.cpp ^
    gmRules/status/*.cpp ^
    gmRules/tests/test_target_resolver.cpp ^
    -o test_gmRules_stub.exe

# Phase 4/5: compile all gmRules tests
clang++ -std=c++17 -I. ^
    gmRules/target/*.cpp ^
    gmRules/condition/*.cpp ^
    gmRules/effect/*.cpp ^
    gmRules/status/*.cpp ^
    gmRules/tests/test_target_resolver.cpp ^
    gmRules/tests/test_condition_evaluator.cpp ^
    gmRules/tests/test_effect_resolver.cpp ^
    gmRules/tests/test_status_engine.cpp ^
    gmRules/tests/test_rules_integration.cpp ^
    -o test_gmRules.exe ; ./test_gmRules.exe
```

---

## 22. Final Implementation Guidance

Build `gmRules` as a practical primitive library, not as a complete rule language.

Do not over-engineer V1.

Start with:

```text
TargetSpec
ConditionSpec
EffectSpec
StatusDefinition
StatusInstance
RuleContext
TargetResolver
ConditionEvaluator
EffectResolver
StatusEngine
gmRulesEngine
```

Then add only what tests and real game integration require.

The result should be a generic rule toolkit that can be reused by many tabletop game engines while staying simple enough to debug.
