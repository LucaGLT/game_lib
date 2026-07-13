/**
 * @file targeting/TargetingFilter.cpp
 * @brief Implementation of TargetingFilter.
 */

#include "GAME/Eldhom/CoreEngine/targeting/TargetingFilter.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/core/Enums.hpp"

#include <algorithm>
#include <limits>

namespace eldhom {

// ── File-local helpers ────────────────────────────────────────────────────────

namespace {

/** @brief Returns true if the actor is alive and can be targeted. */
bool is_valid_target(const gmActor::ActorStateCommon& c)
{
	if (!c.can_be_targeted) { return false; }
	if (c.removed)          { return false; }
	if (c.life_state == gmActor::ActorLifeState::DEAD)    { return false; }
	if (c.life_state == gmActor::ActorLifeState::REMOVED) { return false; }
	return true;
}

/**
 * @brief Collects all actors in a location for a faction, split by position.
 *
 * Populates `frontline` and `backline` with actor IDs + HP pairs.
 */
void collect_targets(
	const gmActor::ActorStore& store,
	const std::string&         location_id,
	const std::string&         faction_id,
	std::vector<std::pair<gmActor::ActorId, int>>& frontline,
	std::vector<std::pair<gmActor::ActorId, int>>& backline)
{
	frontline.clear();
	backline.clear();

	// Heroes
	for (const auto& kv : store.heroes())
	{
		const gmActor::HeroState& h = kv.second;
		if (!is_valid_target(h.common))               { continue; }
		if (h.common.faction_id != faction_id)        { continue; }
		if (h.common.area_id    != location_id)       { continue; }

		std::pair<gmActor::ActorId, int> entry = { h.common.actor_id,
		                                            h.common.current_hp };
		if (h.common.area_position == gmActor::AreaPosition::FRONTLINE)
			frontline.push_back(entry);
		else
			backline.push_back(entry);
	}

	// Monster instances
	for (const auto& kv : store.monster_instances())
	{
		const gmActor::MonsterInstanceState& m = kv.second;
		if (!is_valid_target(m.common))               { continue; }
		if (m.common.faction_id != faction_id)        { continue; }
		if (m.common.area_id    != location_id)       { continue; }

		std::pair<gmActor::ActorId, int> entry = { m.common.actor_id,
		                                            m.common.current_hp };
		if (m.common.area_position == gmActor::AreaPosition::FRONTLINE)
			frontline.push_back(entry);
		else
			backline.push_back(entry);
	}
}

} // anonymous namespace

// ── TargetingFilter ───────────────────────────────────────────────────────────

std::vector<gmActor::ActorId> TargetingFilter::valid_targets(
	const gmActor::ActorStore& store,
	const LocationId&          location_id,
	const std::string&         target_faction) const
{
	std::vector<std::pair<gmActor::ActorId, int>> frontline;
	std::vector<std::pair<gmActor::ActorId, int>> backline;
	collect_targets(store, location_id, target_faction, frontline, backline);

	std::vector<gmActor::ActorId> result;

	// §15 Proiezione: if any frontline exists, only frontline is targetable
	if (!frontline.empty())
	{
		for (const auto& p : frontline) { result.push_back(p.first); }
	}
	else
	{
		for (const auto& p : backline) { result.push_back(p.first); }
	}

	return result;
}

gmActor::ActorId TargetingFilter::nearest_target(
	const gmActor::ActorStore& store,
	const LocationId&          location_id,
	const std::string&         target_faction) const
{
	std::vector<std::pair<gmActor::ActorId, int>> frontline;
	std::vector<std::pair<gmActor::ActorId, int>> backline;
	collect_targets(store, location_id, target_faction, frontline, backline);

	// §15: prefer frontline; among equals prefer lowest HP
	const std::vector<std::pair<gmActor::ActorId, int>>* pool =
		!frontline.empty() ? &frontline : &backline;

	if (pool->empty()) { return {}; }

	// Pick the entry with the lowest HP
	const auto min_it = std::min_element(
		pool->begin(), pool->end(),
		[](const std::pair<gmActor::ActorId, int>& a,
		   const std::pair<gmActor::ActorId, int>& b)
		{
			return a.second < b.second;
		});

	return min_it->first;
}

bool TargetingFilter::has_valid_target(
	const gmActor::ActorStore& store,
	const LocationId&          location_id,
	const std::string&         target_faction) const
{
	std::vector<std::pair<gmActor::ActorId, int>> frontline;
	std::vector<std::pair<gmActor::ActorId, int>> backline;
	collect_targets(store, location_id, target_faction, frontline, backline);
	return !frontline.empty() || !backline.empty();
}

} // namespace eldhom
