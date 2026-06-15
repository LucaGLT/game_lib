# gmFlow — TimelineFlowController Implementation Instructions

**Version:** 0.1.0  
**Status:** Planning / Implementation Brief  
**Language:** C++17 Standard  
**Target library:** `gmFlow`  
**Namespace:** `gmFlow`  
**Goal:** extend the existing `gmFlow` framework with a generic continuous-timeline flow controller suitable for tabletop games where the next actor is selected by the lowest timeline position rather than by classic round/turn order.

---

## 0. Mandatory constraints for the implementing AI

You are extending the existing `gmFlow` library. Do **not** rewrite `gmFlow` from scratch.

Follow the style, file discipline, naming conventions, API documentation approach, and implementation order already used in the existing `gmXxx` libraries, especially:

- `gmFlow`
- `gmDispatch`
- `gmSave`
- `gmLog`
- `gmDeck`
- `gmCompDeck`
- `gmMap`

The implementation must:

- Use **C++17 standard library only**, plus the local `gmXxx` libraries already used by `gmFlow`.
- Keep the design **generic**. Do not hardcode combat rules, card rules, map rules, hit points, or any game-specific title.
- Preserve the existing `gmFlow` architecture:
  - `GameSession` remains the session façade.
  - `IFlowController` remains the flow-control abstraction.
  - `IAction` remains the action plug-in abstraction.
  - `GameContext` remains the fat pointer passed to flow/actions/phases.
  - `EventBus` continues to wrap `gmDispatch`.
- Add a new controller alternative to `SequentialFlowController`; do not remove or break `SequentialFlowController`.
- Keep `Round` optional and unused by this controller.
- Keep actions atomic. Do not reintroduce `ActionStatus::SUSPENDED`.
- Add Doxygen-style comments to all public API symbols.
- Update API documentation after adding the new public types.
- Add tests after implementation, following the existing `gmFlow/tests` style.
- Preserve dependency direction: lower layers must not include higher-layer headers.
- Use string IDs already defined by `gmFlow::ActorId`, `gmFlow::ActionId`, etc.

This feature must support a tactical mission/session engine where actors may include player actors, AI-controlled groups, bosses, and system actors. The controller must not know what these actors represent internally.

---

## 1. Existing `gmFlow` concepts to reuse

The existing `gmFlow` design already provides the right extension points:

```text
GameSession
  owns GameContext, ActorRegistry, ActionQueue, EventBus

IFlowController
  controls who may act and when

IAction
  validates and executes game-specific actions

ActionWindow
  represents a temporary opportunity for one or more actors to submit actions

EventBus
  wraps gmDispatch and publishes lifecycle events
```

The new `TimelineFlowController` must be implemented as another `IFlowController` implementation, just like `SequentialFlowController`.

Conceptual layering:

```text
┌──────────────────────────────────────────────────────────────┐
│ Game-specific code                                           │
│ Timeline positions, actor state, effects, card rules, AI      │
└────────────────────────────┬─────────────────────────────────┘
                             │ implements adapter
┌────────────────────────────▼─────────────────────────────────┐
│ gmFlow::TimelineFlowController                               │
│ Selects next actor by timeline, validates actor eligibility   │
└────────────────────────────┬─────────────────────────────────┘
                             │ uses
┌────────────────────────────▼─────────────────────────────────┐
│ gmFlow core                                                   │
│ GameSession, GameContext, IAction, ActionWindow, EventBus     │
└──────────────────────────────────────────────────────────────┘
```

The controller must know **when** and **who**, not **what an action means**.

---

## 2. Design goal

Implement a continuous timeline controller with this behavior:

1. Each eligible actor has a numeric `timeline_position`.
2. The actor with the lowest `timeline_position` becomes the active actor.
3. If multiple actors share the same lowest position, use a deterministic tie-break policy.
4. A normal action is accepted only from the active actor.
5. Reaction or instant actions may be accepted from non-active actors only when an `ActionWindow` allows them.
6. After an action completes, the controller checks whether the active actor keeps control.
7. If the actor does not keep control, select the next lowest-position actor.
8. The controller never applies game rules such as damage, movement, card drawing, or status effects.
9. The controller does not require rounds.
10. The controller emits timeline-specific events through the existing `EventBus`.

