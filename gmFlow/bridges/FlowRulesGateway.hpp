#ifndef GMFLOW_FLOWRULESGATEWAY_HPP
#define GMFLOW_FLOWRULESGATEWAY_HPP

/**
 * @file bridges/FlowRulesGateway.hpp
 * @brief Callback-based bridge between gmFlow lifecycle events and an external
 *        rules engine.
 *
 * This file defines the **event contract** between gmFlow and any rules engine
 * (e.g. gmRules) that wants to react to lifecycle events without depending on
 * gmFlow's concrete classes.
 *
 * ### Design
 * - gmFlow publishes lifecycle events on its `EventBus`.
 * - `register_flow_rules_gateway()` subscribes to those events and translates
 *   each one into a `FlowRulesPayload`, then invokes the caller-supplied
 *   `FlowRulesCallback`.
 * - The callbacks are pure functions from the framework's perspective: they
 *   receive a payload snapshot and return a bool (true = allow / false = block).
 *   The bool is **informational** for lifecycle events; the action-blocking
 *   semantic is handled by @ref ActionGateway which runs the pre-check inside
 *   `IAction::validate()`.
 * - If a callback pointer is `nullptr` the corresponding subscription is
 *   silently skipped.
 * - gmRules is **not** a compile-time dependency of this header.  The caller
 *   implements the callbacks and injects the gmRules call inside them.
 *
 * ### Usage
 * @code
 *   gmFlow::register_flow_rules_gateway(
 *       session.event_bus(),
 *       // on_turn_started
 *       [&engine](const gmFlow::FlowRulesPayload& p) -> bool {
 *           return engine.notify_turn_started(p.actor_id, p.turn_id);
 *       },
 *       // on_turn_ended
 *       [&engine](const gmFlow::FlowRulesPayload& p) -> bool {
 *           engine.notify_turn_ended(p.actor_id);
 *           return true;
 *       },
 *       nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
 *       nullptr, nullptr);
 * @endcode
 *
 * @see ActionGateway
 * @see EventBus
 */

#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/FlowEvents.hpp"
#include "gmFlow/events/EventType.hpp"

#include <functional>
#include <string>

namespace gmFlow {

// ── FlowRulesPayload ──────────────────────────────────────────────────────────

/**
 * @struct FlowRulesPayload
 * @brief Minimal, stable snapshot passed to every rules-gateway callback.
 *
 * All fields are plain strings for maximum portability across versions of gmRules.
 * Fields that are not applicable to a specific event type are left empty.
 *
 * @note The @p scope_prefix field is empty when the event originates from the
 *       root `GameContext`.  It is non-empty only when the event is published by
 *       an inner controller operating on a @ref PhaseContext (e.g. `"epoch_1"`).
 */
struct FlowRulesPayload
{
	std::string actor_id;     ///< Actor involved in the event (empty if not applicable).
	std::string action_id;    ///< Action ID (empty if not an action event).
	std::string phase_id;     ///< Current phase ID from the publishing context.
	std::string round_id;     ///< Current round ID from the publishing context.
	std::string turn_id;      ///< Current turn ID from the publishing context.
	std::string scope_prefix; ///< PhaseContext scope prefix; empty at root level.
	std::string event_type;   ///< The EVT_* constant that triggered this payload.
};

// ── FlowRulesCallback ─────────────────────────────────────────────────────────

/**
 * @brief Callback signature for a rules-gateway lifecycle hook.
 *
 * The callback receives a @ref FlowRulesPayload snapshot and returns a boolean:
 * - @c true  — the event is accepted / the action is allowed.
 * - @c false — the event is rejected / the action is blocked.
 *
 * For lifecycle events (turn/round/phase/window) the return value is
 * **informational**: the flow engine does not change behaviour based on it.
 * For action pre-checks the return value of `false` is translated into a
 * `ValidationResult::fail(RULE_VIOLATION, …)` by @ref ActionGateway.
 */
using FlowRulesCallback = std::function<bool(const FlowRulesPayload&)>;

// ── register_flow_rules_gateway ───────────────────────────────────────────────

/**
 * @brief Subscribes optional callbacks to gmFlow lifecycle events.
 *
 * Each non-null callback is subscribed to the corresponding event on
 * @p event_bus.  When the event fires, the callback receives a
 * @ref FlowRulesPayload populated from the concrete event struct.
 *
 * Passing @c nullptr for any callback silently skips that subscription.
 *
 * @note This function does not throw and does not store any state itself.
 *       All state resides in the @p event_bus subscriptions.
 *
 * @param event_bus          The session EventBus to subscribe on.
 * @param on_turn_started    Called on @ref EVT_TURN_STARTED.
 * @param on_turn_ended      Called on @ref EVT_TURN_ENDED.
 * @param on_round_started   Called on @ref EVT_ROUND_STARTED.
 * @param on_round_ended     Called on @ref EVT_ROUND_ENDED.
 * @param on_phase_entered   Called on @ref EVT_PHASE_ENTERED.
 * @param on_phase_exited    Called on @ref EVT_PHASE_EXITED.
 * @param on_window_opened   Called on @ref EVT_WINDOW_OPENED.
 * @param on_window_closed   Called on @ref EVT_WINDOW_CLOSED.
 * @param on_action_submitted Called on @ref EVT_ACTION_SUBMITTED (pre-check).
 * @param on_action_completed Called on @ref EVT_ACTION_COMPLETED (post-check).
 */
void register_flow_rules_gateway(
	EventBus&          event_bus,
	FlowRulesCallback  on_turn_started,
	FlowRulesCallback  on_turn_ended,
	FlowRulesCallback  on_round_started,
	FlowRulesCallback  on_round_ended,
	FlowRulesCallback  on_phase_entered,
	FlowRulesCallback  on_phase_exited,
	FlowRulesCallback  on_window_opened,
	FlowRulesCallback  on_window_closed,
	FlowRulesCallback  on_action_submitted,
	FlowRulesCallback  on_action_completed);

} // namespace gmFlow

#endif // GMFLOW_FLOWRULESGATEWAY_HPP
