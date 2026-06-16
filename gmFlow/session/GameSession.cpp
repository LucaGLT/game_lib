/**
 * @file session/GameSession.cpp
 * @brief Implementation of gmFlow::GameSession.
 */

#include "gmFlow/session/GameSession.hpp"
#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/events/FlowEvents.hpp"

#include <stdexcept>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// GameSessionError
// ─────────────────────────────────────────────────────────────────────────────

GameSessionError::GameSessionError(const std::string& message)
    : std::runtime_error("GameSession: " + message)
{}

// ─────────────────────────────────────────────────────────────────────────────
// GameSession
// ─────────────────────────────────────────────────────────────────────────────

GameSession::GameSession(SessionConfig                           config,
                         std::unique_ptr<IFlowController>        flow_controller,
                         std::unique_ptr<GameState>              state,
				 std::shared_ptr<gmDispatch::GmDispatcher> dispatcher)
    : config_(std::move(config))
    , game_state_(std::move(state))
    , event_bus_(std::move(dispatcher))
    , context_(config_.session_id, *game_state_, actor_registry_, event_bus_)
    , flow_controller_(std::move(flow_controller))
{
    if (!flow_controller_) {
        throw std::invalid_argument("GameSession: flow_controller must not be null");
    }
    if (!game_state_) {
        throw std::invalid_argument("GameSession: state must not be null");
    }
}

void GameSession::start()
{
    if (session_state_ != SessionState::CREATED) {
        throw GameSessionError("start() called on a session that is not in CREATED state");
    }

    // Populate actor registry from config.
    for (Actor actor : config_.actors) {
        actor_registry_.add(std::move(actor));
    }

    // Notify state.
    game_state_->on_session_started(config_.session_id);

    session_state_ = SessionState::RUNNING;

    // TODO: Phase 4.7 — log session start via gmLog
    flow_controller_->start(context_);
}

void GameSession::tick()
{
    if (session_state_ != SessionState::RUNNING) {
        throw GameSessionError("tick() called on a session that is not RUNNING");
    }

    // Drain the deferred/reaction action queue.
    // Normal turn actions are stored inside the ActionWindow and executed
    // by ActionWindow::resolve() which is called from process() below.
    while (!action_queue_.empty()) {
        IAction& action = action_queue_.front();
        const ActionResult result = action.execute(context_);
        action_queue_.pop();
        flow_controller_->on_action_completed(context_, result);
    }

    // Advance flow state (checks window completion, advances turn/phase/round).
    flow_controller_->process(context_);

    if (flow_controller_->is_session_complete(context_)) {
        session_state_ = SessionState::COMPLETED;
        game_state_->on_session_completed();
        // EVT_SESSION_COMPLETED is published by the flow controller.
    }
}

void GameSession::pause()
{
    if (session_state_ != SessionState::RUNNING) {
        throw GameSessionError("pause() called on a session that is not RUNNING");
    }
    session_state_ = SessionState::PAUSED;

    // TODO: Phase 4.7 — serialise state snapshot via gmSave.
    SessionPausedEvent ev;
    ev.session_id = config_.session_id;
    event_bus_.publish(ev);
}

void GameSession::resume()
{
    if (session_state_ != SessionState::PAUSED) {
        throw GameSessionError("resume() called on a session that is not PAUSED");
    }
    // TODO: Phase 4.7 — restore state snapshot via gmSave.
    session_state_ = SessionState::RUNNING;

    SessionResumedEvent ev;
    ev.session_id = config_.session_id;
    event_bus_.publish(ev);
}

ValidationResult GameSession::submit_action(const ActorId&           actor,
                                             std::unique_ptr<IAction> action)
{
    if (session_state_ != SessionState::RUNNING) {
        return ValidationResult::fail(
            ValidationError::ACTION_WINDOW_CLOSED,
            "Session is not running.");
    }
    if (!action) {
        return ValidationResult::fail(
            ValidationError::RULE_VIOLATION,
            "Action must not be null.");
    }

    // Stage 1: turn/window eligibility.
    if (!flow_controller_->can_actor_act(context_, actor)) {
        return ValidationResult::fail(
            ValidationError::NOT_ACTOR_TURN,
            "Actor '" + actor + "' is not eligible to act right now.");
    }

    // Stage 2: game-rule validation.
    ValidationResult vr = action->validate(context_);
    if (!vr.valid()) {
        return vr;
    }

    // Accepted: publish event and delegate to the flow controller's ActionWindow.
    ActionSubmittedEvent ev;
    ev.action_id = action->id();
    ev.actor_id  = actor;
    event_bus_.publish(ev);

    // The flow controller routes the action to the current ActionWindow.
    return flow_controller_->accept_action(context_, actor, std::move(action));
}

bool GameSession::is_finished() const
{
    return session_state_ == SessionState::COMPLETED
        || session_state_ == SessionState::FAILED;
}

bool GameSession::is_paused() const
{
    return session_state_ == SessionState::PAUSED;
}

SessionState GameSession::state() const
{
    return session_state_;
}

const GameContext& GameSession::context() const
{
    return context_;
}

EventBus& GameSession::event_bus()
{
    return event_bus_;
}

const SessionId& GameSession::session_id() const
{
    return config_.session_id;
}

} // namespace gmFlow
