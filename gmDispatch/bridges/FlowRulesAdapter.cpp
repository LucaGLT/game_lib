/**
 * @file bridges/FlowRulesAdapter.cpp
 * @brief Implementation of FlowRulesAdapter.
 */

#include "FlowRulesAdapter.hpp"

#include "gmFlow/events/EventType.hpp"

namespace gmDispatch {

FlowRulesAdapter::FlowRulesAdapter(gmFlow::EventBus&    event_bus,
								gmRules::RuleContext& rule_context,
								const std::string&   bus_name)
	: _event_bus(event_bus)
	, _rule_context(rule_context)
	, _bus_name(bus_name)
	, _attached(false)
{}

FlowRulesAdapter::~FlowRulesAdapter()
{
	detach();
}

// ── attach / detach ───────────────────────────────────────────────────────────

void FlowRulesAdapter::attach()
{
	if (_attached) return;

	using std::placeholders::_1;

	_event_bus.subscribe(gmFlow::EVT_SESSION_STARTED,
		[this](const gmFlow::IEvent& e) { on_session_started(e); });

	_event_bus.subscribe(gmFlow::EVT_SESSION_COMPLETED,
		[this](const gmFlow::IEvent& e) { on_session_completed(e); });

	_event_bus.subscribe(gmFlow::EVT_PHASE_ENTERED,
		[this](const gmFlow::IEvent& e) { on_phase_entered(e); });

	_event_bus.subscribe(gmFlow::EVT_PHASE_EXITED,
		[this](const gmFlow::IEvent& e) { on_phase_exited(e); });

	_event_bus.subscribe(gmFlow::EVT_ROUND_STARTED,
		[this](const gmFlow::IEvent& e) { on_round_started(e); });

	_event_bus.subscribe(gmFlow::EVT_ROUND_ENDED,
		[this](const gmFlow::IEvent& e) { on_round_ended(e); });

	_event_bus.subscribe(gmFlow::EVT_TURN_STARTED,
		[this](const gmFlow::IEvent& e) { on_turn_started(e); });

	_event_bus.subscribe(gmFlow::EVT_TURN_ENDED,
		[this](const gmFlow::IEvent& e) { on_turn_ended(e); });

	_event_bus.subscribe(gmFlow::EVT_ACTION_SUBMITTED,
		[this](const gmFlow::IEvent& e) { on_action_submitted(e); });

	_event_bus.subscribe(gmFlow::EVT_ACTION_COMPLETED,
		[this](const gmFlow::IEvent& e) { on_action_completed(e); });

	_event_bus.subscribe(gmFlow::EVT_ACTION_FAILED,
		[this](const gmFlow::IEvent& e) { on_action_failed(e); });

	_event_bus.subscribe(gmFlow::EVT_WINDOW_OPENED,
		[this](const gmFlow::IEvent& e) { on_window_opened(e); });

	_event_bus.subscribe(gmFlow::EVT_WINDOW_CLOSED,
		[this](const gmFlow::IEvent& e) { on_window_closed(e); });

	_attached = true;
}

void FlowRulesAdapter::detach()
{
	// EventBus unsubscribes all handlers with a given type on request.
	// If the bus supports named subscription tokens, use them; otherwise
	// EventBus::unsubscribe_all() clears all subscriptions for this session.
	// Here we simply mark detached — the EventBus lifetime controls cleanup.
	_attached = false;
}

bool FlowRulesAdapter::is_attached() const
{
	return _attached;
}

// ── emit_trigger ──────────────────────────────────────────────────────────────

void FlowRulesAdapter::emit_trigger(const std::string& type,
								const std::string& source_id,
								const std::string& target_id,
								const std::string& payload_json)
{
	gmRules::RuleEvent ev;
	ev.type         = type;
	ev.source_id    = source_id;
	ev.target_id    = target_id;
	ev.payload_json = payload_json;
	ev.priority     = FLOW_TRIGGER_PRIORITY;

	_rule_context.emit_event(ev, _bus_name);
}

// ── Session handlers ──────────────────────────────────────────────────────────

void FlowRulesAdapter::on_session_started(const gmFlow::IEvent& e)
{
	const gmFlow::SessionStartedEvent& ev =
		static_cast<const gmFlow::SessionStartedEvent&>(e);
	emit_trigger("gmFlow.session.started", ev.session_id);
}

void FlowRulesAdapter::on_session_completed(const gmFlow::IEvent& e)
{
	const gmFlow::SessionCompletedEvent& ev =
		static_cast<const gmFlow::SessionCompletedEvent&>(e);
	emit_trigger("gmFlow.session.completed", ev.session_id);
}

// ── Phase handlers ────────────────────────────────────────────────────────────

void FlowRulesAdapter::on_phase_entered(const gmFlow::IEvent& e)
{
	const gmFlow::PhaseEnteredEvent& ev =
		static_cast<const gmFlow::PhaseEnteredEvent&>(e);
	emit_trigger("gmFlow.phase.entered",
		ev.phase_id,
		ev.previous_id,
		"{\"previous\":\"" + ev.previous_id + "\"}");
}

void FlowRulesAdapter::on_phase_exited(const gmFlow::IEvent& e)
{
	const gmFlow::PhaseExitedEvent& ev =
		static_cast<const gmFlow::PhaseExitedEvent&>(e);
	emit_trigger("gmFlow.phase.exited",
		ev.phase_id,
		ev.next_id,
		"{\"next\":\"" + ev.next_id + "\"}");
}

// ── Round handlers ────────────────────────────────────────────────────────────

void FlowRulesAdapter::on_round_started(const gmFlow::IEvent& e)
{
	const gmFlow::RoundStartedEvent& ev =
		static_cast<const gmFlow::RoundStartedEvent&>(e);
	emit_trigger("gmFlow.round.started",
		ev.round_id,
		"",
		"{\"index\":" + std::to_string(ev.index) + "}");
}

void FlowRulesAdapter::on_round_ended(const gmFlow::IEvent& e)
{
	const gmFlow::RoundEndedEvent& ev =
		static_cast<const gmFlow::RoundEndedEvent&>(e);
	emit_trigger("gmFlow.round.ended",
		ev.round_id,
		"",
		"{\"index\":" + std::to_string(ev.index) + "}");
}

// ── Turn handlers ─────────────────────────────────────────────────────────────

void FlowRulesAdapter::on_turn_started(const gmFlow::IEvent& e)
{
	const gmFlow::TurnStartedEvent& ev =
		static_cast<const gmFlow::TurnStartedEvent&>(e);

	// Emit one trigger per active actor so rules can be scoped per actor.
	if (ev.active_actors.empty())
	{
		emit_trigger("gmFlow.turn.started", ev.turn_id);
		return;
	}
	for (const std::string& actor_id : ev.active_actors)
	{
		emit_trigger("gmFlow.turn.started", actor_id, "", "{\"turn_id\":\"" + ev.turn_id + "\"}");
	}
}

void FlowRulesAdapter::on_turn_ended(const gmFlow::IEvent& e)
{
	const gmFlow::TurnEndedEvent& ev =
		static_cast<const gmFlow::TurnEndedEvent&>(e);
	emit_trigger("gmFlow.turn.ended", ev.turn_id);
}

// ── Action handlers ───────────────────────────────────────────────────────────

void FlowRulesAdapter::on_action_submitted(const gmFlow::IEvent& e)
{
	const gmFlow::ActionSubmittedEvent& ev =
		static_cast<const gmFlow::ActionSubmittedEvent&>(e);
	emit_trigger("gmFlow.action.submitted",
		ev.actor_id,
		"",
		"{\"action_id\":\"" + ev.action_id + "\"}");
}

void FlowRulesAdapter::on_action_completed(const gmFlow::IEvent& e)
{
	const gmFlow::ActionCompletedEvent& ev =
		static_cast<const gmFlow::ActionCompletedEvent&>(e);
	emit_trigger("gmFlow.action.completed",
		ev.actor_id,
		"",
		"{\"action_id\":\"" + ev.action_id + "\"}");
}

void FlowRulesAdapter::on_action_failed(const gmFlow::IEvent& e)
{
	const gmFlow::ActionFailedEvent& ev =
		static_cast<const gmFlow::ActionFailedEvent&>(e);
	emit_trigger("gmFlow.action.failed",
		ev.actor_id,
		"",
		"{\"action_id\":\"" + ev.action_id + "\",\"reason\":\"" + ev.reason + "\"}");
}

// ── Window handlers ───────────────────────────────────────────────────────────

void FlowRulesAdapter::on_window_opened(const gmFlow::IEvent& e)
{
	const gmFlow::WindowOpenedEvent& ev =
		static_cast<const gmFlow::WindowOpenedEvent&>(e);

	if (ev.eligible_actors.empty())
	{
		emit_trigger("gmFlow.window.opened", "");
		return;
	}
	for (const std::string& actor_id : ev.eligible_actors)
	{
		emit_trigger("gmFlow.window.opened", actor_id);
	}
}

void FlowRulesAdapter::on_window_closed(const gmFlow::IEvent& /*e*/)
{
	emit_trigger("gmFlow.window.closed", "");
}

} // namespace gmDispatch
