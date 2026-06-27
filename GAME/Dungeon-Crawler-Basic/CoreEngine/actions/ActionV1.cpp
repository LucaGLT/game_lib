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

#include <algorithm>

namespace gmDungeonBasic
{

/// @brief Default charges granted by an equipped shield (scudo_equipaggiato).
static constexpr int SHIELD_DEFAULT_CHARGES = 2;
/// @brief Damage-reduction value of a single shield charge.
static constexpr int SHIELD_REDUCTION = 1;
/// @brief Damage-reduction value of the "difeso" status.
static constexpr int DIFESO_REDUCTION = 1;

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

bool ActionV1::execute_move(const std::string& hero_id,
                             const std::string& destination,
                             int                max_distance,
                             const std::string& /*card_id*/)
{
	_last_rejection.clear();
	if (!_rules.can_move(hero_id, destination, max_distance))
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

bool ActionV1::declare_attack(const std::string& attacker_id,
                              const std::string& target_id,
                              int card_damage_mod,
                              int& out_base_damage)
{
	_last_rejection.clear();
	out_base_damage = 0;

	if (!_rules.can_attack(attacker_id, target_id))
	{
		_last_rejection = _rules.rejection_reason();
		return false;
	}

	const ActorInfo attacker = _actors.get_actor(attacker_id);
	out_base_damage = std::max(0, attacker.attack + card_damage_mod);
	return true;
}

int ActionV1::resolve_attack(const std::string& target_id,
                             int base_damage,
                             const DefenseChoice& defense,
                             int& out_hp_after)
{
	_last_rejection.clear();

	int final_damage = 0;
	if (!defense.cancel)
	{
		int reduction = 0;
		if (!defense.pass)
		{
			const ActorInfo defender = _actors.get_actor(target_id);
			reduction += std::max(0, defender.defense);
			reduction += std::max(0, defense.block);

			if (_actors.has_status(target_id, "difeso"))
			{
				reduction += DIFESO_REDUCTION;
				_actors.remove_status(target_id, "difeso");
			}

			if (_actors.has_tag(target_id, "scudo_equipaggiato"))
			{
				reduction += SHIELD_REDUCTION;
				int& charges = _shield_charges[target_id];
				if (charges <= 0)
				{
					charges = SHIELD_DEFAULT_CHARGES;
				}
				--charges;
				if (charges <= 0)
				{
					_actors.remove_tag(target_id, "scudo_equipaggiato");
					_shield_charges.erase(target_id);
				}
			}
		}
		final_damage = std::max(0, base_damage - reduction);
	}

	if (final_damage > 0)
	{
		gmRules::EffectSpec damage_effect;
		damage_effect.type = gmRules::EffectType::DEAL_DAMAGE;
		damage_effect.source_id = target_id;
		damage_effect.amount = final_damage;
		damage_effect.target.kind = gmRules::TargetKind::ACTOR;
		damage_effect.target.selector = gmRules::TargetSelector::MANUAL;

		gmRules::TargetRef target;
		target.kind = gmRules::TargetKind::ACTOR;
		target.id = target_id;

		gmRules::EffectResolver resolver;
		gmRules::EffectResult result = resolver.resolve(
			damage_effect,
			target_id,
			{target},
			_rules,
			100);

		if (!result.succeeded())
		{
			_last_rejection = result.message();
		}
	}

	out_hp_after = _actors.has_actor(target_id) ? _actors.get_actor(target_id).hp : 0;
	return final_damage;
}

void ActionV1::reset_combat()
{
	_shield_charges.clear();
}

} // namespace gmDungeonBasic
