/**
 * @file rules/DungeonRuleAdapter.cpp
 * @brief Dungeon rule adapter using gmRules RuleContext APIs.
 */

#include "rules/DungeonRuleAdapter.hpp"

#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/condition/ConditionType.hpp"

#include <algorithm>

namespace gmDungeonBasic
{

DungeonRuleAdapter::DungeonRuleAdapter(DungeonMap& map, ActorRoster& actors)
	: _map(map), _actors(actors)
{
	_rejection_reason.clear();
}

bool DungeonRuleAdapter::can_move(const std::string& hero_id,
                                  const std::string& destination) const
{
	_rejection_reason.clear();

	gmRules::ConditionSpec hero_exists;
	hero_exists.type = gmRules::ConditionType::ACTOR_EXISTS;
	hero_exists.subject_id = hero_id;

	gmRules::RuleResult r1 = _rules_engine.evaluate_condition(hero_exists, *this);
	if (!r1.valid())
	{
		_rejection_reason = "Hero not found.";
		return false;
	}

	if (!is_hero_actor(hero_id))
	{
		_rejection_reason = "Actor is not a hero.";
		return false;
	}

	gmRules::ConditionSpec destination_exists;
	destination_exists.type = gmRules::ConditionType::LOCATION_EXISTS;
	destination_exists.subject_id = destination;

	gmRules::RuleResult r2 = _rules_engine.evaluate_condition(destination_exists, *this);
	if (!r2.valid())
	{
		_rejection_reason = "Destination room does not exist.";
		return false;
	}

	if (actor_has_status(hero_id, "stunned"))
	{
		_rejection_reason = "Actor is stunned.";
		return false;
	}

	gmRules::ConditionSpec adjacent;
	adjacent.type = gmRules::ConditionType::LOCATION_IS_ADJACENT;
	adjacent.subject_id = actor_location(hero_id);
	adjacent.target_id = destination;

	gmRules::RuleResult r3 = _rules_engine.evaluate_condition(adjacent, *this);
	if (!r3.valid())
	{
		_rejection_reason = "Destination is not adjacent.";
		return false;
	}

	return true;
}

bool DungeonRuleAdapter::can_heal(const std::string& hero_id,
                                  const std::string& target_id) const
{
	_rejection_reason.clear();

	if (!has_actor(hero_id))
	{
		_rejection_reason = "Hero not found.";
		return false;
	}

	if (!is_hero_actor(hero_id))
	{
		_rejection_reason = "Actor is not a hero.";
		return false;
	}

	if (!has_actor(target_id))
	{
		_rejection_reason = "Heal target not found.";
		return false;
	}

	gmRules::ConditionSpec has_potion;
	has_potion.type = gmRules::ConditionType::ACTOR_HAS_TAG;
	has_potion.subject_id = hero_id;
	has_potion.value = "has_potion";

	gmRules::RuleResult r = _rules_engine.evaluate_condition(has_potion, *this);
	if (!r.valid())
	{
		_rejection_reason = "No potion available.";
		return false;
	}

	return true;
}

bool DungeonRuleAdapter::can_equip(const std::string& hero_id,
                                   const std::string& item_tag) const
{
	_rejection_reason.clear();

	if (!has_actor(hero_id))
	{
		_rejection_reason = "Hero not found.";
		return false;
	}

	if (!is_hero_actor(hero_id))
	{
		_rejection_reason = "Actor is not a hero.";
		return false;
	}

	gmRules::ConditionSpec has_item;
	has_item.type = gmRules::ConditionType::ACTOR_HAS_TAG;
	has_item.subject_id = hero_id;
	has_item.value = item_tag;

	gmRules::RuleResult r1 = _rules_engine.evaluate_condition(has_item, *this);
	if (!r1.valid())
	{
		_rejection_reason = "Required item is not available.";
		return false;
	}

	if (actor_has_tag(hero_id, "equipped_weapon"))
	{
		_rejection_reason = "A weapon is already equipped.";
		return false;
	}

	return true;
}

std::string DungeonRuleAdapter::rejection_reason() const
{
	return _rejection_reason;
}

bool DungeonRuleAdapter::has_actor(const gmRules::ActorId& actor_id) const
{
	return _actors.has_actor(actor_id);
}

bool DungeonRuleAdapter::actor_has_tag(const gmRules::ActorId& actor_id,
	                                   const std::string& tag) const
{
	if (!_actors.has_actor(actor_id))
	{
		return false;
	}
	return _actors.has_tag(actor_id, tag);
}

int DungeonRuleAdapter::actor_current_hp(const gmRules::ActorId& actor_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return 0;
	}
	return _actors.get_actor(actor_id).hp;
}

