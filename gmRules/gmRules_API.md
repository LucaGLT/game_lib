# gmRules API

## Overview

`gmRules` is a C++17 rule toolkit for turn-based and card-driven games.
It provides four core blocks:

- Target resolution (`TargetSpec`, `TargetResolver`)
- Condition evaluation (`ConditionSpec`, `ConditionEvaluator`)
- Effect application (`EffectSpec`, `EffectResolver`)
- Status lifecycle (`StatusDefinition`, `StatusEngine`)

Public facade:

- `gmRules::gmRulesEngine`

Design constraints:

- C++17 only
- No dependency on `gmFlow`
- State access only through `RuleContext`

## Include

```cpp
#include "gmRules/facade/gmRulesEngine.hpp"
```

## Namespace and IDs

All APIs are in namespace `gmRules`.

IDs are aliases of `std::string` in `gmRules/core/Ids.hpp`:

- `RuleId`, `ActorId`, `LocationId`, `CardId`, `DeckId`
- `ItemId`, `StatusId`, `StatusInstanceId`
- `EffectId`, `ConditionId`, `EventType`, `SourceId`

## Error Model

### Runtime results

Non-exception runtime outcomes use:

- `RuleResult` for conditions/status/single checks
- `TargetResult` for target selection
- `EffectResult` for effect pipelines

`RuleError` enum (`gmRules/core/RuleError.hpp`) includes:

- `UNKNOWN_ACTOR`, `UNKNOWN_LOCATION`, `UNKNOWN_DECK`
- `INVALID_TARGET`, `CONDITION_FAILED`, `EFFECT_FAILED`
- `UNSUPPORTED_EFFECT`, `UNSUPPORTED_CONDITION`
- `RULE_VIOLATION`, `CONTEXT_ERROR`, `CUSTOM_ERROR`

### Exceptions

Programming-level structural errors can throw:

- `ERulesError`
- `ETargetError`
- `EConditionError`
- `EEffectError`
- `EStatusError`

Defined in `gmRules/core/RuleTypes.hpp`.

## Core Adapter: RuleContext

`RuleContext` (`gmRules/core/RuleContext.hpp`) is the only bridge to game state.

Const-side queries include:

- Actor checks (`has_actor`, tags, hp, statuses, ally/enemy relation)
- Location checks (`has_location`, adjacency, distance, location tags)
- Actor collection in location (`actors_in_location`)
- Deck existence (`has_deck`)

Mutating operations include:

- HP and tag mutation (`modify_actor_hp`, add/remove tag)
- Status mutation (`add_status_instance`, `remove_status_instance`)
- Movement (`move_actor_to_location`)
- Deck operations (`draw_cards`, `move_card_to_zone`)
- Event emission (`emit_event`)

## Target API

Files:

- `gmRules/target/TargetSpec.hpp`
- `gmRules/target/TargetRef.hpp`
- `gmRules/target/TargetResult.hpp`
- `gmRules/target/TargetResolver.hpp`

### Key types

- `TargetKind`: actor/location/card/deck/item/interactable domains
- `TargetSelector`: `SELF`, `SELECTED_ACTOR`, `ALL_ALLIES_IN_LOCATION`, ...
- `RangeType`: `SAME_LOCATION`, `ADJACENT_LOCATION`, `WITHIN_N_LOCATIONS`, ...
- `TargetSpec`: selector + filters + optional/required behavior
- `TargetRef`: resolved pair (`kind`, `id`)

### Resolver

```cpp
TargetResult resolve(const TargetSpec& spec,
                     const ActorId& source_actor_id,
                     const std::vector<TargetRef>& selected_targets,
                     const RuleContext& ctx) const;
```

Contract:

- Side-effect free
- Applies selector, range, tag filters, self filter
- Fails if target is required and none is resolved

## Condition API

Files:

- `gmRules/condition/ConditionType.hpp`
- `gmRules/condition/ConditionSpec.hpp`
- `gmRules/condition/TriggerSpec.hpp`
- `gmRules/condition/ConditionEvaluator.hpp`

### Key types

- `ConditionType`: atomic predicates (`ALWAYS`, `ACTOR_EXISTS`, `LOCATION_HAS_TAG`, ...)
- `CompositeOperator`: `ALL_OF`, `ANY_OF`, `NONE_OF`, `NOT`
- `ConditionSpec`: atomic fields (`subject_id`, `target_id`, `value`, `amount`) + `children`
- `TriggerSpec`: trigger event + extra condition list for reactive systems

### Evaluator

```cpp
RuleResult evaluate(const ConditionSpec& condition,
                    const RuleContext& ctx) const;

RuleResult evaluate_all(const std::vector<ConditionSpec>& conditions,
                        const RuleContext& ctx) const;
```

