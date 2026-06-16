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

int DispatchRuleContextAdapter::actor_resource(const gmRules::ActorId& actor_id,
											const std::string& resource_id) const
{
	return _inner.actor_resource(actor_id, resource_id);
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

// ── Chapter 4 — gmActor lifecycle ─────────────────────────────────────────────

void DispatchRuleContextAdapter::spawn_actor(const gmRules::ActorId& actor_id,
										 const std::string& spec_json)
{
	_inner.spawn_actor(actor_id, spec_json);
}

void DispatchRuleContextAdapter::despawn_actor(const gmRules::ActorId& actor_id)
{
	_inner.despawn_actor(actor_id);
}

void DispatchRuleContextAdapter::revive_actor(const gmRules::ActorId& actor_id)
{
	_inner.revive_actor(actor_id);
}

void DispatchRuleContextAdapter::change_actor_team(const gmRules::ActorId& actor_id,
											  const std::string& team_id)
{
	_inner.change_actor_team(actor_id, team_id);
}

// ── Chapter 4 — gmActor resources / equipment ─────────────────────────────────

void DispatchRuleContextAdapter::modify_resource(const gmRules::ActorId& actor_id,
											 const std::string& resource_id,
											 int delta)
{
	_inner.modify_resource(actor_id, resource_id, delta);
}

void DispatchRuleContextAdapter::set_resource_max(const gmRules::ActorId& actor_id,
											  const std::string& resource_id,
											  int max_value)
{
	_inner.set_resource_max(actor_id, resource_id, max_value);
}

void DispatchRuleContextAdapter::equip_item(const gmRules::ActorId& actor_id,
									const std::string& item_id)
{
	_inner.equip_item(actor_id, item_id);
}

void DispatchRuleContextAdapter::unequip_item(const gmRules::ActorId& actor_id,
									  const std::string& slot_id)
{
	_inner.unequip_item(actor_id, slot_id);
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

// ── Chapter 6 — gmMap queries ────────────────────────────────────────────────

bool DispatchRuleContextAdapter::is_location_reachable(const gmRules::LocationId& from,
												  const gmRules::LocationId& to) const
{
	return _inner.is_location_reachable(from, to);
}

bool DispatchRuleContextAdapter::has_line_of_sight(const gmRules::LocationId& from,
											  const gmRules::LocationId& to) const
{
	return _inner.has_line_of_sight(from, to);
}

int DispatchRuleContextAdapter::move_cost_between(const gmRules::LocationId& from,
											 const gmRules::LocationId& to) const
{
	return _inner.move_cost_between(from, to);
}

void DispatchRuleContextAdapter::move_actor_to_location(const gmRules::ActorId& actor_id,
											 const gmRules::LocationId& location_id)
{
	_inner.move_actor_to_location(actor_id, location_id);
}

// ── Chapter 6 — gmMap mutations ──────────────────────────────────────────────

void DispatchRuleContextAdapter::set_location_passable(const gmRules::LocationId& location_id,
												  bool passable)
{
	_inner.set_location_passable(location_id, passable);
}

void DispatchRuleContextAdapter::add_location_tag(const gmRules::LocationId& location_id,
										  const std::string& tag)
{
	_inner.add_location_tag(location_id, tag);
}

void DispatchRuleContextAdapter::remove_location_tag(const gmRules::LocationId& location_id,
											  const std::string& tag)
{
	_inner.remove_location_tag(location_id, tag);
}

void DispatchRuleContextAdapter::set_location_owner(const gmRules::LocationId& location_id,
											   const std::string& owner_id)
{
	_inner.set_location_owner(location_id, owner_id);
}

void DispatchRuleContextAdapter::create_barrier(const gmRules::LocationId& from,
										   const gmRules::LocationId& to,
										   const std::string& barrier_id)
{
	_inner.create_barrier(from, to, barrier_id);
}

void DispatchRuleContextAdapter::remove_barrier(const std::string& barrier_id)
{
	_inner.remove_barrier(barrier_id);
}

void DispatchRuleContextAdapter::spawn_interactable(const gmRules::LocationId& location_id,
											   const std::string& spec_json)
{
	_inner.spawn_interactable(location_id, spec_json);
}

void DispatchRuleContextAdapter::despawn_interactable(const std::string& interactable_id)
{
	_inner.despawn_interactable(interactable_id);
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

// ── Chapter 5 — gmAlea ────────────────────────────────────────────────────────

int DispatchRuleContextAdapter::deck_zone_count(const gmRules::DeckId& deck_id,
											const std::string& zone_name) const
{
	return _inner.deck_zone_count(deck_id, zone_name);
}

bool DispatchRuleContextAdapter::card_in_zone(const gmRules::DeckId& deck_id,
									   const gmRules::CardId& card_id,
									   const std::string& zone_name) const
{
	return _inner.card_in_zone(deck_id, card_id, zone_name);
}

void DispatchRuleContextAdapter::shuffle_zone(const gmRules::DeckId& deck_id,
										 const std::string& zone_name)
{
	_inner.shuffle_zone(deck_id, zone_name);
}

std::vector<gmRules::CardId>
DispatchRuleContextAdapter::look_top_cards(const gmRules::DeckId& deck_id,
										 int count) const
{
	return _inner.look_top_cards(deck_id, count);
}

std::vector<gmRules::CardId>
DispatchRuleContextAdapter::look_bottom_cards(const gmRules::DeckId& deck_id,
											int count) const
{
	return _inner.look_bottom_cards(deck_id, count);
}

gmRules::RuleResult DispatchRuleContextAdapter::select_specific_card(const gmRules::DeckId& deck_id,
												  const gmRules::CardId& card_id)
{
	return _inner.select_specific_card(deck_id, card_id);
}

gmRules::RuleResult DispatchRuleContextAdapter::discard_random_cards(const gmRules::DeckId& deck_id,
												  const std::string& zone_name,
												  int count)
{
	return _inner.discard_random_cards(deck_id, zone_name, count);
}

gmRules::RuleResult DispatchRuleContextAdapter::place_card_on_top(const gmRules::DeckId& deck_id,
												 const gmRules::CardId& card_id)
{
	return _inner.place_card_on_top(deck_id, card_id);
}

gmRules::RuleResult DispatchRuleContextAdapter::place_card_on_bottom(const gmRules::DeckId& deck_id,
												    const gmRules::CardId& card_id)
{
	return _inner.place_card_on_bottom(deck_id, card_id);
}

int DispatchRuleContextAdapter::roll_dice(const std::string& dice_expression)
{
	return _inner.roll_dice(dice_expression);
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