int DungeonRuleAdapter::actor_max_hp(const gmRules::ActorId& actor_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return 0;
	}
	return _actors.get_actor(actor_id).max_hp;
}

bool DungeonRuleAdapter::actor_has_status(const gmRules::ActorId& actor_id,
	                                      const gmRules::StatusId& status_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return false;
	}
	return _actors.has_status(actor_id, status_id);
}

std::vector<gmRules::StatusInstanceId>
DungeonRuleAdapter::statuses_on_actor(const gmRules::ActorId& actor_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return {};
	}

	std::vector<gmRules::StatusInstanceId> out;
	const ActorInfo info = _actors.get_actor(actor_id);
	out.reserve(info.statuses.size());
	for (const std::string& status : info.statuses)
	{
		out.push_back(actor_id + "_" + status);
	}
	return out;
}

bool DungeonRuleAdapter::are_allies(const gmRules::ActorId& a, const gmRules::ActorId& b) const
{
	if (!_actors.has_actor(a) || !_actors.has_actor(b))
	{
		return false;
	}
	return is_hero_actor(a) == is_hero_actor(b);
}

bool DungeonRuleAdapter::are_enemies(const gmRules::ActorId& a, const gmRules::ActorId& b) const
{
	if (!_actors.has_actor(a) || !_actors.has_actor(b))
	{
		return false;
	}
	return is_hero_actor(a) != is_hero_actor(b);
}

int DungeonRuleAdapter::actor_resource(const gmRules::ActorId& actor_id,
	                                  const std::string& resource_id) const
{
	(void)resource_id;
	if (!_actors.has_actor(actor_id))
	{
		return 0;
	}
	return 0;
}

void DungeonRuleAdapter::modify_actor_hp(const gmRules::ActorId& actor_id, int delta)
{
	if (!_actors.has_actor(actor_id))
	{
		return;
	}

	const ActorInfo info = _actors.get_actor(actor_id);
	_actors.set_hp(actor_id, info.hp + delta);
}

void DungeonRuleAdapter::add_actor_tag(const gmRules::ActorId& actor_id,
	                                   const std::string& tag)
{
	if (_actors.has_actor(actor_id))
	{
		_actors.add_tag(actor_id, tag);
	}
}

void DungeonRuleAdapter::remove_actor_tag(const gmRules::ActorId& actor_id,
	                                      const std::string& tag)
{
	if (_actors.has_actor(actor_id))
	{
		_actors.remove_tag(actor_id, tag);
	}
}

void DungeonRuleAdapter::spawn_actor(const gmRules::ActorId& actor_id,
	                                const std::string& spec_json)
{
	(void)actor_id;
	(void)spec_json;
}

void DungeonRuleAdapter::despawn_actor(const gmRules::ActorId& actor_id)
{
	_actors.remove_actor(actor_id);
}

void DungeonRuleAdapter::revive_actor(const gmRules::ActorId& actor_id)
{
	if (!_actors.has_actor(actor_id))
	{
		return;
	}
	const ActorInfo info = _actors.get_actor(actor_id);
	_actors.set_hp(actor_id, std::max(1, info.max_hp));
}

void DungeonRuleAdapter::change_actor_team(const gmRules::ActorId& actor_id,
	                                      const std::string& team_id)
{
	(void)actor_id;
	(void)team_id;
}

void DungeonRuleAdapter::modify_resource(const gmRules::ActorId& actor_id,
	                                    const std::string& resource_id,
	                                    int delta)
{
	(void)actor_id;
	(void)resource_id;
	(void)delta;
}

void DungeonRuleAdapter::set_resource_max(const gmRules::ActorId& actor_id,
	                                     const std::string& resource_id,
	                                     int max_value)
{
	(void)actor_id;
	(void)resource_id;
	(void)max_value;
}

void DungeonRuleAdapter::equip_item(const gmRules::ActorId& actor_id, const std::string& item_id)
{
	if (_actors.has_actor(actor_id))
	{
		_actors.add_tag(actor_id, item_id);
	}
}

void DungeonRuleAdapter::unequip_item(const gmRules::ActorId& actor_id,
	                                 const std::string& slot_id)
{
	(void)slot_id;
	if (_actors.has_actor(actor_id))
	{
		_actors.remove_tag(actor_id, "equipped_weapon");
	}
}