Contract:

- Side-effect free
- `evaluate_all` is implicit logical AND

## Effect API

Files:

- `gmRules/effect/EffectType.hpp`
- `gmRules/effect/EffectSpec.hpp`
- `gmRules/effect/EffectResult.hpp`
- `gmRules/effect/EffectResolver.hpp`

### Key types

- `EffectType`: `DEAL_DAMAGE`, `HEAL`, `MOVE_ACTOR`, `DRAW_CARDS`,
  `APPLY_STATUS`, `REMOVE_STATUS`, `MANUAL_EFFECT`, ...
- `EffectSpec`: effect payload + target + preconditions + failure policy
- `EffectResult`: success/partial/failure, events, warnings

### EffectType — reference table

All 41 effect types implemented in `EffectResolver`.
For each type, the relevant `EffectSpec` fields are listed.

| EffectType | `amount` | `value` | Notes |
|---|---|---|---|
| **Health / Position** ||||
| `DEAL_DAMAGE` | damage points (> 0) | — | Calls `modify_actor_hp(-amount)` |
| `HEAL` | HP restored (> 0) | — | Calls `modify_actor_hp(+amount)` |
| `MOVE_ACTOR` | — | destination location ID | Calls `move_actor_to_location()` |
| `SHIFT_POSITION` | — | `"front"` / `"back"` | Area position within a location |
| **Card management** ||||
| `DRAW_CARDS` | number of cards | deck ID | Calls `draw_cards(value, amount)` |
| `DISCARD_CARDS` | number of cards | — | Emits event only; game-specific logic |
| `MOVE_CARD_TO_ZONE` | — | `"card_id:zone_name"` | Calls `move_card_to_zone()` |
| **Status / Modifiers** ||||
| `APPLY_STATUS` | — | status definition ID | Calls `add_status_instance()` |
| `REMOVE_STATUS` | — | status ID to remove | Calls `remove_status_instance()` |
| `ADD_MODIFIER` | — | modifier spec JSON | Game-specific delegation |
| `REMOVE_MODIFIER` | — | modifier instance ID | Game-specific delegation |
| **Tags / State** ||||
| `ADD_TAG` | — | tag string | Calls `add_actor_tag()` |
| `REMOVE_TAG` | — | tag string | Calls `remove_actor_tag()` |
| `SET_STATE` | — | `"key=value"` | Opaque; game-specific |
| **Actor lifecycle** ||||
| `SPAWN_ACTOR` | — | actor spec JSON | Calls `spawn_actor()` |
| `DESPAWN_ACTOR` | — | — | Calls `despawn_actor()` |
| `REVIVE_ACTOR` | — | — | Calls `revive_actor()` |
| `CHANGE_TEAM` | — | new team/faction ID | Calls `change_actor_team()` |
| **Resources / Equipment** ||||
| `MODIFY_RESOURCE` | signed delta | resource name | Calls `modify_resource()` |
| `SET_RESOURCE_MAX` | new max value | resource name | Calls `set_resource_max()` |
| `EQUIP_ITEM` | — | item ID | Calls `equip_item()` |
| `UNEQUIP_ITEM` | — | slot ID | Calls `unequip_item()` |
| **Deck / Dice** ||||
| `SHUFFLE_ZONE` | — | `"deck_id:zone_name"` | Calls `shuffle_zone()` |
| `LOOK_TOP_CARD` | N cards to peek | deck ID | Calls `look_top_cards()` |
| `LOOK_BOTTOM_CARD` | N cards to peek | deck ID | Calls `look_bottom_cards()` |
| `SELECT_SPECIFIC_CARD` | — | `"deck_id:card_id"` | Calls `select_specific_card()` |
| `DISCARD_RANDOM` | N cards | `"deck_id:zone_name"` | Calls `discard_random_cards()` |
| `PLACE_ON_TOP` | — | `"deck_id:card_id"` | Calls `place_card_on_top()` |
| `PLACE_ON_BOTTOM` | — | `"deck_id:card_id"` | Calls `place_card_on_bottom()` |
| `ROLL_DICE` | — | dice expression (e.g. `"2d6"`) | Calls `roll_dice()` |
| **Map** ||||
| `SET_LOCATION_PASSABLE` | `1`=passable / `0`=blocked | location ID | Calls `set_location_passable()` |
| `ADD_LOCATION_TAG` | — | `"location_id:tag"` | Calls `add_location_tag()` |
| `REMOVE_LOCATION_TAG` | — | `"location_id:tag"` | Calls `remove_location_tag()` |
| `SET_LOCATION_OWNER` | — | `"location_id:owner_id"` | Calls `set_location_owner()` |
| `CREATE_BARRIER` | — | `"from:to:barrier_id"` | Calls `create_barrier()` |
| `REMOVE_BARRIER` | — | barrier ID | Calls `remove_barrier()` |
| `SPAWN_INTERACTABLE` | — | interactable spec JSON | Calls `spawn_interactable()` |
| `DESPAWN_INTERACTABLE` | — | interactable ID | Calls `despawn_interactable()` |
| **Events / Escape hatch** ||||
| `EMIT_EVENT` | — | event type string | Calls `emit_event()` without state mutation |
| `MANUAL_EFFECT` | — | event type string | Identical to `EMIT_EVENT` |
| `CUSTOM` | game-defined | game-defined | Delegated to `apply_extended_effect()` |

