# gmFlow — Development Plan v2

**Version:** 2.0
**Date:** 2026-06-11
**Status:** Planning
**Language:** C++17 Standard
**Namespace:** `gmFlow`

> This document supersedes `ai-instruction.md` and translates its design concepts
> into a concrete, phased implementation plan aligned with the conventions of the
> existing `game_lib` libraries (`gmLog`, `gmSave`, `gmDispatch`, `gmDeck`, `gmMap`).

---

## 1. Design Philosophy

`gmFlow` is a **game flow control framework**, not a game rules engine.
It knows *who* can act, *when*, *in which context*, and *whether* an action
is structurally valid.  It does **not** know what "attack", "draw a card", or
"move a pawn" means — those are game-specific plug-ins.

```text
┌──────────────────────────────────────────────────┐
│  Game-specific code (plug-in layer)              │
│  MovePawnAction, DrawCardAction, MyGamePhase…    │
└────────────────────┬─────────────────────────────┘
                     │ implements interfaces
┌────────────────────▼─────────────────────────────┐
│  gmFlow  (framework layer)                       │
│  GameSession, IPhase, IAction, ActionWindow,     │
│  ActionQueue, FlowController, EventBus…          │
└────────────────────┬─────────────────────────────┘
                     │ uses
┌────────────────────▼─────────────────────────────┐
│  game_lib support libraries                      │
│  gmLog · gmSave · gmDispatch · gmDeck · gmMap    │
└──────────────────────────────────────────────────┘
```

---

## 2. Naming Conventions (per style-rules.md)

| Element | Convention | Example |
| ------- | ---------- | ------- |
| Namespace | `gmFlow` | `gmFlow::GameSession` |
| Façade class | `gm` + PascalCase | `gmFlowSession` (if needed) |
| Interface | `I` + PascalCase | `IAction`, `IPhase`, `IFlowController` |
| Supporting class | PascalCase | `ActionQueue`, `EventBus`, `Turn` |
| Enum values | SCREAMING_SNAKE_CASE | `ActionStatus::WAITING_FOR_INPUT` |
| Private members | snake_case + `_` suffix | `current_phase_`, `action_queue_` |
| Methods | snake_case | `submit_action()`, `is_complete()` |
| Include guard | `GMFLOW_CLASSNAME_HPP` | `GMFLOW_IACTION_HPP` |

---

## 3. Integration with Existing Libraries

| Library | How gmFlow uses it |
| ------- | ------------------ |
| `gmLog` | Log every phase transition, action submission, validation failure, event |
| `gmSave` | Serialize `GameSession` / `CampaignState` snapshots |
| `gmDispatch` | Optional: dispatch `IEvent` objects across subsystems (UI, AI, logger) |
| `gmDeck` | Game plug-ins use `gmDeck`/`gmCompDeck` inside their `IAction` implementations |
| `gmMap` | Game plug-ins use `gmMap` inside `GameContext` to represent board state |

---

## 4. File Structure

```text
gmFlow/
├── PLAN.md                        ← this file (final name)
├── ai-instructions.md             ← coding conventions for this library
├── gmFlow_API.md                  ← full API reference (written in Phase 5)
│
├── core/
│   ├── Ids.hpp                    ← PlayerId, ActionId, PhaseId, TurnId, RoundId, SessionId
│   ├── Result.hpp                 ← ActionResult, StepResult, ValidationResult
│   ├── GameContext.hpp            ← runtime context passed to all operations
│   └── GameState.hpp              ← abstract base / default mutable state
│
├── actions/
│   ├── IAction.hpp                ← interface
│   ├── IActionStep.hpp            ← interface for multi-step actions
│   ├── ActionStatus.hpp           ← enum class ActionStatus
│   ├── ActionPriority.hpp         ← enum class ActionPriority
│   ├── ActionQueue.hpp/.cpp       ← priority queue of pending actions
│   ├── ActionWindow.hpp/.cpp      ← window of opportunity (turn / reaction / free)
│   └── StepBasedAction.hpp        ← default multi-step action skeleton
│
├── flow/
│   ├── IPhase.hpp                 ← interface
│   ├── IFlowController.hpp        ← interface (replaces IGameFlowController)
│   ├── Turn.hpp                   ← single- or multi-player turn
│   ├── Round.hpp                  ← optional round container
│   ├── TurnPolicy.hpp             ← struct: simultaneous, async, pass-based
│   └── RoundPolicy.hpp            ← struct: max rounds, enabled flag
│
├── actors/
│   ├── Actor.hpp                  ← ActorId, ActorType (Player/Bot/System/Team/GM)
│   └── ActorRegistry.hpp/.cpp     ← maps ActorId → Actor at session level
│
├── events/
│   ├── IEvent.hpp                 ← interface
│   ├── EventType.hpp              ← enum class EventType (all built-in event types)
│   ├── FlowEvents.hpp             ← TurnStarted, PhaseChanged, ActionCompleted, …
│   └── EventBus.hpp/.cpp          ← subscribe/publish (wraps or delegates to gmDispatch)
│
├── session/
│   ├── GameSession.hpp/.cpp       ← main entry point
│   └── SessionConfig.hpp          ← struct with policies + initial actor list
│
├── campaign/
│   ├── Campaign.hpp/.cpp          ← sequence of sessions, persistent state
│   ├── CampaignState.hpp          ← serializable campaign progress
│   └── SessionDefinition.hpp      ← unlock conditions, session metadata
│
└── tests/
    ├── test_action_queue.cpp
    ├── test_action_window.cpp
    ├── test_flow_sequential.cpp
    └── test_flow_campaign.cpp
```

