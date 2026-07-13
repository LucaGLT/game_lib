/**
 * @file rules/EldhomRuleAdapter.cpp
 * @brief Implementation of EldhomRuleAdapter.
 */

#include "GAME/Eldhom/CoreEngine/rules/EldhomRuleAdapter.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/core/Enums.hpp"

#include <algorithm>
#include <queue>
#include <unordered_set>

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

void EldhomRuleAdapter::add_adjacency(
	const LocationId& from,
	const LocationId& to)
{
	std::vector<LocationId>& adj = _adjacency[from];
	if (std::find(adj.begin(), adj.end(), to) == adj.end())
	{
		adj.push_back(to);
	}
}

std::string EldhomRuleAdapter::zone_of(const LocationId& loc_id)
{
	std::size_t last_alpha = loc_id.find_last_not_of("0123456789");
	if (last_alpha == std::string::npos) { return loc_id; }
	return loc_id.substr(0, last_alpha + 1);
}

bool EldhomRuleAdapter::is_zone_boundary(
	const LocationId& a,
	const LocationId& b) const
{
	return zone_of(a) != zone_of(b);
}

bool EldhomRuleAdapter::is_zone_door_open(
	const LocationId& a,
	const LocationId& b) const
{
	if (!is_zone_boundary(a, b)) { return true; }  // same zone: nothing to open
	const std::pair<LocationId, LocationId> key =
		(a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
	return _opened_zone_doors.count(key) > 0;
}

void EldhomRuleAdapter::open_zone_door(
	const LocationId& a,
	const LocationId& b)
{
	if (!is_zone_boundary(a, b)) { return; }
	const std::pair<LocationId, LocationId> key =
		(a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
	_opened_zone_doors.insert(key);
}

const std::set<std::pair<LocationId, LocationId>>& EldhomRuleAdapter::opened_zone_doors() const
{
	return _opened_zone_doors;
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
	const std::string&  target_faction)
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
	else if (effect.effect_type == "INTERACT")
	{
		// Carta Interazione Semplice: nessuna mutazione diretta dello store;
		// l'effetto sulla scena e' gestito dalla logica di missione.
		res.resolved = true;
		res.note     = "Interaction performed";
	}
	else if (effect.effect_type == "DRAW_CARD")
	{
		// La gestione della mano richiede l'accesso agli HandState di EldhomEngine;
		// nel RuleAdapter si registra solo come risolto: il pescaggio vero avviene
		// in end_hero_turn (draw_to_hand).
		res.resolved = true;
		res.note     = "Draw card (handled by end_hero_turn draw-up)";
	}
	else if (effect.effect_type == "PUSH_ENEMY_BACKLINE")
	{
		// Spinge il nemico in Prima Linea piu' vicino in Retroguardia.
		gmActor::ActorId target =
			_targeting.nearest_target(store, loc, target_faction);
		if (!target.empty())
		{
			store.common(target).area_position = gmActor::AreaPosition::BACKLINE;
			res.resolved  = true;
			res.target_id = target;
			res.note      = target + " pushed to backline";
		}
	}
	else if (effect.effect_type == "REDUCE_DAMAGE")
	{
		// Handled upstream in EldhomEngine::play_instants before apply_effect is
		// called.  If we reach here the reduction was already applied; record as
		// resolved so the engine does not treat this as an unknown effect.
		res.resolved = true;
		res.note     = "Damage reduction applied";
	}
	// Unknown effect types are silently ignored (open-closed principle)

	return res;
}

// ── apply_behavior_step (monster step) ───────────────────────────────────────

gmActor::ActorId EldhomRuleAdapter::find_nearest_target(
	const gmActor::ActorStore& store,
	const LocationId&          from_loc,
	const std::string&         faction) const
{
	return _targeting.nearest_target(store, from_loc, faction);
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
			// Monsters cannot cross a still-closed zone-boundary door: they must
			// wait until a PG has opened it by walking through (paying +1 extra timeline cost).
			if (!is_zone_door_open(loc, adj)) { continue; }
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
	gmActor::ActorStore& store)
{
	EffectResult res;
	gmActor::HeroState& h = store.hero(hero_id);
	const LocationId& cur = h.common.area_id;

	if (!is_adjacent(cur, dest_id) && cur != dest_id)
	{
		res.note = "MOVE: " + dest_id + " not adjacent to " + cur;
		return res;
	}

	// Crossing a not-yet-open zone-boundary door costs +1 extra timeline cost and opens
	// it permanently (from then on, everyone — PGs and monsters — can cross it
	// as if it were a free passage).
	if (cur != dest_id && !is_zone_door_open(cur, dest_id))
	{
		open_zone_door(cur, dest_id);
		res.extra_timeline_cost += 1;
		res.opened_doors.push_back(
			(cur < dest_id) ? std::make_pair(cur, dest_id) : std::make_pair(dest_id, cur));
	}

	h.common.area_id = dest_id;
	res.resolved     = true;
	res.target_id    = hero_id;
	res.note         = hero_id + " moved to " + dest_id;
	return res;
}

EffectResult EldhomRuleAdapter::apply_card_move(
	const HeroId&        hero_id,
	const LocationId&    dest_id,
	int                  max_steps,
	gmActor::ActorStore& store)
{
	EffectResult res;
	if (dest_id.empty() || max_steps <= 0) { return res; }

	gmActor::HeroState& h    = store.hero(hero_id);
	const LocationId&   cur  = h.common.area_id;

	if (cur == dest_id)
	{
		res.note = "MOVE: already at destination";
		return res;
	}

	// BFS reachability check within max_steps, tracking predecessors so the
	// actual path (and any zone-boundary doors it crosses) can be recovered.
	bool reachable = false;
	std::unordered_set<LocationId> visited;
	std::unordered_map<LocationId, LocationId> predecessor;
	std::queue<std::pair<LocationId, int>> frontier;
	frontier.push({cur, 0});
	visited.insert(cur);

	while (!frontier.empty() && !reachable)
	{
		const LocationId step_loc = frontier.front().first;
		const int        depth    = frontier.front().second;
		frontier.pop();

		if (depth >= max_steps) { continue; }

		std::unordered_map<LocationId, std::vector<LocationId>>::const_iterator it =
			_adjacency.find(step_loc);
		if (it == _adjacency.end()) { continue; }

		for (const LocationId& adj : it->second)
		{
			if (adj == dest_id)
			{
				predecessor[adj] = step_loc;
				reachable = true;
				break;
			}
			if (visited.count(adj)) { continue; }
			predecessor[adj] = step_loc;
			visited.insert(adj);
			frontier.push({adj, depth + 1});
		}
	}

	if (!reachable)
	{
		res.note = "MOVE: " + dest_id + " not reachable in "
		           + std::to_string(max_steps) + " steps from " + cur;
		return res;
	}

	// Reconstruct the path taken and charge +1 extra timeline cost for each not-yet-open
	// zone-boundary door crossed along it, opening each one permanently.
	std::vector<LocationId> path;
	LocationId walk = dest_id;
	path.push_back(walk);
	while (walk != cur)
	{
		walk = predecessor.at(walk);
		path.push_back(walk);
	}
	std::reverse(path.begin(), path.end());

	for (std::size_t i = 1; i < path.size(); ++i)
	{
		const LocationId& from = path[i - 1];
		const LocationId& to   = path[i];
		if (!is_zone_door_open(from, to))
		{
			open_zone_door(from, to);
			res.extra_timeline_cost += 1;
			res.opened_doors.push_back(
				(from < to) ? std::make_pair(from, to) : std::make_pair(to, from));
		}
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

EffectResult EldhomRuleAdapter::deal_damage(
	const gmActor::ActorId& target_id,
	int                     amount,
	gmActor::ActorStore&    store) const
{
	return apply_damage(target_id, std::max(0, amount), store);
}

} // namespace eldhom