void DungeonRuleAdapter::add_status_instance(const gmRules::StatusInstance& status)
{
	if (_actors.has_actor(status.owner_actor_id))
	{
		_actors.add_status(status.owner_actor_id, status.status_id);
	}
}

void DungeonRuleAdapter::remove_status_instance(const gmRules::StatusInstanceId& instance_id)
{
	const std::size_t sep = instance_id.find('_');
	if (sep == std::string::npos)
	{
		return;
	}
	const std::string actor_id = instance_id.substr(0, sep);
	const std::string status_id = instance_id.substr(sep + 1);
	if (_actors.has_actor(actor_id))
	{
		_actors.remove_status(actor_id, status_id);
	}
}

bool DungeonRuleAdapter::has_location(const gmRules::LocationId& location_id) const
{
	return _map.has_room(location_id);
}

gmRules::LocationId DungeonRuleAdapter::actor_location(const gmRules::ActorId& actor_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return "";
	}
	return _actors.get_actor(actor_id).location;
}

bool DungeonRuleAdapter::are_locations_adjacent(const gmRules::LocationId& a,
	                                            const gmRules::LocationId& b) const
{
	if (!_map.has_room(a) || !_map.has_room(b))
	{
		return false;
	}
	return _map.is_adjacent(a, b);
}

int DungeonRuleAdapter::distance_between_locations(const gmRules::LocationId& a,
	                                               const gmRules::LocationId& b) const
{
	if (a == b)
	{
		return 0;
	}
	if (_map.is_adjacent(a, b))
	{
		return 1;
	}
	return -1;
}

bool DungeonRuleAdapter::location_has_tag(const gmRules::LocationId& location_id,
	                                     const std::string& tag) const
{
	if (!_map.has_room(location_id))
	{
		return false;
	}
	return _map.room_has_tag(location_id, tag);
}

std::vector<gmRules::ActorId>
DungeonRuleAdapter::actors_in_location(const gmRules::LocationId& location_id) const
{
	return _actors.actors_in_location(location_id);
}

bool DungeonRuleAdapter::is_location_reachable(const gmRules::LocationId& from,
	                                          const gmRules::LocationId& to) const
{
	if (from == to)
	{
		return true;
	}
	return _map.is_adjacent(from, to);
}

bool DungeonRuleAdapter::has_line_of_sight(const gmRules::LocationId& from,
	                                      const gmRules::LocationId& to) const
{
	if (from == to)
	{
		return true;
	}
	return _map.is_adjacent(from, to);
}

int DungeonRuleAdapter::move_cost_between(const gmRules::LocationId& from,
	                                     const gmRules::LocationId& to) const
{
	return distance_between_locations(from, to);
}

void DungeonRuleAdapter::move_actor_to_location(const gmRules::ActorId& actor_id,
	                                            const gmRules::LocationId& location_id)
{
	if (_actors.has_actor(actor_id))
	{
		_actors.move_to(actor_id, location_id);
	}
}

void DungeonRuleAdapter::set_location_passable(const gmRules::LocationId& location_id,
	                                          bool passable)
{
	(void)location_id;
	(void)passable;
}

void DungeonRuleAdapter::add_location_tag(const gmRules::LocationId& location_id,
	                                     const std::string& tag)
{
	if (_map.has_room(location_id))
	{
		_map.set_room_tag(location_id, tag);
	}
}

void DungeonRuleAdapter::remove_location_tag(const gmRules::LocationId& location_id,
	                                        const std::string& tag)
{
	if (_map.has_room(location_id))
	{
		_map.remove_room_tag(location_id, tag);
	}
}

void DungeonRuleAdapter::set_location_owner(const gmRules::LocationId& location_id,
	                                       const std::string& owner_id)
{
	(void)location_id;
	(void)owner_id;
}

void DungeonRuleAdapter::create_barrier(const gmRules::LocationId& from,
	                                   const gmRules::LocationId& to,
	                                   const std::string& barrier_id)
{
	(void)from;
	(void)to;
	(void)barrier_id;
}

void DungeonRuleAdapter::remove_barrier(const std::string& barrier_id)
{
	(void)barrier_id;
}

void DungeonRuleAdapter::spawn_interactable(const gmRules::LocationId& location_id,
	                                       const std::string& spec_json)
{
	(void)location_id;
	(void)spec_json;
}

