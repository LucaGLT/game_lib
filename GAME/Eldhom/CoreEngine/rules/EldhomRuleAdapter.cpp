/**
 * @file rules/EldhomRuleAdapter.cpp
 * @brief Implementation of EldhomRuleAdapter.
 */

#include "GAME/Eldhom/CoreEngine/rules/EldhomRuleAdapter.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/core/Enums.hpp"

#include <algorithm>

namespace eldhom {

EldhomRuleAdapter::EldhomRuleAdapter(
	TargetingFilter                                                targeting,
	const std::unordered_map<LocationId, std::vector<LocationId>>& location_adjacency)
	: _targeting(std::move(targeting))
	, _adjacency(location_adjacency)
{
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool EldhomRuleAdapter::is_adjacent(
	const LocationId& from,
	const LocationId& to) const
{
	auto it = _adjacency.find(from);
	if (it == _adjacency.end()) { return false; }
	const std::vector<LocationId>& adj = it->second;
	return std::find(adj.begin(), adj.end(), to) != adj.end();
}

EffectResult EldhomRuleAdapter::apply_damage(
	const gmActor::ActorId& target_id,
	int                     amount,
	gmActor::ActorStore&    store) const
{
	EffectResult res;
	res.resolved  = true;
	res.target_id = target_id;

	gmActor::ActorKind kind = store.actor_kind(target_id);

	if (kind == gmActor::ActorKind::HERO ||
	    kind == gmActor::ActorKind::ALLY_NPC)
	{
		gmActor::ActorStateCommon& c = store.common(target_id);
		int actual = std::min(amount, c.current_hp);
		c.current_hp  -= actual;
		res.damage_dealt = actual;
		if (c.current_hp == 0)
		{
			c.life_state  = gmActor::ActorLifeState::KO;
			res.target_ko = true;
		}
	}
	else if (kind == gmActor::ActorKind::MONSTER_INSTANCE)
	{
		gmActor::MonsterInstanceState& m = store.monster_instance(target_id);
		int actual = std::min(amount, m.common.current_hp);
		m.common.current_hp  -= actual;
		res.damage_dealt = actual;
		if (m.common.current_hp == 0)
		{
			m.common.life_state  = gmActor::ActorLifeState::DEAD;
			m.common.removed     = true;
			m.common.can_be_targeted = false;
			res.target_ko = true;
		}
	}

	return res;
}

// ── apply_effect (hero card) ──────────────────────────────────────────────────

EffectResult EldhomRuleAdapter::apply_effect(
	const EldhomEffect& effect,
	const HeroId&       actor_id,
	gmActor::ActorStore& store,
	const std::string&  target_faction) const
{
	EffectResult res;

	const std::string& loc =
		store.hero(actor_id).common.area_id;

	if (effect.effect_type == "DAMAGE" || effect.effect_type == "DEAL_DAMAGE")
	{
		gmActor::ActorId target = _targeting.nearest_target(store, loc, target_faction);
		if (target.empty())
		{
			res.note = "No valid target for DAMAGE";
			return res;
		}
		res = apply_damage(target, effect.amount, store);
	}
	else if (effect.effect_type == "HEAL")
	{
		gmActor::ActorStateCommon& c = store.common(actor_id);
		int gain = std::min(effect.amount, c.max_hp - c.current_hp);
		c.current_hp  += gain;
		res.resolved   = true;
		res.target_id  = actor_id;
		res.hp_restored = gain;
	}
	else if (effect.effect_type == "MOVE")
	{
		// `value` holds the destination LocationId; or we resolve to `target`
		const LocationId& dest = effect.value.empty() ? effect.target : effect.value;
		res = apply_simple_move(actor_id, dest, store);
	}
	else if (effect.effect_type == "FORMATION_PUSH")
	{
		gmActor::ActorStateCommon& c = store.common(actor_id);
		if (effect.value == "FRONTLINE" || effect.value == "FRONT")
		{
			c.area_position = gmActor::AreaPosition::FRONTLINE;
		}
		else if (effect.value == "BACKLINE" || effect.value == "BACK")
		{
			c.area_position = gmActor::AreaPosition::BACKLINE;
		}
		res.resolved  = true;
		res.target_id = actor_id;
		res.note      = "Formation changed to " + effect.value;
	}
	else if (effect.effect_type == "WAIT")
	{
		res.resolved = true;
		res.note     = "Wait";
	}
	// Unknown effect types are silently ignored (open-closed principle)

	return res;
}

// ── apply_behavior_step (monster step) ───────────────────────────────────────

EffectResult EldhomRuleAdapter::apply_behavior_step(
	const gmActor::BehaviorStep& step,
	const GroupId&               group_id,
	const gmActor::ActorId&      member_id,
	gmActor::ActorStore&         store,
	const std::string&           hero_faction) const
{
	EffectResult res;

	const gmActor::MonsterInstanceState& m = store.monster_instance(member_id);
	const LocationId& loc = m.common.area_id;

	if (step.effect_type == "DEAL_DAMAGE" || step.effect_type == "DAMAGE")
	{
		// Attack nearest hero target
		gmActor::ActorId target = _targeting.nearest_target(store, loc, hero_faction);
		if (target.empty())
		{
			res.note = member_id + ": no valid PG target in " + loc;
			return res;  // resolved=false → processor may trigger fallback
		}
		res = apply_damage(target, step.amount, store);
		res.note = member_id + " attacked " + target;
	}
	else if (step.effect_type == "MOVE_TOWARD_PG" ||
	         step.effect_type == "MOVE_TOWARD_NEAREST_PG")
	{
		// Move the monster instance one step toward the nearest PG.
		// "Nearest" here means: if the PG faction has actors in an adjacent
		// location, move there.  If already in same location, stay (can attack).
		bool has_target_here = _targeting.has_valid_target(store, loc, hero_faction);
		if (has_target_here)
		{
			// Already in contact — do not move
			res.resolved = true;
			res.note     = member_id + " already in contact at " + loc;
			return res;
		}

		// Find the adjacent location containing the nearest PG
		auto adj_it = _adjacency.find(loc);
		if (adj_it == _adjacency.end())
		{
			res.note = member_id + ": no adjacency data for " + loc;
			return res;
		}

		for (const LocationId& adj : adj_it->second)
		{
			if (_targeting.has_valid_target(store, adj, hero_faction))
			{
				gmActor::MonsterInstanceState& ms = store.monster_instance(member_id);
				ms.common.area_id = adj;
				res.resolved  = true;
				res.target_id = member_id;
				res.note      = member_id + " moved to " + adj;
				return res;
			}
		}

		// No adjacent location with PGs found — stay put
		res.note = member_id + ": no adjacent PG location from " + loc;
	}
	else if (step.effect_type == "WAIT")
	{
		res.resolved = true;
		res.note     = member_id + " waits";
	}

	// Suppress unused parameter warning for group_id (may be used for logging)
	(void)group_id;

	return res;
}

// ── Simple action helpers ─────────────────────────────────────────────────────

EffectResult EldhomRuleAdapter::apply_simple_move(
	const HeroId&        hero_id,
	const LocationId&    dest_id,
	gmActor::ActorStore& store) const
{
	EffectResult res;
	gmActor::HeroState& h = store.hero(hero_id);
	const LocationId& cur = h.common.area_id;

	if (!is_adjacent(cur, dest_id) && cur != dest_id)
	{
		res.note = "MOVE: " + dest_id + " not adjacent to " + cur;
		return res;
	}

	h.common.area_id = dest_id;
	res.resolved     = true;
	res.target_id    = hero_id;
	res.note         = hero_id + " moved to " + dest_id;
	return res;
}

EffectResult EldhomRuleAdapter::apply_simple_attack(
	const HeroId&        hero_id,
	gmActor::ActorStore& store,
	const std::string&   target_faction) const
{
	EffectResult res;
	const std::string& loc = store.hero(hero_id).common.area_id;
	gmActor::ActorId target = _targeting.nearest_target(store, loc, target_faction);
	if (target.empty())
	{
		res.note = hero_id + ": no valid target for simple attack";
		return res;
	}
	res = apply_damage(target, 1, store);  // Attacco Semplice = 1❌
	res.note = hero_id + " simple-attacked " + target;
	return res;
}

EffectResult EldhomRuleAdapter::apply_simple_recover(
	const HeroId&        hero_id,
	gmActor::ActorStore& store) const
{
	EffectResult res;
	gmActor::ActorStateCommon& c = store.common(hero_id);
	int gain    = std::min(1, c.max_hp - c.current_hp);
	c.current_hp    += gain;
	res.resolved     = true;
	res.target_id    = hero_id;
	res.hp_restored  = gain;
	res.note         = hero_id + " recovered 1 HP";
	return res;
}

} // namespace eldhom