---

## 3. New files to add

Add the following files under `gmFlow/`:

```text
gmFlow/flow/TimelineTypes.hpp
gmFlow/flow/TimelinePolicy.hpp
gmFlow/flow/ITimelineAdapter.hpp
gmFlow/flow/TimelineFlowController.hpp
gmFlow/flow/TimelineFlowController.cpp
gmFlow/events/TimelineEvents.hpp
gmFlow/tests/test_timeline_flow_controller.cpp
```

Update these existing files if needed:

```text
gmFlow/events/EventType.hpp
gmFlow/gmFlow_API.md
gmFlow/ai-instruction_v2.md or PLAN.md, if it tracks implemented phases
```

Do not modify unrelated modules unless required for compilation.

---

## 4. TimelineTypes.hpp

Create timeline-specific type aliases.

```cpp
#ifndef GMFLOW_TIMELINETYPES_HPP
#define GMFLOW_TIMELINETYPES_HPP

#include <cstdint>

namespace gmFlow {

using TimelineValue = std::int64_t;

} // namespace gmFlow

#endif // GMFLOW_TIMELINETYPES_HPP
```

Rationale:

- `TimelineValue` must support long sessions and large values.
- Use a signed type to make validation and sentinel handling easier.
- Game-specific code may still restrict values to non-negative numbers.

---

## 5. TimelinePolicy.hpp

Create a small configuration struct for the controller.

```cpp
#ifndef GMFLOW_TIMELINEPOLICY_HPP
#define GMFLOW_TIMELINEPOLICY_HPP

namespace gmFlow {

struct TimelinePolicy {
    bool allow_reactions = true;
    bool auto_select_next_actor = true;
    bool publish_timeline_events = true;
    bool open_main_action_window = true;
    bool stable_tie_break = true;
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEPOLICY_HPP
```

Semantics:

| Field | Meaning |
| ----- | ------- |
| `allow_reactions` | Allows the controller to manage reaction/instant action windows. |
| `auto_select_next_actor` | After action completion, automatically choose the next actor when control is not retained. |
| `publish_timeline_events` | Publish timeline events on the session `EventBus`. |
| `open_main_action_window` | Open a main action window for the selected active actor. |
| `stable_tie_break` | Preserve registry/order stability when timeline position and rank are equal. |

Do not add timer support. Timer-based windows are explicitly deferred to V2 in existing `gmFlow` planning.

---

## 6. ITimelineAdapter.hpp

`TimelineFlowController` must stay generic. It must obtain game-specific timeline information through an adapter interface.

Create:

```cpp
#ifndef GMFLOW_ITIMELINEADAPTER_HPP
#define GMFLOW_ITIMELINEADAPTER_HPP

#include <vector>
#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"

namespace gmFlow {

class ITimelineAdapter {
public:
    virtual ~ITimelineAdapter() = default;

    virtual std::vector<ActorId>
    timeline_actors(const GameContext& ctx) const = 0;

    virtual TimelineValue
    timeline_position(const GameContext& ctx,
                      const ActorId& actor) const = 0;

    virtual bool
    is_actor_enabled(const GameContext& ctx,
                     const ActorId& actor) const = 0;

    virtual int
    tie_break_rank(const GameContext& ctx,
                   const ActorId& actor) const = 0;

    virtual bool
    actor_keeps_control(const GameContext& ctx,
                        const ActorId& actor) const = 0;

    virtual bool
    is_session_complete(const GameContext& ctx) const = 0;

    virtual void
    on_actor_selected(GameContext& ctx,
                      const ActorId& actor) = 0;

    virtual void
    on_time_advanced(GameContext& ctx,
                     TimelineValue old_time,
                     TimelineValue new_time) = 0;
};

} // namespace gmFlow

#endif // GMFLOW_ITIMELINEADAPTER_HPP
```

