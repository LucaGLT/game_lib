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
    std::vector<std::unique_ptr<IPhase>> phases)
    : phases_(std::move(phases))
{
    if (phases_.empty()) {
        throw std::invalid_argument(
            "SequentialFlowController: phases list must not be empty");
    }
}

void SequentialFlowController::start(GameContext& ctx)
{
    // TODO: Phase 4.6 — full implementation; stubs below preserve compile-ability.
    current_phase_index_ = 0;
    current_actor_index_ = 0;
    round_index_         = 1;
    session_complete_    = false;

    ctx.set_current_phase_id(phases_[0]->id());
    phases_[0]->on_enter(ctx);

    SessionStartedEvent ev;
    ev.session_id = ctx.session_id();
    ctx.event_bus().publish(ev);

    open_next_turn(ctx);
}

void SequentialFlowController::process(GameContext& ctx)
{
    // TODO: Phase 4.6 — process action queue, advance window/turn/phase/round.
    if (session_complete_) return;

    if (!current_window_) return;

    if (!current_window_->is_complete(ctx)) return;

    current_window_->resolve(ctx);

    IPhase& phase = *phases_[current_phase_index_];
    if (phase.is_complete(ctx)) {
        advance_phase(ctx);
    } else {
        open_next_turn(ctx);
    }
}

bool SequentialFlowController::can_actor_act(const GameContext& /*ctx*/,
                                             const ActorId& actor) const
{
    // TODO: Phase 4.6 — check if actor is in the current window's eligible list.
    if (!current_window_ || current_window_->is_closed()) return false;
    return current_window_->can_submit(actor);
}

void SequentialFlowController::on_action_completed(GameContext& /*ctx*/,
                                                   const ActionResult& /*result*/)
{
    // TODO: Phase 4.6 — react to action completion (trigger follow-ups, etc.).
}

bool SequentialFlowController::is_session_complete(const GameContext& /*ctx*/) const
{
    return session_complete_;
}

std::vector<ActorId> SequentialFlowController::determine_turn_order(
    const GameContext& ctx) const
{
    // Default: registry insertion order.
    return ctx.actor_registry().all_ids();
}

void SequentialFlowController::advance_phase(GameContext& ctx)
{
    // TODO: Phase 4.6 — emit phase exit/enter events, handle round wrapping.
    phases_[current_phase_index_]->on_exit(ctx);

    ++current_phase_index_;
    current_actor_index_ = 0;

    if (current_phase_index_ >= phases_.size()) {
        session_complete_ = true;
        SessionCompletedEvent ev;
        ev.session_id = ctx.session_id();
        ctx.event_bus().publish(ev);
        return;
    }

    ctx.set_current_phase_id(phases_[current_phase_index_]->id());
    phases_[current_phase_index_]->on_enter(ctx);
    open_next_turn(ctx);
}

void SequentialFlowController::open_next_turn(GameContext& ctx)
{
    // TODO: Phase 4.6 — use TurnPolicy to decide window type (simultaneous vs sequential).
    const std::vector<ActorId> order = determine_turn_order(ctx);
    if (order.empty()) return;

    const ActorId& actor = order[current_actor_index_ % order.size()];
    const TurnId   tid   = "round_" + std::to_string(round_index_)
                         + "_turn_" + std::to_string(current_actor_index_ + 1);

    ctx.set_current_turn_id(tid);

    TurnStartedEvent tev;
    tev.turn_id       = tid;
    tev.active_actors = {actor};
    ctx.event_bus().publish(tev);

    current_window_ = std::make_unique<ActionWindow>(
        std::vector<ActorId>{actor},
        CompletionPolicy::MANUAL_CLOSE);

    WindowOpenedEvent wev;
    wev.eligible_actors = {actor};
    ctx.event_bus().publish(wev);
}

} // namespace gmFlow