### Resolver

```cpp
EffectResult resolve(const EffectSpec& effect,
                     const ActorId& source_actor_id,
                     const std::vector<TargetRef>& selected_targets,
                     RuleContext& ctx) const;

EffectResult resolve_many(const std::vector<EffectSpec>& effects,
                          const ActorId& source_actor_id,
                          const std::vector<TargetRef>& selected_targets,
                          RuleContext& ctx) const;
```

Contract:

- Primary mutation point in `gmRules`
- Evaluates preconditions before applying each effect
- Optional failures become warnings
- With `stop_on_failure = true`, first hard failure stops pipeline

## Status API

Files:

- `gmRules/status/Duration.hpp`
- `gmRules/status/StackingPolicy.hpp`
- `gmRules/status/Modifier.hpp`
- `gmRules/status/StatusDefinition.hpp`
- `gmRules/status/StatusInstance.hpp`
- `gmRules/status/StatusEngine.hpp`

### Key types

- `DurationType`, `DurationSpec`, `DurationState`
- `StackingMode`, `StackingPolicy`
- `Modifier` (`stat_id`, operation, amount, optional conditions)
- `StatusDefinition` immutable schema
- `StatusInstance` mutable runtime application

### Engine

```cpp
RuleResult apply_status(const StatusDefinition& def,
                        const ActorId& owner_actor_id,
                        const std::string& source_id,
                        RuleContext& ctx);

RuleResult remove_status(const StatusInstanceId& status_instance_id,
                         const ActorId& owner_actor_id,
                         const StatusDefinition& def,
                         RuleContext& ctx);

RuleResult on_activation_start(const ActorId& actor_id,
                               const std::vector<StatusInstance>& statuses,
                               const std::vector<StatusDefinition>& defs,
                               RuleContext& ctx);

RuleResult on_activation_end(const ActorId& actor_id,
                             const std::vector<StatusInstance>& statuses,
                             const std::vector<StatusDefinition>& defs,
                             RuleContext& ctx);
```

Contract:

- Uses `EffectResolver` for lifecycle hooks (`on_apply`, `on_remove`, etc.)
- Stacking and duration policies are enforced through context mutations

## Facade: gmRulesEngine

File:

- `gmRules/facade/gmRulesEngine.hpp`

Main methods:

- `resolve_target(...)`
- `evaluate_condition(...)`
- `evaluate_conditions(...)`
- `resolve_effect(...)`
- `resolve_effects(...)`
- `apply_status(...)`

`gmRulesEngine` composes:

- `TargetResolver`
- `ConditionEvaluator`
- `EffectResolver`
- `StatusEngine`

## Minimal Usage

```cpp
#include "gmRules/facade/gmRulesEngine.hpp"

gmRules::gmRulesEngine engine;

gmRules::EffectSpec effect;
effect.type = gmRules::EffectType::DEAL_DAMAGE;
effect.amount = 2;
effect.target.selector = gmRules::TargetSelector::SELECTED_ENEMY;

std::vector<gmRules::TargetRef> selected;
gmRules::TargetRef selected_target;
selected_target.kind = gmRules::TargetKind::ACTOR;
selected_target.id = "enemy_01";
selected.push_back(selected_target);

// MyRuleContext must implement gmRules::RuleContext.
// MyRuleContext ctx = ...
// gmRules::EffectResult result = engine.resolve_effect(effect, "hero_01", selected, ctx);
```

## Build Integration

`gmRules` provides a static library target in `gmRules/CMakeLists.txt`:

- `gmRules`

and test targets:

- `test_gmRules_target_resolver`
- `test_gmRules_condition_evaluator`
- `test_gmRules_effect_resolver`
- `test_gmRules_status_engine`
- `test_gmRules_integration`

To include from root CMake, enable:

```cmake
add_subdirectory(gmRules)
```
