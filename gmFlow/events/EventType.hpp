#ifndef GMFLOW_EVENTTYPE_HPP
#define GMFLOW_EVENTTYPE_HPP

/**
 * @file events/EventType.hpp
 * @brief String constants for all built-in gmFlow event types.
 *
 * Use these constants when subscribing to or publishing built-in events to
 * avoid error-prone raw string literals.
 *
 * @code
 *   session.event_bus().subscribe(gmFlow::EVT_TURN_STARTED,
 *       [](const gmFlow::IEvent& e) {
 *           const auto& ev = static_cast<const gmFlow::TurnStartedEvent&>(e);
 *           // handle turn start
 *       });
 * @endcode
 *
 * Game-specific code may define its own event type strings freely; the
 * `"gmFlow."` prefix is reserved for this library.
 */

namespace gmFlow {

// ── Session lifecycle ──────────────────────────────────────────────────────

/// @brief Published when a GameSession transitions to RUNNING.
inline constexpr const char* EVT_SESSION_STARTED   = "gmFlow.session.started";

/// @brief Published when a GameSession is paused via GameSession::pause().
inline constexpr const char* EVT_SESSION_PAUSED    = "gmFlow.session.paused";

/// @brief Published when a paused GameSession is resumed.
inline constexpr const char* EVT_SESSION_RESUMED   = "gmFlow.session.resumed";

/// @brief Published when the session flow controller reports is_session_complete().
inline constexpr const char* EVT_SESSION_COMPLETED = "gmFlow.session.completed";

// ── Phase lifecycle ────────────────────────────────────────────────────────

/// @brief Published immediately after IPhase::on_enter() returns.
inline constexpr const char* EVT_PHASE_ENTERED = "gmFlow.phase.entered";

/// @brief Published immediately before IPhase::on_exit() is called.
inline constexpr const char* EVT_PHASE_EXITED  = "gmFlow.phase.exited";

// ── Round lifecycle ────────────────────────────────────────────────────────

/// @brief Published when a new Round begins.
inline constexpr const char* EVT_ROUND_STARTED = "gmFlow.round.started";

/// @brief Published when a Round ends (all turns in the round are complete).
inline constexpr const char* EVT_ROUND_ENDED   = "gmFlow.round.ended";

// ── Turn lifecycle ─────────────────────────────────────────────────────────

/// @brief Published when a Turn begins (active actors are set).
inline constexpr const char* EVT_TURN_STARTED = "gmFlow.turn.started";

/// @brief Published when a Turn ends (all actors have acted or passed).
inline constexpr const char* EVT_TURN_ENDED   = "gmFlow.turn.ended";

// ── Action lifecycle ───────────────────────────────────────────────────────

/// @brief Published when an action is accepted by GameSession::submit_action().
inline constexpr const char* EVT_ACTION_SUBMITTED = "gmFlow.action.submitted";

/// @brief Published when an action passes validation.
inline constexpr const char* EVT_ACTION_VALIDATED = "gmFlow.action.validated";

/// @brief Published when an action transitions to EXECUTING.
inline constexpr const char* EVT_ACTION_STARTED   = "gmFlow.action.started";

/// @brief Published when an action finishes with COMPLETED status.
inline constexpr const char* EVT_ACTION_COMPLETED = "gmFlow.action.completed";

/// @brief Published when an action finishes with FAILED status.
inline constexpr const char* EVT_ACTION_FAILED    = "gmFlow.action.failed";

/// @brief Published when an action is explicitly cancelled.
inline constexpr const char* EVT_ACTION_CANCELLED = "gmFlow.action.cancelled";

// ── ActionWindow lifecycle ─────────────────────────────────────────────────

/// @brief Published when an ActionWindow opens (eligible actors may now act).
inline constexpr const char* EVT_WINDOW_OPENED = "gmFlow.window.opened";

/// @brief Published when an ActionWindow closes (no more submissions accepted).
inline constexpr const char* EVT_WINDOW_CLOSED = "gmFlow.window.closed";

// ── Campaign lifecycle ─────────────────────────────────────────────────────

/// @brief Published when a campaign session becomes available to start.
inline constexpr const char* EVT_CAMPAIGN_SESSION_UNLOCKED = "gmFlow.campaign.session_unlocked";

/// @brief Published when all campaign sessions have been completed.
inline constexpr const char* EVT_CAMPAIGN_COMPLETED = "gmFlow.campaign.completed";

// ── Timeline lifecycle ─────────────────────────────────────────────────────

/// @brief Published when the controller selects the next active timeline actor.
inline constexpr const char* EVT_TIMELINE_ACTOR_SELECTED =
    "gmFlow.timeline.actor_selected";

/// @brief Published when the minimum timeline position advances.
inline constexpr const char* EVT_TIMELINE_TIME_ADVANCED =
    "gmFlow.timeline.time_advanced";

/// @brief Published when two or more actors share the same lowest timeline position.
inline constexpr const char* EVT_TIMELINE_TIE_DETECTED =
    "gmFlow.timeline.tie_detected";

/// @brief Published when no enabled actor is available for selection.
inline constexpr const char* EVT_TIMELINE_NO_ACTOR_AVAILABLE =
    "gmFlow.timeline.no_actor_available";

} // namespace gmFlow

#endif // GMFLOW_EVENTTYPE_HPP