---

## 5. Core Types Reference

### Ids.hpp

```cpp
namespace gmFlow {

using PlayerId  = std::string;
using ActorId   = std::string;
using ActionId  = std::string;
using StepId    = std::string;
using PhaseId   = std::string;
using TurnId    = std::string;
using RoundId   = std::string;
using SessionId = std::string;
using EventType = std::string;

} // namespace gmFlow
```

String IDs keep the core dependency-free and easily serializable.

---

### ActionStatus

```cpp
enum class ActionStatus {
    CREATED,
    SUBMITTED,
    VALIDATING,
    WAITING_FOR_INPUT,       // multi-step action awaiting UI input
    WAITING_FOR_REACTION,    // action window open, waiting for responses
    EXECUTING,
    COMPLETED,
    FAILED,
    CANCELLED
    // SUSPENDED removed — actions are atomic in V1.
    // Session-level pause/resume is handled by GameSession::pause().
};
```

### ActionPriority

```cpp
enum class ActionPriority {
    IMMEDIATE,    // interrupts, cancels
    REACTION,     // responses to triggered windows
    NORMAL,       // standard turn action
    DEFERRED      // end-of-phase effects, cleanup
};
```

### ValidationResult

```cpp
class ValidationResult {
public:
    static ValidationResult ok();
    static ValidationResult fail(ValidationError error, std::string message);

    bool valid() const;
    const std::string& message() const;
    ValidationError error() const;
};

enum class ValidationError {
    NONE,
    NOT_ACTOR_TURN,
    PHASE_DOES_NOT_ALLOW,
    ACTION_WINDOW_CLOSED,
    INVALID_TARGET,
    NOT_ENOUGH_RESOURCES,
    RULE_VIOLATION,
    ACTION_ALREADY_SUBMITTED
};
```

---

### IAction (interface)

```cpp
class IAction {
public:
    virtual ~IAction() = default;

    virtual ActionId       id()     const = 0;
    virtual ActorId        owner()  const = 0;
    virtual ActionStatus   status() const = 0;

    virtual ValidationResult validate(const GameContext& ctx) const = 0;
    virtual ActionResult     execute(GameContext& ctx) = 0;

    virtual bool is_async()        const = 0;
    virtual bool requires_turn()   const = 0;
    virtual bool is_multi_step()   const = 0;
};
```

### IActionStep (interface)

```cpp
class IActionStep {
public:
    virtual ~IActionStep() = default;

    virtual StepId id() const = 0;
    virtual bool   can_enter(const GameContext& ctx) const = 0;
    virtual StepResult execute(GameContext& ctx, const StepInput& input) = 0;
    virtual bool   is_complete(const GameContext& ctx) const = 0;
};
```

### IPhase (interface)

```cpp
class IPhase {
public:
    virtual ~IPhase() = default;

    virtual PhaseId id() const = 0;

    virtual void on_enter(GameContext& ctx) = 0;
    virtual void on_exit(GameContext& ctx) = 0;

    virtual std::vector<std::unique_ptr<IAction>>
        available_actions(const GameContext& ctx, const ActorId& actor) const = 0;

    virtual bool is_complete(const GameContext& ctx) const = 0;
};
```

