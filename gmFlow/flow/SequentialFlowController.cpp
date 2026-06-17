/**
 * @file flow/SequentialFlowController.cpp
 * @brief Implementation of gmFlow::SequentialFlowController.
 */

#include "gmFlow/flow/SequentialFlowController.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/FlowEvents.hpp"

#include <stdexcept>

namespace gmFlow {

SequentialFlowController::SequentialFlowController(
    std::vector<std::unique_ptr<IPhase>> phases,
    TurnPolicy                           turn_policy,
    RoundPolicy                          round_policy)
    : _phases(std::move(phases))
    , _turn_policy(turn_policy)
    , _round_policy(round_policy)
{
    if (_phases.empty()) {
        throw std::invalid_argument(
            "SequentialFlowController: phases list must not be empty");
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void SequentialFlowController::start(GameContext& ctx)
{
    _current_phase_index = 0;
    _current_actor_index = 0;
    _round_index         = 1;
    _session_complete    = false;
    _rounds_exhausted    = false;

    // Enter first phase.
    const PhaseId first_id = _phases[0]->id();
    ctx.set_current_phase_id(first_id);
    _phases[0]->on_enter(ctx);

    PhaseEnteredEvent pev;
    pev.phase_id    = first_id;
    pev.previous_id = "";
    ctx.event_bus().publish(pev);

    // Start first round.
    if (_round_policy.enabled) {
        const RoundId rid = "round_" + std::to_string(_round_index);
        ctx.set_current_round_id(rid);

        RoundStartedEvent rev;
        rev.round_id = rid;
        rev.index    = _round_index;
        ctx.event_bus().publish(rev);
    }

    // Announce session start.
    SessionStartedEvent sev;
    sev.session_id = ctx.session_id();
    ctx.event_bus().publish(sev);

    open_next_turn(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────

void SequentialFlowController::process(GameContext& ctx)
{
    if (_session_complete || _rounds_exhausted) return;
    if (!_current_window)                       return;
    if (!_current_window->is_complete(ctx))     return;

    // Resolve window: executes stored actions and emits WindowClosed.
    _current_window->resolve(ctx);
    _current_window.reset();

    // Emit TurnEnded.
    TurnEndedEvent tev;
    tev.turn_id = ctx.current_turn_id();
    ctx.event_bus().publish(tev);

    // Advance turn counter.
    const std::vector<ActorId> order = determine_turn_order(ctx);
    const std::size_t n = order.size();
    ++_current_actor_index;

    // Check if we completed one full actor cycle (= one round).
    if (n > 0 && _current_actor_index % n == 0) {
        if (_round_policy.enabled) {
            RoundEndedEvent rev;
            rev.round_id = ctx.current_round_id();
            rev.index    = _round_index;
            ctx.event_bus().publish(rev);

            if (_round_policy.max_rounds > 0 && _round_index >= _round_policy.max_rounds) {
                _rounds_exhausted = true;
                return;  // is_session_complete() will now return true.
            }

            ++_round_index;
            const RoundId new_rid = "round_" + std::to_string(_round_index);
            ctx.set_current_round_id(new_rid);

            RoundStartedEvent rsev;
            rsev.round_id = new_rid;
            rsev.index    = _round_index;
            ctx.event_bus().publish(rsev);
        }
    }

    // Check if the current phase is complete.
    if (_phases[_current_phase_index]->is_complete(ctx)) {
        advance_phase(ctx);
    } else {
        open_next_turn(ctx);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

bool SequentialFlowController::can_actor_act(const GameContext& /*ctx*/,
                                             const ActorId& actor) const
{
    if (!_current_window || _current_window->is_closed()) return false;
    return _current_window->can_submit(actor);
}

void SequentialFlowController::on_action_completed(GameContext& /*ctx*/,
                                                   const ActionResult& /*result*/)
{
    // ActionWindow::resolve() already handles execution and events.
    // For MANUAL_CLOSE windows (allow_multiple_actions_per_turn), the flow
    // controller would force_close() here when a PASS action arrives.
    // For standard ANY_SUBMITTED windows this is a no-op.
}

ValidationResult SequentialFlowController::accept_action(
    GameContext& /*ctx*/,
    const ActorId& actor,
    std::unique_ptr<IAction> action)
{
    if (!_current_window) {
        return ValidationResult::fail(
            ValidationError::ACTION_WINDOW_CLOSED,
            "No active action window.");
    }
    return _current_window->submit(actor, std::move(action));
}

bool SequentialFlowController::is_session_complete(const GameContext& /*ctx*/) const
{
    return _session_complete || _rounds_exhausted;
}

// ─────────────────────────────────────────────────────────────────────────────

std::vector<ActorId> SequentialFlowController::determine_turn_order(
    const GameContext& ctx) const
{
    return ctx.actor_registry().all_ids();
}

void SequentialFlowController::advance_phase(GameContext& ctx)
{
    const PhaseId exiting_id = _phases[_current_phase_index]->id();
    const std::size_t next_idx = _current_phase_index + 1;
    const PhaseId next_id = (next_idx < _phases.size())
                             ? _phases[next_idx]->id()
                             : "";

    // Announce phase exit before calling on_exit().
    PhaseExitedEvent pxev;
    pxev.phase_id = exiting_id;
    pxev.next_id  = next_id;
    ctx.event_bus().publish(pxev);

    _phases[_current_phase_index]->on_exit(ctx);
    ++_current_phase_index;
    _current_actor_index = 0;

    if (_current_phase_index >= _phases.size()) {
        _session_complete = true;
        SessionCompletedEvent sev;
        sev.session_id = ctx.session_id();
        ctx.event_bus().publish(sev);
        return;
    }

    // Enter the next phase.
    ctx.set_current_phase_id(next_id);
    _phases[_current_phase_index]->on_enter(ctx);

    PhaseEnteredEvent penev;
    penev.phase_id    = next_id;
    penev.previous_id = exiting_id;
    ctx.event_bus().publish(penev);

    open_next_turn(ctx);
}

void SequentialFlowController::open_next_turn(GameContext& ctx)
{
    const std::vector<ActorId> order = determine_turn_order(ctx);
    if (order.empty()) return;

    CompletionPolicy     policy;
    std::vector<ActorId> eligible;

    if (_turn_policy.allow_simultaneous_turns) {
        eligible = order;
        policy = _turn_policy.require_all_actors_to_pass
                    ? CompletionPolicy::UNTIL_ALL_PASSED
                    : CompletionPolicy::ALL_SUBMITTED;
    } else {
        const ActorId& actor = order[_current_actor_index % order.size()];
        eligible = {actor};
        policy = _turn_policy.allow_multiple_actions_per_turn
                    ? CompletionPolicy::MANUAL_CLOSE
                    : CompletionPolicy::ANY_SUBMITTED;
    }

    const TurnId tid = "round_" + std::to_string(_round_index)
                     + "_turn_" + std::to_string(_current_actor_index + 1);
    ctx.set_current_turn_id(tid);

    TurnStartedEvent tev;
    tev.turn_id       = tid;
    tev.active_actors = eligible;
    ctx.event_bus().publish(tev);

    _current_window = std::make_unique<ActionWindow>(eligible, policy);

    WindowOpenedEvent wev;
    wev.eligible_actors = eligible;
    ctx.event_bus().publish(wev);
}

} // namespace gmFlow