### Adapter responsibilities

The adapter is implemented by game-specific code.

It must know:

- which actors participate in the timeline;
- each actor's current timeline position;
- whether an actor is enabled or skipped;
- how to rank ties;
- whether an actor should keep control after an action;
- whether the session is complete;
- how to resolve game-specific consequences when time advances.

### Controller responsibilities

The controller must not know:

- hit points;
- cards;
- decks;
- map areas;
- monster groups;
- player classes;
- combat rules;
- objectives;
- status effect rules.

All of those remain game-specific.

---

## 7. Timeline events

Add timeline-specific event constants to `gmFlow/events/EventType.hpp`.

Recommended constants:

```cpp
inline constexpr const char* EVT_TIMELINE_ACTOR_SELECTED =
    "gmFlow.timeline.actor_selected";

inline constexpr const char* EVT_TIMELINE_TIME_ADVANCED =
    "gmFlow.timeline.time_advanced";

inline constexpr const char* EVT_TIMELINE_TIE_DETECTED =
    "gmFlow.timeline.tie_detected";

inline constexpr const char* EVT_TIMELINE_NO_ACTOR_AVAILABLE =
    "gmFlow.timeline.no_actor_available";
```

If the existing codebase does not use `inline constexpr`, follow the existing constant style exactly.

Create `gmFlow/events/TimelineEvents.hpp`.

```cpp
#ifndef GMFLOW_TIMELINEEVENTS_HPP
#define GMFLOW_TIMELINEEVENTS_HPP

#include <vector>
#include "gmFlow/core/Ids.hpp"
#include "gmFlow/events/IEvent.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"

namespace gmFlow {

struct TimelineActorSelectedEvent : IEvent {
    ActorId actor_id;
    TimelineValue timeline_position = 0;

    EventType type() const override {
        return EVT_TIMELINE_ACTOR_SELECTED;
    }
};

struct TimelineTimeAdvancedEvent : IEvent {
    TimelineValue old_time = 0;
    TimelineValue new_time = 0;

    EventType type() const override {
        return EVT_TIMELINE_TIME_ADVANCED;
    }
};

struct TimelineTieDetectedEvent : IEvent {
    std::vector<ActorId> tied_actors;
    TimelineValue timeline_position = 0;

    EventType type() const override {
        return EVT_TIMELINE_TIE_DETECTED;
    }
};

struct TimelineNoActorAvailableEvent : IEvent {
    EventType type() const override {
        return EVT_TIMELINE_NO_ACTOR_AVAILABLE;
    }
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEEVENTS_HPP
```

Keep events lightweight. Use IDs and values, not pointers to live objects.

---

## 8. TimelineFlowController public API

Create `TimelineFlowController.hpp`.

```cpp
#ifndef GMFLOW_TIMELINEFLOWCONTROLLER_HPP
#define GMFLOW_TIMELINEFLOWCONTROLLER_HPP

#include <memory>
#include <optional>
#include <vector>

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/flow/IFlowController.hpp"
#include "gmFlow/flow/ITimelineAdapter.hpp"
#include "gmFlow/flow/TimelinePolicy.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"
#include "gmFlow/actions/ActionWindow.hpp"

namespace gmFlow {

class TimelineFlowController : public IFlowController {
public:
    TimelineFlowController(std::unique_ptr<ITimelineAdapter> adapter,
                           TimelinePolicy policy = TimelinePolicy{});

    void start(GameContext& ctx) override;
    void process(GameContext& ctx) override;

    bool can_actor_act(const GameContext& ctx,
                       const ActorId& actor) const override;

    void on_action_completed(GameContext& ctx,
                             const ActionResult& result) override;

    bool is_session_complete(const GameContext& ctx) const override;

    const std::optional<ActorId>& active_actor() const;
    TimelineValue current_time() const;

    bool has_action_window() const;
    void force_close_action_window();

    void open_reaction_window(std::vector<ActorId> eligible_actors,
                              CompletionPolicy policy);

protected:
    std::optional<ActorId> select_next_actor(GameContext& ctx);
    std::vector<ActorId> sorted_enabled_actors(const GameContext& ctx) const;
    TimelineValue compute_current_time(const GameContext& ctx) const;
    void publish_actor_selected(GameContext& ctx,
                                const ActorId& actor,
                                TimelineValue position) const;
    void publish_time_advanced(GameContext& ctx,
                               TimelineValue old_time,
                               TimelineValue new_time) const;
    void publish_tie_detected(GameContext& ctx,
                              const std::vector<ActorId>& tied,
                              TimelineValue position) const;
    void publish_no_actor_available(GameContext& ctx) const;

private:
    std::unique_ptr<ITimelineAdapter> adapter_;
    TimelinePolicy policy_;
    std::optional<ActorId> active_actor_;
    TimelineValue current_time_ = 0;
    std::optional<ActionWindow> current_window_;
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEFLOWCONTROLLER_HPP
```