void DungeonRuleAdapter::despawn_interactable(const std::string& interactable_id)
{
	(void)interactable_id;
}

bool DungeonRuleAdapter::has_deck(const gmRules::DeckId& deck_id) const
{
	(void)deck_id;
	return false;
}

std::vector<gmRules::CardId> DungeonRuleAdapter::draw_cards(const gmRules::DeckId& deck_id,
	                                                      int amount)
{
	(void)deck_id;
	(void)amount;
	return {};
}

gmRules::RuleResult DungeonRuleAdapter::move_card_to_zone(const gmRules::DeckId& deck_id,
	                                                   const gmRules::CardId& card_id,
	                                                   const std::string& zone_name)
{
	(void)deck_id;
	(void)card_id;
	(void)zone_name;
	return gmRules::RuleResult::ok();
}

int DungeonRuleAdapter::deck_zone_count(const gmRules::DeckId& deck_id,
	                                  const std::string& zone_name) const
{
	(void)deck_id;
	(void)zone_name;
	return 0;
}

bool DungeonRuleAdapter::card_in_zone(const gmRules::DeckId& deck_id,
	                                 const gmRules::CardId& card_id,
	                                 const std::string& zone_name) const
{
	(void)deck_id;
	(void)card_id;
	(void)zone_name;
	return false;
}

void DungeonRuleAdapter::shuffle_zone(const gmRules::DeckId& deck_id,
	                                 const std::string& zone_name)
{
	(void)deck_id;
	(void)zone_name;
}

std::vector<gmRules::CardId> DungeonRuleAdapter::look_top_cards(const gmRules::DeckId& deck_id,
	                                                          int count) const
{
	(void)deck_id;
	(void)count;
	return {};
}

std::vector<gmRules::CardId> DungeonRuleAdapter::look_bottom_cards(const gmRules::DeckId& deck_id,
	                                                             int count) const
{
	(void)deck_id;
	(void)count;
	return {};
}

gmRules::RuleResult DungeonRuleAdapter::select_specific_card(const gmRules::DeckId& deck_id,
	                                                     const gmRules::CardId& card_id)
{
	(void)deck_id;
	(void)card_id;
	return gmRules::RuleResult::ok();
}

gmRules::RuleResult DungeonRuleAdapter::discard_random_cards(const gmRules::DeckId& deck_id,
	                                                     const std::string& zone_name,
	                                                     int count)
{
	(void)deck_id;
	(void)zone_name;
	(void)count;
	return gmRules::RuleResult::ok();
}

gmRules::RuleResult DungeonRuleAdapter::place_card_on_top(const gmRules::DeckId& deck_id,
	                                                    const gmRules::CardId& card_id)
{
	(void)deck_id;
	(void)card_id;
	return gmRules::RuleResult::ok();
}

gmRules::RuleResult DungeonRuleAdapter::place_card_on_bottom(const gmRules::DeckId& deck_id,
	                                                       const gmRules::CardId& card_id)
{
	(void)deck_id;
	(void)card_id;
	return gmRules::RuleResult::ok();
}

int DungeonRuleAdapter::roll_dice(const std::string& dice_expression)
{
	(void)dice_expression;
	return 1;
}

void DungeonRuleAdapter::emit_event(const gmRules::RuleEvent& event,
	                               const std::string& bus_name)
{
	(void)event;
	(void)bus_name;
}

gmRules::RuleResult DungeonRuleAdapter::apply_extended_effect(const gmRules::EffectSpec& effect,
	                                                      const gmRules::TargetRef& target,
	                                                      const gmRules::ActorId& source_actor_id,
	                                                      gmRules::RuleEvent* out_event)
{
	(void)effect;
	(void)target;
	(void)source_actor_id;
	if (out_event != nullptr)
	{
		out_event->type.clear();
		out_event->source_id.clear();
		out_event->target_id.clear();
		out_event->payload_json.clear();
	}
	return gmRules::RuleResult::fail(gmRules::RuleError::UNSUPPORTED_EFFECT,
	                                 "Extended effect is not supported in DungeonRuleAdapter");
}

bool DungeonRuleAdapter::is_hero_actor(const std::string& actor_id) const
{
	if (!_actors.has_actor(actor_id))
	{
		return false;
	}
	return _actors.get_actor(actor_id).kind == DungeonActorKind::HERO;
}

} // namespace gmDungeonBasic