### IFlowController (interface)

```cpp
class IFlowController {
public:
    virtual ~IFlowController() = default;

    virtual void start(GameContext& ctx) = 0;
    virtual void process(GameContext& ctx) = 0;

    virtual bool can_actor_act(const GameContext& ctx, const ActorId& actor) const = 0;
    virtual void on_action_completed(GameContext& ctx, const ActionResult& result) = 0;

    virtual bool is_session_complete(const GameContext& ctx) const = 0;
};
```

### GameSession (main façade)

```cpp
class GameSession {
public:
    explicit GameSession(SessionConfig config,
                         std::unique_ptr<IFlowController> flow_controller);

    void start();
    void pause();
    void resume();
    void tick();

    ValidationResult submit_action(const ActorId& actor,
                                   std::unique_ptr<IAction> action);

    bool is_finished() const;
    bool is_paused()   const;

    const GameContext& context() const;
    EventBus&          event_bus();

private:
    SessionConfig                      config_;
    GameContext                        context_;
    std::unique_ptr<IFlowController>   flow_controller_;
    ActionQueue                        action_queue_;
    EventBus                           event_bus_;
};
```

---

## 6. Action Window — Simultaneous / Reaction Support

`ActionWindow` is the key concept for simultaneous turns, reactions, and
async out-of-turn actions.  Instead of separate code paths for each case,
everything is an `ActionWindow` with a different `CompletionPolicy`.

```cpp
enum class CompletionPolicy {
    ALL_SUBMITTED,        // everyone in the window must act
    ANY_SUBMITTED,        // first submission closes the window
    MANUAL_CLOSE,         // closed explicitly by the flow controller
    UNTIL_ALL_PASSED,     // everyone passes → window closes
    PRIORITY_RESOLVED     // actions sorted by priority, then resolved
    // TIMEOUT_EXPIRED — deferred to V2 (no timer support in V1)
};

class ActionWindow {
public:
    bool can_submit(const ActorId& actor) const;
    ValidationResult submit(const ActorId& actor, std::unique_ptr<IAction> action);
    bool is_complete(const GameContext& ctx) const;
    void resolve(GameContext& ctx);

private:
    std::vector<ActorId>        eligible_actors_;
    std::vector<SubmittedAction> submitted_actions_;
    CompletionPolicy             completion_policy_;
};
```

Standard flow maps cleanly onto windows:

| Scenario | Window type |
| -------- | ----------- |
| Normal turn | Main `ActionWindow` for one actor |
| Simultaneous turn | Main `ActionWindow` for all actors, `ALL_SUBMITTED` |
| Reaction | Nested `ActionWindow` opened by an event |
| Free out-of-turn action | `ActionWindow` opened by the flow controller |
| Card stack resolution | Nested windows with `PRIORITY_RESOLVED` |

---

## 7. State Model — Recommended Approach

**Hybrid model**: actions mutate `GameState` AND emit events.

- Simple enough for V1 (no event-sourcing complexity).
- Events enable UI updates, logging, replay, future undo.

```cpp
// Inside a concrete action implementation (game-specific plug-in):
ActionResult MovePawnAction::execute(GameContext& ctx) {
    ctx.state().move_pawn(owner_, from_, to_);

    ctx.event_bus().publish(
        std::make_unique<PawnMovedEvent>(owner_, from_, to_));

    return ActionResult::success();
}
```

---

## 8. Development Phases (Execution Order Locked)

The implementation workflow is **strictly ordered** and must be followed for
every module (`core`, `actions`, `flow`, `events`, `session`, `campaign`):

1. Plan by phases and steps.
2. Implement interfaces, headers, and function signatures with stub bodies.
3. Write inline comments in English and public API comments in Doxygen style.
4. Generate the API manual (`.md`) from the stub-level API (as done in `gmDispatch`).
5. Implement function/method bodies progressively by phase and step.
6. Add tests at the end for each completed phase/step.

### Phase 1 — Planning and phase decomposition

**Goal:** Freeze architecture, dependencies, and deliverables per phase.

Deliverables:

- Phase/step plan approved in this file.
- File tree approved.
- Integration contracts with existing `gmXxx` libraries fixed.

---

### Phase 2 — Interfaces + headers + signatures + stubs

**Goal:** Compile-ready API surface with no business logic implementation.

Rules:

- Public API declared in headers.
- `.cpp` files contain stub bodies only (TODO + deterministic placeholder result).
- Naming/style follow `style-rules.md`.
- Doxygen comments on all public symbols.

Files (initial pass):

- `core/Ids.hpp`, `core/Result.hpp`, `core/GameContext.hpp`, `core/GameState.hpp`
- `actions/IAction.hpp`, `IActionStep.hpp`, `ActionStatus.hpp`, `ActionPriority.hpp`
- `actions/ActionQueue.hpp/.cpp`, `actions/ActionWindow.hpp/.cpp`, `actions/StepBasedAction.hpp`
- `flow/IPhase.hpp`, `IFlowController.hpp`, `Turn.hpp`, `Round.hpp`, `TurnPolicy.hpp`, `RoundPolicy.hpp`
- `actors/Actor.hpp`, `actors/ActorRegistry.hpp/.cpp`
- `events/IEvent.hpp`, `EventType.hpp`, `FlowEvents.hpp`, `events/EventBus.hpp/.cpp`
- `session/GameSession.hpp/.cpp`, `session/SessionConfig.hpp`
- `campaign/Campaign.hpp/.cpp`, `campaign/CampaignState.hpp`, `campaign/SessionDefinition.hpp`

Deliverable: all files compile with stub logic only.

---

### Phase 3 — Documentation first (stub-driven manual)

**Goal:** Publish API manual before real implementation.

Files:

- `gmFlow_API.md` — generated from Phase 2 API.

Content requirements:

- Concept overview and architecture.
- Full interface reference with signatures.
- Usage examples based on stub API.
- Build commands.
- Integration section showing mandatory use of `gmDispatch`, `gmSave`, `gmLog`.

---

### Phase 4 — Body implementation by phase and step

**Goal:** Replace stubs with real logic progressively (never all at once).

Implementation sequence:

Step 4.1:

- `actions/ActionQueue.cpp`
- `actions/ActionWindow.cpp`
- `actions/StepBasedAction.hpp` (inline logic)

Step 4.2:

- `events/EventBus.cpp` (thin wrapper over `gmDispatch::EventBusChannel`)
- `events/FlowEvents.hpp` completion

Step 4.3:

- `session/GameSession.cpp`
- `flow/SequentialFlowController.hpp/.cpp`
- `actors/ActorRegistry.cpp`

Step 4.4:

- `campaign/Campaign.cpp`

Implementation constraints:

- Actions remain atomic (no mid-action suspension).
- Session pause/resume implemented via `gmSave` snapshot.
- Event delivery delegated to `gmDispatch`.

---

### Phase 5 — Tests by phase and step (final stage)

**Goal:** Add tests only after bodies are implemented for the corresponding step.

Test batches:

Batch 5.1 (after Step 4.1):

- `tests/test_action_queue.cpp`
- `tests/test_action_window.cpp`

Batch 5.2 (after Step 4.2 + 4.3):

- `tests/test_flow_sequential.cpp`

Batch 5.3 (after Step 4.4):

- `tests/test_flow_campaign.cpp`

Core scenarios:

- queue priority ordering
- action-window completion policies
- sequential turn flow and phase transition
- session pause/save/load/resume
- campaign unlock and persistence

---

## 9. Suggested Build Commands (from game_lib root)

```powershell
# Phase 2 tests — action infrastructure
clang++ -std=c++17 -I. gmFlow/actions/ActionQueue.cpp gmFlow/actions/ActionWindow.cpp `
    gmFlow/tests/test_action_queue.cpp gmFlow/tests/test_action_window.cpp `
    -o test_gmFlow_p2.exe ; ./test_gmFlow_p2.exe

# Phase 3+4 tests — session + flow
clang++ -std=c++17 -I. gmFlow/actions/ActionQueue.cpp gmFlow/actions/ActionWindow.cpp `
    gmFlow/events/EventBus.cpp gmFlow/session/GameSession.cpp `
    gmFlow/flow/SequentialFlowController.cpp gmFlow/actors/ActorRegistry.cpp `
    gmFlow/tests/test_flow_sequential.cpp -o test_gmFlow_p4.exe ; ./test_gmFlow_p4.exe