### Notes

- `open_reaction_window()` is intentionally public for game-specific code that owns or can access the concrete controller.
- If the project prefers not to expose this method publicly, make it protected and open windows through a game-specific subclass.
- Do not add hardcoded reaction logic in this controller.
- `current_window_` is for actor eligibility. Actual action validation remains in `IAction::validate()`.

---

## 9. Selection algorithm

The core selection algorithm must be deterministic.

Pseudo-code:

```cpp
std::optional<ActorId>
TimelineFlowController::select_next_actor(GameContext& ctx) {
    auto actors = sorted_enabled_actors(ctx);

    if (actors.empty()) {
        active_actor_.reset();
        publish_no_actor_available(ctx);
        return std::nullopt;
    }

    const ActorId& selected = actors.front();
    active_actor_ = selected;

    const auto selected_pos = adapter_->timeline_position(ctx, selected);

    // Detect tie at the lowest timeline position before tie-break resolution.
    std::vector<ActorId> tied;
    for (const auto& id : actors) {
        if (adapter_->timeline_position(ctx, id) == selected_pos) {
            tied.push_back(id);
        }
    }
    if (tied.size() > 1) {
        publish_tie_detected(ctx, tied, selected_pos);
    }

    adapter_->on_actor_selected(ctx, selected);
    publish_actor_selected(ctx, selected, selected_pos);

    if (policy_.open_main_action_window) {
        current_window_.emplace(std::vector<ActorId>{selected},
                                CompletionPolicy::MANUAL_CLOSE);
    }

    return selected;
}
```

`sorted_enabled_actors()` must sort by:

1. `timeline_position` ascending;
2. `tie_break_rank` ascending;
3. original order from `timeline_actors(ctx)` if `stable_tie_break == true`.

Do not sort alphabetically as a hidden fallback unless all else is unavailable. Stable ordering is more predictable for tests and game replay.

---

## 10. Current time calculation

The default timeline current time is the minimum timeline position among enabled actors.

```cpp
TimelineValue
TimelineFlowController::compute_current_time(const GameContext& ctx) const {
    auto actors = adapter_->timeline_actors(ctx);
    bool found = false;
    TimelineValue min_value = 0;

    for (const auto& actor : actors) {
        if (!adapter_->is_actor_enabled(ctx, actor)) {
            continue;
        }
        const auto pos = adapter_->timeline_position(ctx, actor);
        if (!found || pos < min_value) {
            min_value = pos;
            found = true;
        }
    }

    return found ? min_value : current_time_;
}
```

Rationale:

- In a continuous timeline system, global mission time advances when the lowest active position advances.
- Game-specific code may use `on_time_advanced()` to resolve threshold events.
- The controller must not resolve mission events directly.

---

## 11. `start(ctx)` behavior

Implementation:

1. Compute `current_time_` from enabled actors.
2. Select the first actor.
3. Publish timeline events if enabled.
4. Do not mutate game-specific actor state except through adapter hooks.

Pseudo-code:

```cpp
void TimelineFlowController::start(GameContext& ctx) {
    current_time_ = compute_current_time(ctx);
    select_next_actor(ctx);
}
```

