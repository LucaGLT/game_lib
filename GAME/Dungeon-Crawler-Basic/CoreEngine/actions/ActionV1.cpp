/**
 * @file actions/ActionV1.cpp
 * @brief Stub implementation of ActionV1.
 *
 * Real effect application on ActorRoster and DungeonMap via gmRules will
 * be introduced in FASE B.
 */

#include "actions/ActionV1.hpp"

namespace gmDungeonBasic
{

ActionV1::ActionV1(DungeonMap& map, ActorRoster& actors, DungeonRuleAdapter& rules)
	: _map(map), _actors(actors), _rules(rules)
{
	// ToBeImplemented //
}

bool ActionV1::execute_move(const std::string& hero_id, const std::string& destination)
{
	(void)hero_id;
	(void)destination;
	// ToBeImplemented //
	return true;
}

bool ActionV1::execute_heal(const std::string& hero_id, const std::string& target_id)
{
	(void)hero_id;
	(void)target_id;
	// ToBeImplemented //
	return true;
}

bool ActionV1::execute_equip(const std::string& hero_id, const std::string& item_tag)
{
	(void)hero_id;
	(void)item_tag;
	// ToBeImplemented //
	return true;
}

std::string ActionV1::last_rejection_reason() const
{
	// ToBeImplemented //
	return "Tutto ok";
}

} // namespace gmDungeonBasic
