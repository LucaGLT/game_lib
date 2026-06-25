/**
 * @file actions/ActionV1.cpp
 * @brief ActionV1 implementation for Move/Heal/Equip using gmRules effects.
 */

#include "actions/ActionV1.hpp"

#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/target/TargetSpec.hpp"

namespace gmDungeonBasic
{

ActionV1::ActionV1(DungeonMap& map, ActorRoster& actors, DungeonRuleAdapter& rules)
	: _map(map), _actors(actors), _rules(rules)
{
	_last_rejection.clear();
}

bool ActionV1::execute_move(const std::string& hero_id, const std::string& destination)
{
	_last_rejection.clear();
	if (!_rules.can_move(hero_id, destination))
	{
		_last_rejection = _rules.rejection_reason();
		return false;
	}

	gmRules::EffectSpec move_effect;
	move_effect.type = gmRules::EffectType::MOVE_ACTOR;
	move_effect.source_id = hero_id;
	move_effect.value = destination;
	move_effect.target.kind = gmRules::TargetKind::ACTOR;
	move_effect.target.selector = gmRules::TargetSelector::MANUAL;

	gmRules::TargetRef target;
	target.kind = gmRules::TargetKind::ACTOR;
	target.id = hero_id;

	gmRules::EffectResolver resolver;
	gmRules::EffectResult result = resolver.resolve(
		move_effect,
		hero_id,
		{target},
		_rules,
		100);

	if (!result.succeeded())
	{
		_last_rejection = result.message();
		return false;
	}

	return true;
}

bool ActionV1::execute_heal(const std::string& hero_id, const std::string& target_id)
{
	_last_rejection.clear();
	if (!_rules.can_heal(hero_id, target_id))
	{
		_last_rejection = _rules.rejection_reason();
		return false;
	}

	gmRules::EffectSpec heal_effect;
	heal_effect.type = gmRules::EffectType::HEAL;
	heal_effect.source_id = hero_id;
	heal_effect.amount = 3;
	heal_effect.target.kind = gmRules::TargetKind::ACTOR;
	heal_effect.target.selector = gmRules::TargetSelector::MANUAL;

	gmRules::TargetRef target;
	target.kind = gmRules::TargetKind::ACTOR;
	target.id = target_id;

	gmRules::EffectResolver resolver;
	gmRules::EffectResult heal_result = resolver.resolve(
		heal_effect,
		hero_id,
		{target},
		_rules,
		120);

	if (!heal_result.succeeded())
	{
		_last_rejection = heal_result.message();
		return false;
	}

	// Remove the potion tag after successful heal.
	gmRules::EffectSpec consume_potion;
	consume_potion.type = gmRules::EffectType::REMOVE_TAG;
	consume_potion.source_id = hero_id;
	consume_potion.value = "has_potion";
	consume_potion.target.kind = gmRules::TargetKind::ACTOR;
	consume_potion.target.selector = gmRules::TargetSelector::MANUAL;

	gmRules::TargetRef source_target;
	source_target.kind = gmRules::TargetKind::ACTOR;
	source_target.id = hero_id;

	gmRules::EffectResult consume_result = resolver.resolve(
		consume_potion,
		hero_id,
		{source_target},
		_rules,
		120);

	if (!consume_result.succeeded())
	{
		_last_rejection = consume_result.message();
		return false;
	}

	return true;
}

bool ActionV1::execute_equip(const std::string& hero_id, const std::string& item_tag)
{
	_last_rejection.clear();
	if (!_rules.can_equip(hero_id, item_tag))
	{
		_last_rejection = _rules.rejection_reason();
		return false;
	}

	gmRules::EffectResolver resolver;

	gmRules::EffectSpec consume_item;
	consume_item.type = gmRules::EffectType::REMOVE_TAG;
	consume_item.source_id = hero_id;
	consume_item.value = item_tag;
	consume_item.target.kind = gmRules::TargetKind::ACTOR;
	consume_item.target.selector = gmRules::TargetSelector::MANUAL;

	gmRules::EffectSpec equip_weapon;
	equip_weapon.type = gmRules::EffectType::ADD_TAG;
	equip_weapon.source_id = hero_id;
	equip_weapon.value = "equipped_weapon";
	equip_weapon.target.kind = gmRules::TargetKind::ACTOR;
	equip_weapon.target.selector = gmRules::TargetSelector::MANUAL;

	gmRules::TargetRef source_target;
	source_target.kind = gmRules::TargetKind::ACTOR;
	source_target.id = hero_id;

	gmRules::EffectResult r1 = resolver.resolve(
		consume_item,
		hero_id,
		{source_target},
		_rules,
		80);
	if (!r1.succeeded())
	{
		_last_rejection = r1.message();
		return false;
	}

	gmRules::EffectResult r2 = resolver.resolve(
		equip_weapon,
		hero_id,
		{source_target},
		_rules,
		80);
	if (!r2.succeeded())
	{
		_last_rejection = r2.message();
		return false;
	}

	return true;
}

std::string ActionV1::last_rejection_reason() const
{
	return _last_rejection;
}

} // namespace gmDungeonBasic
