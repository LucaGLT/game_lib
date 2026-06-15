# gmRules — Development Plan

**Version:** 0.5.0  
**Status:** Phase 5 — Tests ✅  
**Language:** C++17 Standard  
**Namespace:** `gmRules`

---

## Goal

Generic, reusable C++17 library providing rule primitives for tabletop game engines:

```text
Target    → who or what a rule affects
Condition → whether a rule can happen or trigger
Effect    → what a rule does
Status    → persistent effects, modifiers, durations, stacking
```

`gmRules` is not a full game engine. It provides the shared grammar usable
by cards, abilities, items, status effects, scenario events, traps,
objectives, reactions, and AI behaviour cards.

---

## Architecture

```
┌────────────────────────────────────────────────────────────────┐
│ Game-specific code                                             │
│ PlayCardAction, MoveAction, ScenarioEvent, TrapInteraction     │
└──────────────────────┬─────────────────────────────────────────┘
                       │ uses
┌──────────────────────▼─────────────────────────────────────────┐
│ gmRules                                                        │
│ TargetSpec · ConditionSpec · EffectSpec · StatusEngine         │
└──────────────────────┬─────────────────────────────────────────┘
                       │ accesses through adapters/context
┌──────────────────────▼─────────────────────────────────────────┐
│ gmActor · gmMap · gmDeck · gmCompDeck · gmDispatch · gmLog     │
└────────────────────────────────────────────────────────────────┘
```

`gmRules` must **not** include headers from `gmFlow`.

---

## Design Decisions (locked)

| # | Decision | Choice |
|---|----------|--------|
| D1 | `RuleContext` | Abstract interface — game code implements it via gmActor/gmMap/gmDeck. |
| D2 | IDs | All references are `std::string` aliases. No integer-only IDs. |
| D3 | `ConditionEvaluator` | Side-effect-free. Never mutates state. |
| D4 | `TargetResolver` | Side-effect-free. |
| D5 | `EffectResolver` | Primary mutation point. Operates through `RuleContext`. |
| D6 | `MANUAL_EFFECT` | Emits event only; does not mutate state. |
| D7 | Serialization | Free functions `to_json`/`from_json` via gmSave/json.hpp. |
| D8 | Private members | `_` suffix convention (project rule). |
| D9 | Exception class prefix | `E` prefix: `ETargetError`, `EConditionError`, `EEffectError`. |
| D10 | `stop_on_failure` | Default `true` in `EffectSpec`. Optional effects become warnings. |

---

## File Structure

```
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
│   ├── TargetResult.hpp
│   ├── TargetResolver.hpp
│   └── TargetResolver.cpp
│
├── condition/
│   ├── ConditionType.hpp
│   ├── ConditionSpec.hpp
│   ├── TriggerSpec.hpp
│   ├── ConditionEvaluator.hpp
│   └── ConditionEvaluator.cpp
│
├── effect/
│   ├── EffectType.hpp
│   ├── EffectSpec.hpp
│   ├── EffectResult.hpp
│   ├── EffectResolver.hpp
│   └── EffectResolver.cpp
│
├── status/
│   ├── Duration.hpp
│   ├── StackingPolicy.hpp
│   ├── Modifier.hpp
│   ├── StatusDefinition.hpp
│   ├── StatusInstance.hpp
│   ├── StatusEngine.hpp
│   └── StatusEngine.cpp
│
├── facade/
│   └── gmRulesEngine.hpp
│
└── tests/
    ├── test_target_resolver.cpp
    ├── test_condition_evaluator.cpp
    ├── test_effect_resolver.cpp
    ├── test_status_engine.cpp
    ├── test_rules_integration.cpp
    └── run_all_gmRules_tests.ps1
```

Total: **28 files** (23 headers/sources + 5 tests + script).

---

## Integration Boundaries

| Library | gmRules use | Forbidden in gmRules |
|---------|-------------|----------------------|
| `gmActor` | HP, status, tags via `RuleContext` adapter | Direct include of gmActor headers in core |
| `gmMap` | Location, adjacency via `RuleContext` | Map graph internals |
| `gmDeck/gmCompDeck` | Card draw, zone move via `RuleContext` | Deck shuffle, zone lifecycle |
| `gmDispatch` | `RuleEvent` emission via `RuleContext` | Event bus creation |
| `gmSave` | `to_json`/`from_json` for specs | Save file management |
| `gmFlow` | **Forbidden** — no gmFlow headers in gmRules | Everything |

---

## Exceptions

```cpp
class ERulesError             : public std::runtime_error {};
class ETargetError            : public ERulesError {};
class EConditionError         : public ERulesError {};
class EEffectError            : public ERulesError {};
class EStatusError            : public ERulesError {};
```

---

## Development Phases

### Phase 1 — Planning ✅

- [x] Confirm file tree
- [x] Confirm design decisions (D1–D10)
- [x] Confirm integration boundaries
- [x] Confirm V1 feature subset

### Phase 2 — Headers and stubs ✅

- [x] `core/` — Ids, RuleError, RuleResult, RuleContext, RuleEvent, RuleTypes
- [x] `target/` — TargetSpec, TargetRef, TargetResult, TargetResolver
- [x] `condition/` — ConditionType, ConditionSpec, TriggerSpec, ConditionEvaluator
- [x] `effect/` — EffectType, EffectSpec, EffectResult, EffectResolver
- [x] `status/` — Duration, StackingPolicy, Modifier, StatusDefinition, StatusInstance, StatusEngine
- [x] `facade/` — gmRulesEngine
- [x] `CMakeLists.txt`

### Phase 3 — API documentation

- [ ] Write `gmRules_API.md`

### Phase 4 — Implementation ✅

1. [x] TargetResolver
2. [x] ConditionEvaluator
3. [x] EffectResolver
4. [x] StatusEngine
5. [x] gmRulesEngine façade (delegates to above)

### Phase 5 — Tests ✅

- [x] test_target_resolver.cpp
- [x] test_condition_evaluator.cpp
- [x] test_effect_resolver.cpp
- [x] test_status_engine.cpp
- [x] test_rules_integration.cpp

---

## V1 Feature Subset

### Target V1

```text
SELF, SELECTED_ACTOR, SELECTED_ALLY, SELECTED_ENEMY,
ALL_ALLIES_IN_LOCATION, ALL_ENEMIES_IN_LOCATION, LOCATION
```

### Condition V1

```text
ALWAYS, NEVER, ACTOR_EXISTS, ACTOR_HAS_STATUS, ACTOR_HAS_TAG,
ACTOR_HP_AT_OR_BELOW, ACTOR_IN_LOCATION, LOCATION_HAS_TAG,
ALL_OF / ANY_OF / NOT composite
```

### Effect V1

```text
DEAL_DAMAGE, HEAL, MOVE_ACTOR, DRAW_CARDS, DISCARD_CARDS,
MOVE_CARD_TO_ZONE, APPLY_STATUS, REMOVE_STATUS, MANUAL_EFFECT
```

### Status V1

```text
StatusDefinition, StatusInstance, DurationSpec, DurationState,
StackingPolicy, on_apply, on_remove,
on_activation_start, on_activation_end
```

---

## Build Commands (from game_lib root)

```powershell
# Core tests (no gmSave required)
clang++ -std=c++17 -I. `
    gmRules/target/TargetResolver.cpp `
    gmRules/condition/ConditionEvaluator.cpp `
    gmRules/effect/EffectResolver.cpp `
    gmRules/status/StatusEngine.cpp `
    gmRules/tests/test_target_resolver.cpp `
    -o bin/exe/test_gmRules_target.exe
```
