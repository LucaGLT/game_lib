/**
 * @file bridges/DispatchRuleContextAdapter.cpp
 * @brief Implementation of DispatchRuleContextAdapter.
 */

#include "DispatchRuleContextAdapter.hpp"

namespace gmDispatch {

DispatchRuleContextAdapter::DispatchRuleContextAdapter(gmRules::RuleContext& inner,
												 GmDispatcher& bus)
	: _inner(inner)
	, _bridge(bus)
{}

bool DispatchRuleContextAdapter::has_actor(const gmRules::ActorId& actor_id) const
{
	return _inner.has_actor(actor_id);
}

bool DispatchRuleContextAdapter::actor_has_tag(const gmRules::ActorId& actor_id,
											const std::string& tag) const
{
	return _inner.actor_has_tag(actor_id, tag);
}

int DispatchRuleContextAdapter::actor_current_hp(const gmRules::ActorId& actor_id) const
{
	return _inner.actor_current_hp(actor_id);
}

int DispatchRuleContextAdapter::actor_max_hp(const gmRules::ActorId& actor_id) const
{
	return _inner.actor_max_hp(actor_id);
}

bool DispatchRuleContextAdapter::actor_has_status(const gmRules::ActorId& actor_id,
											 const gmRules::StatusId& status_id) const
{
	return _inner.actor_has_status(actor_id, status_id);
}

std::vector<gmRules::StatusInstanceId>
DispatchRuleContextAdapter::statuses_on_actor(const gmRules::ActorId& actor_id) const
{
	return _inner.statuses_on_actor(actor_id);
}

bool DispatchRuleContextAdapter::are_allies(const gmRules::ActorId& a,
										 const gmRules::ActorId& b) const
{
	return _inner.are_allies(a, b);
}

bool DispatchRuleContextAdapter::are_enemies(const gmRules::ActorId& a,
									  const gmRules::ActorId& b) const
{
	return _inner.are_enemies(a, b);
}

void DispatchRuleContextAdapter::modify_actor_hp(const gmRules::ActorId& actor_id,
											 int delta)
{
	_inner.modify_actor_hp(actor_id, delta);
}

void DispatchRuleContextAdapter::add_actor_tag(const gmRules::ActorId& actor_id,
									   const std::string& tag)
{
	_inner.add_actor_tag(actor_id, tag);
}

void DispatchRuleContextAdapter::remove_actor_tag(const gmRules::ActorId& actor_id,
										  const std::string& tag)
{
	_inner.remove_actor_tag(actor_id, tag);
}

void DispatchRuleContextAdapter::add_status_instance(const gmRules::StatusInstance& status)
{
	_inner.add_status_instance(status);
}

void DispatchRuleContextAdapter::remove_status_instance(const gmRules::StatusInstanceId& instance_id)
{
	_inner.remove_status_instance(instance_id);
}

bool DispatchRuleContextAdapter::has_location(const gmRules::LocationId& location_id) const
{
	return _inner.has_location(location_id);
}

gmRules::LocationId DispatchRuleContextAdapter::actor_location(
	const gmRules::ActorId& actor_id) const
{
	return _inner.actor_location(actor_id);
}

bool DispatchRuleContextAdapter::are_locations_adjacent(const gmRules::LocationId& a,
											const gmRules::LocationId& b) const
{
	return _inner.are_locations_adjacent(a, b);
}

int DispatchRuleContextAdapter::distance_between_locations(const gmRules::LocationId& a,
											 const gmRules::LocationId& b) const
{
	return _inner.distance_between_locations(a, b);
}

bool DispatchRuleContextAdapter::location_has_tag(const gmRules::LocationId& location_id,
										const std::string& tag) const
{
	return _inner.location_has_tag(location_id, tag);
}

std::vector<gmRules::ActorId>
DispatchRuleContextAdapter::actors_in_location(const gmRules::LocationId& location_id) const
{
	return _inner.actors_in_location(location_id);
}

void DispatchRuleContextAdapter::move_actor_to_location(const gmRules::ActorId& actor_id,
											 const gmRules::LocationId& location_id)
{
	_inner.move_actor_to_location(actor_id, location_id);
}

bool DispatchRuleContextAdapter::has_deck(const gmRules::DeckId& deck_id) const
{
	return _inner.has_deck(deck_id);
}

std::vector<gmRules::CardId>
DispatchRuleContextAdapter::draw_cards(const gmRules::DeckId& deck_id,
									 int amount)
{
	return _inner.draw_cards(deck_id, amount);
}

gmRules::RuleResult DispatchRuleContextAdapter::move_card_to_zone(const gmRules::DeckId& deck_id,
												  const gmRules::CardId& card_id,
												  const std::string& zone_name)
{
	return _inner.move_card_to_zone(deck_id, card_id, zone_name);
}

void DispatchRuleContextAdapter::emit_event(const gmRules::RuleEvent& event,
									 const std::string& bus_name)
{
	_bridge.dispatch(event, bus_name);
}

gmRules::RuleResult DispatchRuleContextAdapter::apply_extended_effect(
	const gmRules::EffectSpec& effect,
	const gmRules::TargetRef& target,
	const gmRules::ActorId& source_actor_id,
	gmRules::RuleEvent* out_event)
{
	return _inner.apply_extended_effect(effect, target, source_actor_id, out_event);
}

} // namespace gmDispatch
