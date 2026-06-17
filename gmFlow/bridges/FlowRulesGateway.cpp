/**
 * @file bridges/FlowRulesGateway.cpp
 * @brief Implementation of gmFlow::register_flow_rules_gateway().
 */

#include "gmFlow/bridges/FlowRulesGateway.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/events/FlowEvents.hpp"

#include <utility>

namespace gmFlow {

namespace {

// Helper: build a payload from the base GameContext IDs stored in the event
// publishing context.  For lifecycle events the context fields we care about
// are embedded in each concrete event struct; we fill the rest with empty strings.

FlowRulesPayload make_turn_started_payload(const IEvent& e)
{
	const TurnStartedEvent& ev = static_cast<const TurnStartedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_TURN_STARTED;
	p.turn_id    = ev.turn_id;
	if (!ev.active_actors.empty())
		p.actor_id = ev.active_actors[0];
	return p;
}

FlowRulesPayload make_turn_ended_payload(const IEvent& e)
{
	const TurnEndedEvent& ev = static_cast<const TurnEndedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_TURN_ENDED;
	p.turn_id    = ev.turn_id;
	return p;
}

FlowRulesPayload make_round_started_payload(const IEvent& e)
{
	const RoundStartedEvent& ev = static_cast<const RoundStartedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_ROUND_STARTED;
	p.round_id   = ev.round_id;
	return p;
}

FlowRulesPayload make_round_ended_payload(const IEvent& e)
{
	const RoundEndedEvent& ev = static_cast<const RoundEndedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_ROUND_ENDED;
	p.round_id   = ev.round_id;
	return p;
}

FlowRulesPayload make_phase_entered_payload(const IEvent& e)
{
	const PhaseEnteredEvent& ev = static_cast<const PhaseEnteredEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_PHASE_ENTERED;
	p.phase_id   = ev.phase_id;
	return p;
}

FlowRulesPayload make_phase_exited_payload(const IEvent& e)
{
	const PhaseExitedEvent& ev = static_cast<const PhaseExitedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_PHASE_EXITED;
	p.phase_id   = ev.phase_id;
	return p;
}

FlowRulesPayload make_window_opened_payload(const IEvent& e)
{
	const WindowOpenedEvent& ev = static_cast<const WindowOpenedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_WINDOW_OPENED;
	if (!ev.eligible_actors.empty())
		p.actor_id = ev.eligible_actors[0];
	return p;
}

FlowRulesPayload make_window_closed_payload(const IEvent& /*e*/)
{
	FlowRulesPayload p;
	p.event_type = EVT_WINDOW_CLOSED;
	return p;
}

FlowRulesPayload make_action_submitted_payload(const IEvent& e)
{
	const ActionSubmittedEvent& ev = static_cast<const ActionSubmittedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_ACTION_SUBMITTED;
	p.action_id  = ev.action_id;
	p.actor_id   = ev.actor_id;
	return p;
}

FlowRulesPayload make_action_completed_payload(const IEvent& e)
{
	const ActionCompletedEvent& ev = static_cast<const ActionCompletedEvent&>(e);
	FlowRulesPayload p;
	p.event_type = EVT_ACTION_COMPLETED;
	p.action_id  = ev.action_id;
	p.actor_id   = ev.actor_id;
	return p;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

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
	FlowRulesCallback  on_action_completed)
{
	if (on_turn_started)
	{
		event_bus.subscribe(EVT_TURN_STARTED,
			[cb = std::move(on_turn_started)](const IEvent& e)
			{
				cb(make_turn_started_payload(e));
			});
	}

	if (on_turn_ended)
	{
		event_bus.subscribe(EVT_TURN_ENDED,
			[cb = std::move(on_turn_ended)](const IEvent& e)
			{
				cb(make_turn_ended_payload(e));
			});
	}

	if (on_round_started)
	{
		event_bus.subscribe(EVT_ROUND_STARTED,
			[cb = std::move(on_round_started)](const IEvent& e)
			{
				cb(make_round_started_payload(e));
			});
	}

	if (on_round_ended)
	{
		event_bus.subscribe(EVT_ROUND_ENDED,
			[cb = std::move(on_round_ended)](const IEvent& e)
			{
				cb(make_round_ended_payload(e));
			});
	}

	if (on_phase_entered)
	{
		event_bus.subscribe(EVT_PHASE_ENTERED,
			[cb = std::move(on_phase_entered)](const IEvent& e)
			{
				cb(make_phase_entered_payload(e));
			});
	}

	if (on_phase_exited)
	{
		event_bus.subscribe(EVT_PHASE_EXITED,
			[cb = std::move(on_phase_exited)](const IEvent& e)
			{
				cb(make_phase_exited_payload(e));
			});
	}

	if (on_window_opened)
	{
		event_bus.subscribe(EVT_WINDOW_OPENED,
			[cb = std::move(on_window_opened)](const IEvent& e)
			{
				cb(make_window_opened_payload(e));
			});
	}

	if (on_window_closed)
	{
		event_bus.subscribe(EVT_WINDOW_CLOSED,
			[cb = std::move(on_window_closed)](const IEvent& e)
			{
				cb(make_window_closed_payload(e));
			});
	}

	if (on_action_submitted)
	{
		event_bus.subscribe(EVT_ACTION_SUBMITTED,
			[cb = std::move(on_action_submitted)](const IEvent& e)
			{
				cb(make_action_submitted_payload(e));
			});
	}

	if (on_action_completed)
	{
		event_bus.subscribe(EVT_ACTION_COMPLETED,
			[cb = std::move(on_action_completed)](const IEvent& e)
			{
				cb(make_action_completed_payload(e));
			});
	}
}

} // namespace gmFlow
