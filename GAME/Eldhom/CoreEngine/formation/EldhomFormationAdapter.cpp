/**
 * @file formation/EldhomFormationAdapter.cpp
 * @brief Implementation of EldhomFormationAdapter.
 */

#include "GAME/Eldhom/CoreEngine/formation/EldhomFormationAdapter.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/core/Enums.hpp"

#include <vector>

namespace eldhom {

EldhomFormationAdapter::EldhomFormationAdapter()
	: _validator(gmActor::FormationRules{})
{
}

// ── Helpers (file-local) ──────────────────────────────────────────────────────

namespace {

/**
 * @brief Counts frontline and backline live actors of a faction in a location.
 *
 * Only `MonsterInstanceState` and `HeroState` actors contribute to formation
 * counts.  MonsterGroups are excluded (they are timeline actors, not positional).
 *
 * @param store       The actor store to query.
 * @param location_id Target location area_id.
 * @param faction_id  Faction whose actors are counted.
 * @param[out] front  Number of FRONTLINE actors found.
 * @param[out] back   Number of BACKLINE actors found.
 */
void count_formation(
	const gmActor::ActorStore& store,
	const std::string&         location_id,
	const std::string&         faction_id,
	int&                       front,
	int&                       back)
{
	front = 0;
	back  = 0;

	// Count heroes
	for (const auto& kv : store.heroes())
	{
		const gmActor::HeroState& h = kv.second;
		if (h.common.faction_id        != faction_id)  { continue; }
		if (h.common.area_id           != location_id) { continue; }
		if (h.common.removed)                          { continue; }
		if (h.common.life_state == gmActor::ActorLifeState::DEAD)    { continue; }
		if (h.common.life_state == gmActor::ActorLifeState::REMOVED) { continue; }

		if (h.common.area_position == gmActor::AreaPosition::FRONTLINE) { ++front; }
		else if (h.common.area_position == gmActor::AreaPosition::BACKLINE) { ++back; }
	}

	// Count monster instances
	for (const auto& kv : store.monster_instances())
	{
		const gmActor::MonsterInstanceState& m = kv.second;
		if (m.common.faction_id        != faction_id)  { continue; }
		if (m.common.area_id           != location_id) { continue; }
		if (m.common.removed)                          { continue; }
		if (m.common.life_state == gmActor::ActorLifeState::DEAD)    { continue; }
		if (m.common.life_state == gmActor::ActorLifeState::REMOVED) { continue; }

		if (m.common.area_position == gmActor::AreaPosition::FRONTLINE) { ++front; }
		else if (m.common.area_position == gmActor::AreaPosition::BACKLINE) { ++back; }
	}
}

} // anonymous namespace

// ── EldhomFormationAdapter implementation ────────────────────────────────────

FormationCheckResult EldhomFormationAdapter::check(
	const gmActor::ActorStore& store,
	const LocationId&          location_id,
	const std::string&         faction_id) const
{
	FormationCheckResult result;
	result.location_id = location_id;
	result.faction_id  = faction_id;

	int front = 0;
	int back  = 0;
	count_formation(store, location_id, faction_id, front, back);

	if (front == 0 && back > 0)
	{
		result.valid           = false;
		result.scompaginamento = true;
		result.overflow        = back;
		result.message         =
			"Scompaginamento: frontline vuota, " +
			std::to_string(back) + " attore/i backline forzato/i avanti.";
	}
	else
	{
		result.valid    = _validator.is_valid(front, back);
		result.overflow = _validator.backline_overflow(front, back);
		if (!result.valid)
		{
			result.message =
				"Formazione non valida: PL=" + std::to_string(front) +
				" RG=" + std::to_string(back) +
				" (overflow=" + std::to_string(result.overflow) + ")";
		}
	}

	return result;
}

void EldhomFormationAdapter::resolve_scompaginamento(
	gmActor::ActorStore& store,
	const LocationId&    location_id,
	const std::string&   faction_id) const
{
	// Heroes
	for (auto& kv : store.heroes())
	{
		gmActor::HeroState& h = store.hero(kv.first);
		if (h.common.faction_id  != faction_id)                           { continue; }
		if (h.common.area_id     != location_id)                          { continue; }
		if (h.common.removed)                                             { continue; }
		if (h.common.area_position != gmActor::AreaPosition::BACKLINE)    { continue; }

		h.common.area_position = gmActor::AreaPosition::FRONTLINE;
	}

	// Monster instances
	for (auto& kv : store.monster_instances())
	{
		gmActor::MonsterInstanceState& m = store.monster_instance(kv.first);
		if (m.common.faction_id  != faction_id)                           { continue; }
		if (m.common.area_id     != location_id)                          { continue; }
		if (m.common.removed)                                             { continue; }
		if (m.common.area_position != gmActor::AreaPosition::BACKLINE)    { continue; }

		m.common.area_position = gmActor::AreaPosition::FRONTLINE;
	}
}

} // namespace eldhom
