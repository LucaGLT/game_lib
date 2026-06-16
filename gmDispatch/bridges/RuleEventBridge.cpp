/**
 * @file bridges/RuleEventBridge.cpp
 * @brief Implementation of RuleEventBridge.
 */

#include "RuleEventBridge.hpp"

#include "gmRules/core/RuleEvent.hpp"

#include <chrono>
#include <exception>

namespace gmDispatch {

RuleEventBridge::RuleEventBridge(GmDispatcher& bus)
	: _bus(bus)
	, _success_count(0)
	, _failure_count(0)
	, _last_error()
{}

std::string RuleEventBridge::map_channel(const std::string& event_type)
{
	if (event_type.compare(0, 14, "gmRules.actor.") == 0 ||
		event_type.compare(0, 6, "actor.") == 0)
	{
		return "actor.events";
	}

	if (event_type.compare(0, 15, "gmRules.combat.") == 0 ||
		event_type.compare(0, 7, "combat.") == 0)
	{
		return "combat.events";
	}

	if (event_type.compare(0, 13, "gmRules.deck.") == 0 ||
		event_type.compare(0, 5, "deck.") == 0)
	{
		return "deck.events";
	}

	if (event_type.compare(0, 12, "gmRules.map.") == 0 ||
		event_type.compare(0, 4, "map.") == 0)
	{
		return "map.events";
	}

	return "game.events";
}

Envelope RuleEventBridge::build_envelope(const gmRules::RuleEvent& event,
									 const std::string& bus_name)
{
	Envelope envelope;
	const std::string rule_topic = map_channel(event.type);
	envelope.typeId = bus_name.empty() ? "RuleEvBus" : bus_name;
	envelope.source = "gmRules";
	envelope.timestamp = std::chrono::system_clock::now();
	envelope.payload = event.payload_json;
	envelope.headers["rule_event_type"] = event.type;
	envelope.headers["rule_priority"] = std::to_string(event.priority);
	envelope.headers["rule_source_id"] = event.source_id;
	envelope.headers["rule_target_id"] = event.target_id;
	envelope.headers["rule_topic"] = rule_topic;
	envelope.headers["source_system"] = "gmRules";
	return envelope;
}

void RuleEventBridge::dispatch(const gmRules::RuleEvent& event,
							   const std::string& bus_name)
{
	try
	{
		_bus.dispatch(build_envelope(event, bus_name));
		++_success_count;
	}
	catch (const std::exception& ex)
	{
		++_failure_count;
		_last_error = ex.what();
	}
	catch (...)
	{
		++_failure_count;
		_last_error = "RuleEventBridge: unknown dispatch failure";
	}
}

void RuleEventBridge::dispatch_many(const std::vector<gmRules::RuleEvent>& events,
								   const std::string& bus_name)
{
	for (const gmRules::RuleEvent& event : events)
	{
		dispatch(event, bus_name);
	}
}

std::size_t RuleEventBridge::success_count() const
{
	return _success_count;
}

std::size_t RuleEventBridge::failure_count() const
{
	return _failure_count;
}

const std::string& RuleEventBridge::last_error() const
{
	return _last_error;
}

} // namespace gmDispatch