---

## 12. `process(ctx)` behavior

Implementation:

1. If adapter says session is complete, do nothing.
2. If an action window is open, do not auto-select another actor.
3. If no active actor exists and auto-selection is enabled, select next actor.
4. Otherwise, leave the state unchanged.

Pseudo-code:

```cpp
void TimelineFlowController::process(GameContext& ctx) {
    if (adapter_->is_session_complete(ctx)) {
        return;
    }

    if (current_window_.has_value() && !current_window_->is_closed()) {
        return;
    }

    if (!active_actor_.has_value() && policy_.auto_select_next_actor) {
        select_next_actor(ctx);
    }
}
```

Do not drain the `ActionQueue` directly here unless the existing `SequentialFlowController` already follows that pattern. Follow existing `gmFlow` conventions.

---

## 13. `can_actor_act(ctx, actor)` behavior

Eligibility rules:

1. If there is an open `ActionWindow` and it accepts `actor`, return true.
2. Otherwise, return true only if `actor == active_actor_`.
3. Disabled actors must not act.

Pseudo-code:

```cpp
bool TimelineFlowController::can_actor_act(const GameContext& ctx,
                                           const ActorId& actor) const {
    if (!adapter_->is_actor_enabled(ctx, actor)) {
        return false;
    }

    if (current_window_.has_value() &&
        !current_window_->is_closed() &&
        current_window_->can_submit(actor)) {
        return true;
    }

    return active_actor_.has_value() && active_actor_.value() == actor;
}
```

Important: this method checks structural flow eligibility only. Game-rule legality remains in `IAction::validate()`.

---

## 14. `on_action_completed(ctx, result)` behavior

After each completed action:

1. Compute the new current time.
2. If time advanced, call `adapter_->on_time_advanced(ctx, old_time, new_time)`.
3. Publish `TimelineTimeAdvancedEvent`.
4. If the active actor keeps control, keep `active_actor_` and keep or reopen a main window.
5. Otherwise, close/reset the current window, clear active actor, and select the next actor if policy allows.

Pseudo-code:

```cpp
void TimelineFlowController::on_action_completed(GameContext& ctx,
                                                 const ActionResult& result) {
    if (!result.succeeded()) {
        if (current_window_.has_value()) {
            current_window_->force_close();
        }
        return;
    }

    const auto old_time = current_time_;
    const auto new_time = compute_current_time(ctx);

    if (new_time > old_time) {
        current_time_ = new_time;
        adapter_->on_time_advanced(ctx, old_time, new_time);
        publish_time_advanced(ctx, old_time, new_time);
    }

    if (active_actor_.has_value() &&
        adapter_->actor_keeps_control(ctx, active_actor_.value())) {
        if (policy_.open_main_action_window &&
            (!current_window_.has_value() || current_window_->is_closed())) {
            current_window_.emplace(std::vector<ActorId>{active_actor_.value()},
                                    CompletionPolicy::MANUAL_CLOSE);
        }
        return;
    }

    if (current_window_.has_value()) {
        current_window_->force_close();
    }
    current_window_.reset();
    active_actor_.reset();

    if (policy_.auto_select_next_actor && !adapter_->is_session_complete(ctx)) {
        select_next_actor(ctx);
    }
}
```

Do not assume how actions advance timeline values. Actions mutate game-specific state; the adapter reads the updated positions afterward.

---

## 15. Actor keeps control

The adapter method `actor_keeps_control(ctx, actor)` is required for games where an actor may perform a multi-part sequence and retain control across several atomic actions.

Examples of generic use cases:

- card sequence still open;
- chained action still resolving;
- active actor has chosen to continue a combo;
- a multi-step action is awaiting confirmation.

The controller does not inspect sequence state directly. It simply asks the adapter.

---

## 16. Reaction windows

`ActionWindow` should be reused, not duplicated.

The controller should support opening a reaction window:

