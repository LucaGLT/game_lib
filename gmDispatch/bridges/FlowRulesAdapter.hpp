#ifndef GMDISPATCH_FLOWRULESADAPTER_HPP
#define GMDISPATCH_FLOWRULESADAPTER_HPP

/**
 * @file bridges/FlowRulesAdapter.hpp
 * @brief Adapter that translates gmFlow lifecycle events into gmRules trigger events.
 *
 * FlowRulesAdapter subscribes to an EventBus and, for each lifecycle event
 * it receives, fires the corresponding gmRules trigger by emitting a
 * RuleEvent through a RuleContext.  This allows rules authored in gmRules
 * to react to turn/round/action lifecycle without gmRules depending on gmFlow.
 *
 * ## Dependency direction (correct)
 * @code
 *   gmFlow <── FlowRulesAdapter ──> gmRules
 *              (in gmDispatch/bridges/)
 * @endcode
 *
 * ## Lifecycle events translated
 * | gmFlow event              | RuleEvent.type emitted                |
 * |---------------------------|---------------------------------------|
 * | EVT_TURN_STARTED          | gmFlow.turn.started                   |
 * | EVT_TURN_ENDED            | gmFlow.turn.ended                     |
 * | EVT_ROUND_STARTED         | gmFlow.round.started                  |
 * | EVT_ROUND_ENDED           | gmFlow.round.ended                    |
 * | EVT_PHASE_ENTERED         | gmFlow.phase.entered                  |
 * | EVT_PHASE_EXITED          | gmFlow.phase.exited                   |
 * | EVT_ACTION_SUBMITTED      | gmFlow.action.submitted               |
 * | EVT_ACTION_COMPLETED      | gmFlow.action.completed               |
 * | EVT_ACTION_FAILED         | gmFlow.action.failed                  |
 * | EVT_WINDOW_OPENED         | gmFlow.window.opened                  |
 * | EVT_WINDOW_CLOSED         | gmFlow.window.closed                  |
 * | EVT_SESSION_STARTED       | gmFlow.session.started                |
 * | EVT_SESSION_COMPLETED     | gmFlow.session.completed              |
 *
 * ## Usage
 * @code
 *   FlowRulesAdapter adapter(event_bus, rule_context);
 *   adapter.attach(); // subscribes all handlers
 *   // ... game runs ...
 *   adapter.detach(); // unsubscribes all handlers
 * @endcode
 */

#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/FlowEvents.hpp"
#include "gmRules/core/RuleContext.hpp"
#include "gmRules/core/RuleEvent.hpp"

#include <string>
#include <vector>

namespace gmDispatch {

/**
 * @brief Priority assigned to lifecycle trigger events emitted by FlowRulesAdapter.
 *
 * Lifecycle events have high priority (low numeric value) so that reactive
 * rules are resolved before normal-priority game effects.
 */
static constexpr int FLOW_TRIGGER_PRIORITY = 50;

/**
 * @class FlowRulesAdapter
 * @brief Bridges gmFlow lifecycle events to gmRules trigger events.
 *
 * One instance per session.  Attach at session start, detach at session end.
 */
class FlowRulesAdapter
{
public:
	/**
	 * @brief Constructs the adapter.
	 * @param event_bus    gmFlow EventBus to subscribe on.
	 * @param rule_context gmRules RuleContext to emit trigger events through.
	 * @param bus_name     Name of the gmDispatch bus used for trigger events.
	 *                     Defaults to "RuleEvBus".
	 */
	FlowRulesAdapter(gmFlow::EventBus&    event_bus,
				 gmRules::RuleContext& rule_context,
				 const std::string&   bus_name = "RuleEvBus");

	~FlowRulesAdapter();

	// Non-copyable, non-movable.
	FlowRulesAdapter(const FlowRulesAdapter&)            = delete;
	FlowRulesAdapter& operator=(const FlowRulesAdapter&) = delete;
	FlowRulesAdapter(FlowRulesAdapter&&)                 = delete;
	FlowRulesAdapter& operator=(FlowRulesAdapter&&)      = delete;

	/**
	 * @brief Subscribes all lifecycle handlers to the EventBus.
	 *
	 * Safe to call multiple times — subsequent calls are no-ops after the
	 * first successful attach.
	 */
	void attach();

	/**
	 * @brief Unsubscribes all handlers from the EventBus.
	 *
	 * Safe to call when not attached.
	 */
	void detach();

	/** @brief Returns true if handlers are currently subscribed. */
	bool is_attached() const;

private:
	// ── Handlers ────────────────────────────────────────────────────────────

	void on_session_started(const gmFlow::IEvent& e);
	void on_session_completed(const gmFlow::IEvent& e);
	void on_phase_entered(const gmFlow::IEvent& e);
	void on_phase_exited(const gmFlow::IEvent& e);
	void on_round_started(const gmFlow::IEvent& e);
	void on_round_ended(const gmFlow::IEvent& e);
	void on_turn_started(const gmFlow::IEvent& e);
	void on_turn_ended(const gmFlow::IEvent& e);
	void on_action_submitted(const gmFlow::IEvent& e);
	void on_action_completed(const gmFlow::IEvent& e);
	void on_action_failed(const gmFlow::IEvent& e);
	void on_window_opened(const gmFlow::IEvent& e);
	void on_window_closed(const gmFlow::IEvent& e);

	// ── Helper ───────────────────────────────────────────────────────────────

	/**
	 * @brief Builds and emits a RuleEvent from the flow event fields.
	 * @param type           RuleEvent type string.
	 * @param source_id      Source actor or component identifier.
	 * @param target_id      Target actor (may be empty).
	 * @param payload_json   Optional JSON payload.
	 */
	void emit_trigger(const std::string& type,
				  const std::string& source_id,
				  const std::string& target_id  = "",
				  const std::string& payload_json = "");

	// ── Members ──────────────────────────────────────────────────────────────

	gmFlow::EventBus&    _event_bus;
	gmRules::RuleContext& _rule_context;
	std::string           _bus_name;
	bool                  _attached;
};

} // namespace gmDispatch

#endif // GMDISPATCH_FLOWRULESADAPTER_HPP
