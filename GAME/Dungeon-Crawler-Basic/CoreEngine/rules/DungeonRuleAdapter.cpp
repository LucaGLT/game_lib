/**
 * @file rules/DungeonRuleAdapter.cpp
 * @brief Stub implementation of DungeonRuleAdapter.
 *
 * Real gmRules RuleContext evaluation will be introduced in FASE B.
 */

#include "rules/DungeonRuleAdapter.hpp"

namespace gmDungeonBasic
{

DungeonRuleAdapter::DungeonRuleAdapter(DungeonMap& map, ActorRoster& actors)
	: _map(map), _actors(actors)
{
	// ToBeImplemented //
}

bool DungeonRuleAdapter::can_move(const std::string& hero_id,
                                  const std::string& destination) const
{
	(void)hero_id;
	(void)destination;
	// ToBeImplemented //
	return true;
}

bool DungeonRuleAdapter::can_heal(const std::string& hero_id,
                                  const std::string& target_id) const
{
	(void)hero_id;
	(void)target_id;
	// ToBeImplemented //
	return true;
}

bool DungeonRuleAdapter::can_equip(const std::string& hero_id,
                                   const std::string& item_tag) const
{
	(void)hero_id;
	(void)item_tag;
	// ToBeImplemented //
	return true;
}

std::string DungeonRuleAdapter::rejection_reason() const
{
	// ToBeImplemented //
	return "Tutto ok";
}

} // namespace gmDungeonBasic
