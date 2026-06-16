/**
 * @file actors/ActorQueries.cpp
 * @brief Stub implementation of ActorQueries free functions.
 */

#include "gmActor/actors/ActorQueries.hpp"

namespace gmActor {

bool is_hero(const ActorStore& store, const ActorId& id)
{
	return store.has_actor(id) && store.actor_kind(id) == ActorKind::HERO;
}

bool is_monster_group(const ActorStore& store, const ActorId& id)
{
	return store.has_actor(id) && store.actor_kind(id) == ActorKind::MONSTER_GROUP;
}

bool is_targetable(const ActorStore& store, const ActorId& id)
{
	if (!store.has_actor(id)) return false;
	ActorKind k = store.actor_kind(id);
	if (k == ActorKind::MONSTER_GROUP || k == ActorKind::MISSION_SYSTEM) return false;
	return store.common(id).can_be_targeted;
}

bool can_act(const ActorStore& store, const ActorId& id)
{
	if (!store.has_actor(id)) return false;
	if (store.actor_kind(id) == ActorKind::MONSTER_GROUP)
	{
		const MonsterGroupState& g = store.monster_group(id);
		return g.enabled && !g.removed;
	}
	return store.common(id).can_act;
}

std::vector<ActorId> actors_by_faction(const ActorStore& store, const FactionId& faction)
{
	std::vector<ActorId> result;
	for (const ActorId& id : store.timeline_actor_ids())
	{
		ActorKind k = store.actor_kind(id);
		if (k == ActorKind::MONSTER_GROUP || k == ActorKind::MISSION_SYSTEM) continue;
		if (store.common(id).faction_id == faction)
			result.push_back(id);
	}
	return result;
}

std::vector<ActorId> actors_in_area(const ActorStore& store, const AreaId& area)
{
	return store.actors_in_area(area);
}

std::vector<ActorId> living_heroes(const ActorStore& store)
{
	std::vector<ActorId> result;
	for (const ActorId& id : store.timeline_actor_ids())
	{
		if (store.actor_kind(id) != ActorKind::HERO) continue;
		if (store.common(id).life_state == ActorLifeState::ACTIVE)
			result.push_back(id);
	}
	return result;
}

std::vector<ActorId> enabled_timeline_actors(const ActorStore& store)
{
	return store.timeline_actor_ids();
}

} // namespace gmActor
