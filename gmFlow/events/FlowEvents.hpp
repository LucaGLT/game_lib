#ifndef GMFLOW_FLOWEVENTS_HPP
#define GMFLOW_FLOWEVENTS_HPP

/**
 * @file events/FlowEvents.hpp
 * @brief Concrete event structs for all built-in gmFlow lifecycle events.
 *
 * All structs in this file inherit from @ref IEvent and correspond to one of
 * the event type constants declared in @ref EventType.hpp.
 *
 * ### Subscribing to a built-in event
 * @code
 *   session.event_bus().subscribe(gmFlow::EVT_TURN_STARTED,
 *       [](const gmFlow::IEvent& e) {
 *           const auto& ev = static_cast<const gmFlow::TurnStartedEvent&>(e);
 *           // ev.turn_id, ev.active_actors are populated
 *       });
 * @endcode
 *
 * All event structs are **non-owning value types**: they capture IDs and
 * lightweight state snapshots, never pointers or references to live objects.
 */

#include "gmFlow/events/IEvent.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"

#include <vector>
#include <string>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// Session events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when a GameSession transitions to RUNNING.
struct SessionStartedEvent : public IEvent {
    EventType type() const override { return EVT_SESSION_STARTED; }
    SessionId session_id; ///< ID of the session that started.
};

/// @brief Published when a GameSession is paused via GameSession::pause().
struct SessionPausedEvent : public IEvent {
    EventType type() const override { return EVT_SESSION_PAUSED; }
    SessionId session_id; ///< ID of the paused session.
};

/// @brief Published when a paused GameSession is resumed.
struct SessionResumedEvent : public IEvent {
    EventType type() const override { return EVT_SESSION_RESUMED; }
    SessionId session_id; ///< ID of the resumed session.
};

/// @brief Published when the session flow controller reports is_session_complete().
struct SessionCompletedEvent : public IEvent {
    EventType type() const override { return EVT_SESSION_COMPLETED; }
    SessionId session_id; ///< ID of the completed session.
};

// ─────────────────────────────────────────────────────────────────────────────
// Phase events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published immediately after IPhase::on_enter() returns.
struct PhaseEnteredEvent : public IEvent {
    EventType type() const override { return EVT_PHASE_ENTERED; }
    PhaseId phase_id;     ///< ID of the phase that was entered.
    PhaseId previous_id;  ///< ID of the previous phase; empty at session start.
};

/// @brief Published immediately before IPhase::on_exit() is called.
struct PhaseExitedEvent : public IEvent {
    EventType type() const override { return EVT_PHASE_EXITED; }
    PhaseId phase_id;  ///< ID of the phase being exited.
    PhaseId next_id;   ///< ID of the next phase; empty at session end.
};

// ─────────────────────────────────────────────────────────────────────────────
// Round events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when a new Round begins.
struct RoundStartedEvent : public IEvent {
    EventType type() const override { return EVT_ROUND_STARTED; }
    RoundId round_id; ///< ID of the round that started.
    int     index;    ///< 1-based round index within the session.
};

/// @brief Published when a Round ends.
struct RoundEndedEvent : public IEvent {
    EventType type() const override { return EVT_ROUND_ENDED; }
    RoundId round_id; ///< ID of the round that ended.
    int     index;    ///< 1-based round index within the session.
};

// ─────────────────────────────────────────────────────────────────────────────
// Turn events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when a Turn begins.
struct TurnStartedEvent : public IEvent {
    EventType type() const override { return EVT_TURN_STARTED; }
    TurnId               turn_id;       ///< ID of the turn that started.
    std::vector<ActorId> active_actors; ///< Actors eligible to act this turn.
};

/// @brief Published when a Turn ends.
struct TurnEndedEvent : public IEvent {
    EventType type() const override { return EVT_TURN_ENDED; }
    TurnId turn_id; ///< ID of the turn that ended.
};

// ─────────────────────────────────────────────────────────────────────────────
// Action events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when an action is accepted by GameSession::submit_action().
struct ActionSubmittedEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_SUBMITTED; }
    ActionId action_id; ///< ID of the submitted action.
    ActorId  actor_id;  ///< Actor that submitted the action.
};

/// @brief Published when an action passes validation.
struct ActionValidatedEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_VALIDATED; }
    ActionId action_id; ///< ID of the validated action.
    ActorId  actor_id;  ///< Actor that owns the action.
};

/// @brief Published when an action transitions to EXECUTING.
struct ActionStartedEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_STARTED; }
    ActionId action_id; ///< ID of the action that started executing.
    ActorId  actor_id;  ///< Actor that owns the action.
};

/// @brief Published when an action finishes with COMPLETED status.
struct ActionCompletedEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_COMPLETED; }
    ActionId action_id; ///< ID of the completed action.
    ActorId  actor_id;  ///< Actor that owns the action.
};

/// @brief Published when an action finishes with FAILED status.
struct ActionFailedEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_FAILED; }
    ActionId    action_id; ///< ID of the failed action.
    ActorId     actor_id;  ///< Actor that owns the action.
    std::string reason;    ///< Human-readable failure reason from ActionResult.
};

/// @brief Published when an action is explicitly cancelled.
struct ActionCancelledEvent : public IEvent {
    EventType type() const override { return EVT_ACTION_CANCELLED; }
    ActionId action_id; ///< ID of the cancelled action.
    ActorId  actor_id;  ///< Actor that owns the action.
};

// ─────────────────────────────────────────────────────────────────────────────
// ActionWindow events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when an ActionWindow opens.
struct WindowOpenedEvent : public IEvent {
    EventType type() const override { return EVT_WINDOW_OPENED; }
    std::vector<ActorId> eligible_actors; ///< Actors that may submit to this window.
};

/// @brief Published when an ActionWindow closes.
struct WindowClosedEvent : public IEvent {
    EventType type() const override { return EVT_WINDOW_CLOSED; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Campaign events
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Published when a campaign session becomes available to start.
struct CampaignSessionUnlockedEvent : public IEvent {
    EventType type() const override { return EVT_CAMPAIGN_SESSION_UNLOCKED; }
    SessionId session_id; ///< ID of the session that was unlocked.
};

/// @brief Published when all campaign sessions have been completed.
struct CampaignCompletedEvent : public IEvent {
    EventType type() const override { return EVT_CAMPAIGN_COMPLETED; }
};

} // namespace gmFlow

#endif // GMFLOW_FLOWEVENTS_HPP