```cpp
void TimelineFlowController::open_reaction_window(
    std::vector<ActorId> eligible_actors,
    CompletionPolicy policy
) {
    if (!policy_.allow_reactions) {
        return;
    }
    current_window_.emplace(std::move(eligible_actors), policy);
}
```

Typical policies:

| Use case | CompletionPolicy |
| -------- | ---------------- |
| first reaction wins | `ANY_SUBMITTED` |
| all eligible actors may respond/pass | `UNTIL_ALL_PASSED` |
| collect then resolve by priority | `PRIORITY_RESOLVED` |
| explicitly closed by controller/game | `MANUAL_CLOSE` |

Do not implement trigger discovery inside `TimelineFlowController`. Trigger discovery belongs to game-specific code.

---

## 17. Interaction with `GameSession::submit_action()`

Existing `GameSession::submit_action()` should continue to perform two-stage validation:

1. `flow_controller->can_actor_act(ctx, actor)`
2. `action->validate(ctx)`

The new controller must therefore make `can_actor_act()` correct and side-effect-free.

If existing `GameSession` does not notify the flow controller after action execution, add or verify this call:

```cpp
flow_controller_->on_action_completed(context_, result);
```

This is already part of the `IFlowController` contract and must be preserved.

---

## 18. What not to implement

Do **not** implement any of these in `TimelineFlowController`:

```text
hit point rules
movement rules
deck/card draw rules
card sequence legality
combat targeting
status effects
mission objectives
AI behavior
map traversal
inventory/equipment logic
boss phases
hardcoded actor types
hardcoded tie-break categories
round start/end logic
undo/redo
timer-based windows
```

Those belong to the game-specific engine or other `gmXxx` libraries.

---

## 19. Error handling and validation

The controller should fail safely.

Recommended behavior:

- If adapter pointer is null in constructor: throw `std::invalid_argument`.
- If `timeline_actors()` contains duplicate actor IDs: either ignore duplicates deterministically or throw. Prefer throwing in debug/test path if existing library style supports it.
- If no enabled actor exists: publish `TimelineNoActorAvailableEvent` and leave `active_actor_` empty.
- If `timeline_position()` returns negative values: allow it at controller level; game-specific validation may forbid it.
- If `on_time_advanced()` throws: allow exception to propagate unless existing `gmFlow` style catches flow exceptions.

Do not silently correct broken game state.

---

## 20. Tests to implement

Create `gmFlow/tests/test_timeline_flow_controller.cpp`.

Use a minimal fake game state and fake adapter.

### Fake state

```cpp
class FakeTimelineState : public gmFlow::GameState {
public:
    std::unordered_map<gmFlow::ActorId, gmFlow::TimelineValue> positions;
    std::unordered_map<gmFlow::ActorId, bool> enabled;
    std::unordered_map<gmFlow::ActorId, int> ranks;
    bool complete = false;
    bool keep_control = false;
    gmFlow::ActorId selected_actor;
    int time_advanced_calls = 0;

    // implement GameState interface...
};
```

### Fake adapter

```cpp
class FakeTimelineAdapter : public gmFlow::ITimelineAdapter {
public:
    // read/write FakeTimelineState through static_cast from GameContext::state()
};
```

### Required tests

1. **Selects lowest timeline actor**
   - A at 5, B at 2, C at 8.
   - Selected actor must be B.

2. **Tie-break rank resolves same timeline value**
   - A at 3 rank 2, B at 3 rank 1.
   - Selected actor must be B.

3. **Stable tie-break preserves adapter order**
   - A and B both at 3 and same rank.
   - If adapter order is `[A, B]`, selected actor must be A.

4. **Disabled actors are ignored**
   - A at 0 but disabled; B at 5 enabled.
   - Selected actor must be B.

5. **can_actor_act accepts only active actor**
   - Active actor A.
   - `can_actor_act(A)` true.
   - `can_actor_act(B)` false.

6. **Reaction window allows non-active actor**
   - Active actor A.
   - Open reaction window for B.
   - `can_actor_act(B)` true.

7. **Current time advances when minimum position advances**
   - Start with A at 0, B at 2.
   - Move A to 5 in fake state.
   - After action completion, current time becomes 2.

