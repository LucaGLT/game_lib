# gmFlow — AI Coding Instructions

> Conventions specific to this library.  Must be read alongside the
> project-wide `style-rules.md` located at the workspace root.

---

## Library Purpose

`gmFlow` is a **game flow control framework** — it knows *who* can act, *when*,
and *in which context*.  It does **not** know what specific game actions mean.
Game-specific rules are plug-ins that implement the interfaces declared here.

---

## File Layout

```text
gmFlow/
├── core/        ← Ids, Result, GameContext, GameState
├── actions/     ← IAction, IActionStep, ActionQueue, ActionWindow, StepBasedAction
├── flow/        ← IPhase, IFlowController, Turn, Round, TurnPolicy, RoundPolicy
├── actors/      ← Actor, ActorRegistry
├── events/      ← IEvent, EventType, FlowEvents, EventBus
├── session/     ← GameSession, SessionConfig
├── campaign/    ← Campaign, CampaignState, SessionDefinition
└── tests/       ← one test file per major component
```

---

## Namespace

Always `gmFlow`.  Never nest namespaces inside it.

```cpp
namespace gmFlow {
// ...
} // namespace gmFlow
```

---

## Include Guards

Pattern: `GMFLOW_<UPPERCASE_FILENAME>_HPP`

Examples:
- `core/Ids.hpp`          → `GMFLOW_IDS_HPP`
- `actions/IAction.hpp`   → `GMFLOW_IACTION_HPP`
- `flow/IPhase.hpp`       → `GMFLOW_IPHASE_HPP`
- `session/GameSession.hpp` → `GMFLOW_GAMESESSION_HPP`

---

## Key Architectural Invariants

1. **Actions are atomic in V1.**  There is no `ActionStatus::SUSPENDED`.
   Session-level pause/resume is handled by `GameSession::pause()` which
   serialises the entire session snapshot via `gmSave`.

2. **`EventBus` wraps `GmDispatch::EventBusChannel`.**  Never implement a
   custom pub/sub mechanism.  The wrapper must forward all `publish()` calls
   to the underlying `GmDispatch::Dispatcher`.

3. **`CompletionPolicy::TIMEOUT_EXPIRED` does not exist in V1.**  Timer-based
   window expiry is deferred to V2.

4. **`GameContext` is the "fat pointer"** passed to all interface methods.
   It provides access to `GameState`, `EventBus`, and session metadata.
   Never pass these individually; always pass a `GameContext&` or
   `const GameContext&`.

5. **Dependency direction**: `campaign → session → flow/actions → core`.
   Lower layers must never include higher-layer headers.

---

## Dependency on Other game_lib Libraries

| Library | Used in |
| ------- | ------- |
| `gmLog` | `GameSession`, `ActionQueue`, `ActionWindow` — log every state transition |
| `gmSave` | `GameSession::pause()`, `CampaignState` — serialize/deserialize snapshots |
| `gmDispatch` | `EventBus` — wraps `GmDispatch::EventBusChannel` for pub/sub |
| `gmDeck` | Used only in game-specific plug-ins, never directly in gmFlow core |
| `gmMap` | Used only in game-specific plug-ins via `GameContext` |

---

## Implementation Phases

| Phase | Content |
| ----- | ------- |
| 1 | Plan (`ai-instruction_v2.md`) — **DONE** |
| 2 | All headers + stubs with full Doxygen — **DONE** |
| 3 | `gmFlow_API.md` generated from stubs |
| 4 | Body implementations, subsystem by subsystem |
| 5 | Tests (`tests/` folder) |

---

## Coding Style Checklist

- [ ] Include guard matches `GMFLOW_<UPPERCASE_FILENAME>_HPP`
- [ ] All public symbols have `@brief` Doxygen comment
- [ ] All parameters in public methods documented with `@param`
- [ ] Return values documented with `@return`
- [ ] No `auto` in public function prototypes
- [ ] Private members use `_` suffix (e.g., `status_`, `actors_`)
- [ ] Enum values SCREAMING_SNAKE_CASE
- [ ] `.cpp` stubs contain `// TODO: Phase 4.X — implement` comments
- [ ] All comments in English
