#ifndef GMACTOR_ACTORS_ACTORQUERIES_HPP
#define GMACTOR_ACTORS_ACTORQUERIES_HPP

/**
 * @file actors/ActorQueries.hpp
 * @brief Pure free-function query helpers on ActorStore.
 *
 * All functions in this file are **const** with respect to the store.
 * None of them mutate actor state.
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/core/Ids.hpp"

#include <vector>

namespace gmActor {

// ── Classification predicates ─────────────────────────────────────────────────

/**
 * @brief Returns true if the actor is a hero.
 */
bool is_hero(const ActorStore& store, const ActorId& id);

/**
 * @brief Returns true if the actor is a monster group.
 */
bool is_monster_group(const ActorStore& store, const ActorId& id);

/**
 * @brief Returns true if the actor can currently be targeted.
 *
 * Valid for HERO, ALLY_NPC, and MONSTER_INSTANCE.
 * Always false for MONSTER_GROUP and MISSION_SYSTEM.
 */
bool is_targetable(const ActorStore& store, const ActorId& id);

/**
 * @brief Returns true if the actor can currently take an activation.
 *
 * For common-bearing actors: `common.can_act == true`.
 * For MONSTER_GROUP: `group.enabled == true && !group.removed`.
 */
bool can_act(const ActorStore& store, const ActorId& id);

// ── Filtered collections ──────────────────────────────────────────────────────

/**
 * @brief Returns IDs of all actors in the given faction.
 *
 * Searches heroes, allies, and monster instances.
 *
 * @param store   The actor store.
 * @param faction Faction ID to filter by.
 */
std::vector<ActorId> actors_by_faction(const ActorStore& store, const FactionId& faction);

/**
 * @brief Returns IDs of all actors in the given area.
 *
 * Equivalent to `ActorStore::actors_in_area()` but available as a free function.
 *
 * @param store The actor store.
 * @param area  Area ID to filter by.
 */
std::vector<ActorId> actors_in_area(const ActorStore& store, const AreaId& area);

/**
 * @brief Returns IDs of all heroes whose `life_state == ACTIVE`.
 *
 * @param store The actor store.
 */
std::vector<ActorId> living_heroes(const ActorStore& store);

/**
 * @brief Returns IDs of all actors that should appear on the current timeline.
 *
 * Equivalent to `ActorStore::timeline_actor_ids()` but available as a free
 * function and excludes actors where `enabled == false` or `removed == true`.
 *
 * @param store The actor store.
 */
std::vector<ActorId> enabled_timeline_actors(const ActorStore& store);

} // namespace gmActor

#endif // GMACTOR_ACTORS_ACTORQUERIES_HPP
