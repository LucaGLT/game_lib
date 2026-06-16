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
// EGameSessionError
// ─────────────────────────────────────────────────────────────────────────────

EGameSessionError::EGameSessionError(const std::string& message)
    : std::runtime_error("GameSession: " + message)
{}

// ─────────────────────────────────────────────────────────────────────────────
// GameSession
// ─────────────────────────────────────────────────────────────────────────────

GameSession::GameSession(SessionConfig                           config,
                         std::unique_ptr<IFlowController>        flow_controller,
                         std::unique_ptr<GameState>              state,
				 std::shared_ptr<gmDispatch::GmDispatcher> dispatcher)
    : _config(std::move(config))
    , _game_state(std::move(state))
    , _event_bus(std::move(dispatcher))
    , _context(_config.session_id, *_game_state, _actor_registry, _event_bus)
    , _flow_controller(std::move(flow_controller))
{
    if (!_flow_controller) {
        throw std::invalid_argument("GameSession: flow_controller must not be null");
    }
    if (!_game_state) {
        throw std::invalid_argument("GameSession: state must not be null");
    }
}

void GameSession::start()
{
    if (_session_state != SessionState::CREATED) {
        throw EGameSessionError("start() called on a session that is not in CREATED state");
    }

    // Populate actor registry from config.
    for (Actor actor : _config.actors) {
        _actor_registry.add(std::move(actor));
    }

    // Notify state.
    _game_state->on_session_started(_config.session_id);

    _session_state = SessionState::RUNNING;

    // TODO: Phase 4.7 — log session start via gmLog
    _flow_controller->start(_context);
}

void GameSession::tick()
{
    if (_session_state != SessionState::RUNNING) {
        throw EGameSessionError("tick() called on a session that is not RUNNING");
    }

    // Drain the deferred/reaction action queue.
    while (!_action_queue.empty()) {
        IAction& action = _action_queue.front();
        const ActionResult result = action.execute(_context);
        _action_queue.pop();
        _flow_controller->on_action_completed(_context, result);
    }

    // Advance flow state.
    _flow_controller->process(_context);

    if (_flow_controller->is_session_complete(_context)) {
        _session_state = SessionState::COMPLETED;
        _game_state->on_session_completed();
        // EVT_SESSION_COMPLETED is published by the flow controller.
    }
}

void GameSession::pause()
{
    if (_session_state != SessionState::RUNNING) {
        throw EGameSessionError("pause() called on a session that is not RUNNING");
    }
    _session_state = SessionState::PAUSED;

    // TODO: Phase 4.7 — serialise state snapshot via gmSave.
    SessionPausedEvent ev;
    ev.session_id = _config.session_id;
    _event_bus.publish(ev);
}

void GameSession::resume()
{
    if (_session_state != SessionState::PAUSED) {
        throw EGameSessionError("resume() called on a session that is not PAUSED");
    }
    // TODO: Phase 4.7 — restore state snapshot via gmSave.
    _session_state = SessionState::RUNNING;

    SessionResumedEvent ev;
    ev.session_id = _config.session_id;
    _event_bus.publish(ev);
}

ValidationResult GameSession::submit_action(const ActorId&           actor,
                                             std::unique_ptr<IAction> action)
{
    if (_session_state != SessionState::RUNNING) {
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
    if (!_flow_controller->can_actor_act(_context, actor)) {
        return ValidationResult::fail(
            ValidationError::NOT_ACTOR_TURN,
            "Actor '" + actor + "' is not eligible to act right now.");
    }

    // Stage 2: game-rule validation.
    ValidationResult vr = action->validate(_context);
    if (!vr.valid()) {
        return vr;
    }

    // Accepted: publish event and delegate to the flow controller's ActionWindow.
    ActionSubmittedEvent ev;
    ev.action_id = action->id();
    ev.actor_id  = actor;
    _event_bus.publish(ev);

    // The flow controller routes the action to the current ActionWindow.
    return _flow_controller->accept_action(_context, actor, std::move(action));
}

bool GameSession::is_finished() const
{
    return _session_state == SessionState::COMPLETED
        || _session_state == SessionState::FAILED;
}

bool GameSession::is_paused() const
{
    return _session_state == SessionState::PAUSED;
}

SessionState GameSession::state() const
{
    return _session_state;
}

const GameContext& GameSession::context() const
{
    return _context;
}

EventBus& GameSession::event_bus()
{
    return _event_bus;
}

const SessionId& GameSession::session_id() const
{
    return _config.session_id;
}

} // namespace gmFlow
