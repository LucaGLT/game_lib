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