8. **actor_keeps_control prevents immediate actor switch**
   - Active actor A.
   - Adapter returns true for `actor_keeps_control()`.
   - After action completion, active actor remains A.

9. **No enabled actors produces no active actor**
   - All actors disabled.
   - Controller active actor is empty.
   - No crash.

10. **Session complete delegates to adapter**
    - Adapter returns complete.
    - `is_session_complete(ctx)` returns true.

Tests must be deterministic and must not rely on wall-clock time.

---

## 21. Documentation update

Update `gmFlow_API.md` with a new section:

```text
flow/ — Timeline Flow Control
  TimelineValue
  TimelinePolicy
  ITimelineAdapter
  TimelineFlowController
  Timeline events
```

Include a minimal example:

```cpp
class MyTimelineAdapter : public gmFlow::ITimelineAdapter { ... };

auto controller = std::make_unique<gmFlow::TimelineFlowController>(
    std::make_unique<MyTimelineAdapter>()
);

gmFlow::GameSession session(cfg, std::move(controller), std::move(state), dispatcher);
```

The example must remain generic. Do not include game-specific combat or card rules.

---

## 22. Build command example

Add a build command similar to existing `gmFlow` test commands.

Example:

```powershell
clang++ -std=c++17 -I. `
    gmFlow/actions/ActionQueue.cpp `
    gmFlow/actions/ActionWindow.cpp `
    gmFlow/events/EventBus.cpp `
    gmFlow/session/GameSession.cpp `
    gmFlow/flow/TimelineFlowController.cpp `
    gmFlow/actors/ActorRegistry.cpp `
    gmFlow/tests/test_timeline_flow_controller.cpp `
    -o test_gmFlow_timeline.exe ; ./test_gmFlow_timeline.exe
```

Adjust the command to match the actual repository layout.

---

## 23. Implementation order

Follow the same phased approach used in the existing `gmFlow` plan.

### Phase 1 — Planning

- Confirm file list.
- Confirm public API.
- Confirm no breaking changes to existing `gmFlow` classes.

### Phase 2 — Headers and stubs

Create all new headers and `.cpp` files with stub implementations.

The code must compile before full logic is added.

### Phase 3 — Documentation

Update `gmFlow_API.md` from the stub-level API.

### Phase 4 — Implementation

Implement in this order:

1. `TimelineTypes.hpp`
2. `TimelinePolicy.hpp`
3. `ITimelineAdapter.hpp`
4. `TimelineEvents.hpp`
5. `TimelineFlowController` constructor and trivial accessors
6. `sorted_enabled_actors()`
7. `compute_current_time()`
8. `select_next_actor()`
9. `start()` / `process()`
10. `can_actor_act()`
11. `on_action_completed()`
12. reaction-window support
13. event publishing helpers

### Phase 5 — Tests

Add tests only after the implementation compiles.

---

## 24. Acceptance criteria

The implementation is acceptable when:

- Existing `gmFlow` tests still pass.
- New timeline tests pass.
- `SequentialFlowController` is not broken.
- The new controller can select actors by lowest timeline position.
- Ties are deterministic.
- Disabled actors are skipped.
- Reaction/action windows can make non-active actors eligible.
- `actor_keeps_control()` allows a game-specific sequence to retain control.
- Timeline events are published via the existing `EventBus` / `gmDispatch` path.
- No game-specific rules are hardcoded.
- Public API is documented.

---

## 25. Final design summary

`TimelineFlowController` is a generic controller for continuous-time tabletop sessions.

It provides:

```text
who acts next
when global timeline time advances
whether a submitted actor is flow-eligible
when timeline events are published
how reaction/action windows affect eligibility
```

It does not provide:

```text
combat rules
card rules
movement rules
AI rules
mission objectives
actor statistics
inventory logic
```

The game-specific layer must implement `ITimelineAdapter` and the actual `IAction` classes.

This keeps `gmFlow` reusable while supporting continuous timeline games cleanly.
