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
    : phases_(std::move(phases))
    , turn_policy_(turn_policy)
    , round_policy_(round_policy)
{
    if (phases_.empty()) {
        throw std::invalid_argument(
            "SequentialFlowController: phases list must not be empty");
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void SequentialFlowController::start(GameContext& ctx)
{
    current_phase_index_ = 0;
    current_actor_index_ = 0;
    round_index_         = 1;
    session_complete_    = false;
    rounds_exhausted_    = false;

    // Enter first phase.
    const PhaseId first_id = phases_[0]->id();
    ctx.set_current_phase_id(first_id);
    phases_[0]->on_enter(ctx);

    PhaseEnteredEvent pev;
    pev.phase_id    = first_id;
    pev.previous_id = "";
    ctx.event_bus().publish(pev);

    // Start first round.
    if (round_policy_.enabled) {
        const RoundId rid = "round_" + std::to_string(round_index_);
        ctx.set_current_round_id(rid);

        RoundStartedEvent rev;
        rev.round_id = rid;
        rev.index    = round_index_;
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
    if (session_complete_ || rounds_exhausted_) return;
    if (!current_window_)                       return;
    if (!current_window_->is_complete(ctx))     return;

    // Resolve window: executes stored actions and emits WindowClosed.
    current_window_->resolve(ctx);
    current_window_.reset();

    // Emit TurnEnded.
    TurnEndedEvent tev;
    tev.turn_id = ctx.current_turn_id();
    ctx.event_bus().publish(tev);

    // Advance turn counter.
    const std::vector<ActorId> order = determine_turn_order(ctx);
    const std::size_t n = order.size();
    ++current_actor_index_;

    // Check if we completed one full actor cycle (= one round).
    if (n > 0 && current_actor_index_ % n == 0) {
        if (round_policy_.enabled) {
            RoundEndedEvent rev;
            rev.round_id = ctx.current_round_id();
            rev.index    = round_index_;
            ctx.event_bus().publish(rev);

            if (round_policy_.max_rounds > 0 && round_index_ >= round_policy_.max_rounds) {
                rounds_exhausted_ = true;
                return;  // is_session_complete() will now return true.
            }

            ++round_index_;
            const RoundId new_rid = "round_" + std::to_string(round_index_);
            ctx.set_current_round_id(new_rid);

            RoundStartedEvent rsev;
            rsev.round_id = new_rid;
            rsev.index    = round_index_;
            ctx.event_bus().publish(rsev);
        }
    }

    // Check if the current phase is complete.
    if (phases_[current_phase_index_]->is_complete(ctx)) {
        advance_phase(ctx);
    } else {
        open_next_turn(ctx);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

bool SequentialFlowController::can_actor_act(const GameContext& /*ctx*/,
                                             const ActorId& actor) const
{
    if (!current_window_ || current_window_->is_closed()) return false;
    return current_window_->can_submit(actor);
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
    if (!current_window_) {
        return ValidationResult::fail(
            ValidationError::ACTION_WINDOW_CLOSED,
            "No active action window.");
    }
    return current_window_->submit(actor, std::move(action));
}

bool SequentialFlowController::is_session_complete(const GameContext& /*ctx*/) const
{
    return session_complete_ || rounds_exhausted_;
}

// ─────────────────────────────────────────────────────────────────────────────

std::vector<ActorId> SequentialFlowController::determine_turn_order(
    const GameContext& ctx) const
{
    return ctx.actor_registry().all_ids();
}

void SequentialFlowController::advance_phase(GameContext& ctx)
{
    const PhaseId exiting_id = phases_[current_phase_index_]->id();
    const std::size_t next_idx = current_phase_index_ + 1;
    const PhaseId next_id = (next_idx < phases_.size())
                             ? phases_[next_idx]->id()
                             : "";

    // Announce phase exit before calling on_exit().
    PhaseExitedEvent pxev;
    pxev.phase_id = exiting_id;
    pxev.next_id  = next_id;
    ctx.event_bus().publish(pxev);

    phases_[current_phase_index_]->on_exit(ctx);
    ++current_phase_index_;
    current_actor_index_ = 0;

    if (current_phase_index_ >= phases_.size()) {
        session_complete_ = true;
        SessionCompletedEvent sev;
        sev.session_id = ctx.session_id();
        ctx.event_bus().publish(sev);
        return;
    }

    // Enter the next phase.
    ctx.set_current_phase_id(next_id);
    phases_[current_phase_index_]->on_enter(ctx);

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

    if (turn_policy_.allow_simultaneous_turns) {
        eligible = order;
        policy = turn_policy_.require_all_actors_to_pass
                    ? CompletionPolicy::UNTIL_ALL_PASSED
                    : CompletionPolicy::ALL_SUBMITTED;
    } else {
        const ActorId& actor = order[current_actor_index_ % order.size()];
        eligible = {actor};
        policy = turn_policy_.allow_multiple_actions_per_turn
                    ? CompletionPolicy::MANUAL_CLOSE
                    : CompletionPolicy::ANY_SUBMITTED;
    }

    const TurnId tid = "round_" + std::to_string(round_index_)
                     + "_turn_" + std::to_string(current_actor_index_ + 1);
    ctx.set_current_turn_id(tid);

    TurnStartedEvent tev;
    tev.turn_id       = tid;
    tev.active_actors = eligible;
    ctx.event_bus().publish(tev);

    current_window_ = std::make_unique<ActionWindow>(eligible, policy);

    WindowOpenedEvent wev;
    wev.eligible_actors = eligible;
    ctx.event_bus().publish(wev);
}

} // namespace gmFlow