# Phase 5 tests — campaign (requires gmSave)
clang++ -std=c++17 -I. gmFlow/actions/ActionQueue.cpp gmFlow/actions/ActionWindow.cpp `
    gmFlow/events/EventBus.cpp gmFlow/session/GameSession.cpp `
    gmFlow/flow/SequentialFlowController.cpp gmFlow/actors/ActorRegistry.cpp `
    gmFlow/campaign/Campaign.cpp gmSave/gmSave.cpp `
    gmFlow/tests/test_flow_campaign.cpp -o test_gmFlow_p5.exe ; ./test_gmFlow_p5.exe
```

---

## 10. Design Decisions Summary

| Decision | Choice | Rationale |
| -------- | ------ | --------- |
| State model | Hybrid: mutate + emit events | Simple V1, events enable logging/replay |
| Simultaneous turns | `ActionWindow` with `CompletionPolicy` | Unifies all async/reaction cases |
| Player model | `Actor` with `ActorType` | Supports bots, system, teams from day one |
| IDs | `std::string` aliases | Serializable, debuggable, no integer collisions |
| Template usage | Minimal — virtual interfaces preferred | Keeps error messages readable, code debuggable |
| Serialization | ID-based references, state/behaviour separate | gmSave-compatible from the start |
| Campaign vs Session | Campaign is above Session, not inside | Clean layering, campaign is optional |
| EventBus | Thin wrapper over `gmDispatch::EventBusChannel` | Reuse existing lib, no duplicate pub/sub |
| Session persistence | `gmSave` serializes `GameSession` snapshot | `pause()` saves, `resume()` reloads |
| Action atomicity | Actions are atomic — no mid-action suspension | Session suspends; individual actions do not |
| Undo/redo | Deferred to V2 | Not needed for game-design testing in V1 |
| Timer support | Deferred to V2 | `TIMEOUT_EXPIRED` policy removed from V1 |
| UI isolation | gmFlow emits events only, never calls UI | UI listens to EventBus and renders independently |

---

## 11. Open Questions — ✅ Resolved

**1. Single vs multi-game target** ✅

V1 must support all of these archetypes without specialisation:

| Game | Key flow characteristics |
| ---- | ------------------------ |
| Hero Quest / Dungeon Crawler | Sequential turns, phases (hero act → monster act), map movement, combat |
| Gloomhaven | Scenario-based sessions, campaign persistence, simultaneous planning + sequential resolution |
| Risiko! | Sequential turns, multi-phase per turn (reinforce → attack → manoeuvre), reaction windows |
| Game of Thrones board game | Simultaneous action programming, bidding/voting, multiple sub-phases |
| Generic wargame on map | Alternating or simultaneous turns, stack resolution, priority-based reaction windows |

Implication: `ActionWindow` + `CompletionPolicy` must cover sequential, simultaneous,
and reaction-window patterns from day one. The campaign layer is required in V1.

---

**2. Undo/redo** ✅ — **Deferred to V2.**

`GameState` does not need to be copyable/snapshottable in V1.
Actions are applied directly and are not reversible.

---

**3. Timer support** ✅ — **Deferred to V2.**

`CompletionPolicy::TIMEOUT_EXPIRED` is removed from V1.
`ActionWindow` has no timer dependency.

---

**4. `gmDispatch` integration** ✅ — **Mandatory.**

Wherever an existing `gmXxx` library solves a problem, use it.
`gmFlow` does not reinvent event delivery, serialisation, or logging.

| Responsibility | Library used |
| -------------- | ------------ |
| Event pub/sub | `gmDispatch::EventBusChannel` |
| Session / campaign persistence | `gmSave` |
| Diagnostic logging | `gmLog` |
| Deck / card lifecycle | `gmDeck` / `gmCompDeck` (in game plug-ins) |
| Board topology | `gmMap` (in game plug-ins) |

---

**5. Session suspension / multiplayer** ✅

- **Actions are atomic.** `ActionStatus::SUSPENDED` is removed.
  An action is either `COMPLETED`, `FAILED`, or `CANCELLED`. It never
  persists in a half-executed state across save/load boundaries.
- **Sessions can be paused and resumed.** `GameSession::pause()` serializes
  the full session snapshot via `gmSave`. `GameSession::resume()` reloads it.
  On restart the session continues from the exact phase / round / turn.
- Primary V1 use case: **save and resume a game-design test session**
  without losing progress.

---

**Created:** 2026-06-11
**Namespace:** `gmFlow`
**Depends on:** C++17 stdlib, optionally `gmLog`, `gmSave`, `gmDispatch`
