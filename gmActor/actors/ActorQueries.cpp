/**
 * @file actors/ActorQueries.cpp
 * @brief Stub implementation of ActorQueries free functions.
 */

#include "gmActor/actors/ActorQueries.hpp"

namespace gmActor {

bool is_hero(const ActorStore& store, const ActorId& id)
{
    // TODO Phase 4
    (void)store; (void)id;
    return false;
}

bool is_monster_group(const ActorStore& store, const ActorId& id)
{
    // TODO Phase 4
    (void)store; (void)id;
    return false;
}

bool is_targetable(const ActorStore& store, const ActorId& id)
{
    // TODO Phase 4
    (void)store; (void)id;
    return false;
}

bool can_act(const ActorStore& store, const ActorId& id)
{
    // TODO Phase 4
    (void)store; (void)id;
    return false;
}

std::vector<ActorId> actors_by_faction(const ActorStore& store, const FactionId& faction)
{
    // TODO Phase 4
    (void)store; (void)faction;
    return {};
}

std::vector<ActorId> actors_in_area(const ActorStore& store, const AreaId& area)
{
    return store.actors_in_area(area);
}

std::vector<ActorId> living_heroes(const ActorStore& store)
{
    // TODO Phase 4
    (void)store;
    return {};
}

std::vector<ActorId> enabled_timeline_actors(const ActorStore& store)
{
    return store.timeline_actor_ids();
}

} // namespace gmActor
