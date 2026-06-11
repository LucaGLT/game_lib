#ifndef GMACTOR_ADAPTERS_GMFLOWACTORADAPTER_HPP
#define GMACTOR_ADAPTERS_GMFLOWACTORADAPTER_HPP

/**
 * @file adapters/GmFlowActorAdapter.hpp
 * @brief Builds gmFlow::Actor descriptors from gmActor state.
 *
 * This header bridges `gmActor` (mutable game state) and `gmFlow`
 * (immutable flow descriptors).  Include it only in translation units
 * that need the gmFlow integration.
 *
 * ## ActorKind → ActorType mapping
 *
 * | gmActor::ActorKind     | gmFlow::ActorType |
 * |------------------------|-------------------|
 * | HERO                   | PLAYER            |
 * | ALLY_NPC               | BOT               |
 * | MONSTER_INSTANCE       | BOT               |
 * | MONSTER_GROUP          | BOT               |
 * | BOSS                   | BOT               |
 * | MISSION_SYSTEM         | SYSTEM            |
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/core/Ids.hpp"

#include "gmFlow/actors/Actor.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"

#include <stdexcept>

namespace gmActor {

/**
 * @brief Maps a gmActor::ActorKind to the corresponding gmFlow::ActorType.
 *
 * @param kind gmActor actor kind.
 * @return     Matching gmFlow::ActorType.
 */
inline gmFlow::ActorType to_flow_actor_type(ActorKind kind)
{
    switch (kind) {
        case ActorKind::HERO:             return gmFlow::ActorType::PLAYER;
        case ActorKind::ALLY_NPC:         return gmFlow::ActorType::BOT;
        case ActorKind::MONSTER_INSTANCE: return gmFlow::ActorType::BOT;
        case ActorKind::MONSTER_GROUP:    return gmFlow::ActorType::BOT;
        case ActorKind::BOSS:             return gmFlow::ActorType::BOT;
        case ActorKind::MISSION_SYSTEM:   return gmFlow::ActorType::SYSTEM;
    }
    return gmFlow::ActorType::BOT; // unreachable — suppresses compiler warning
}

/**
 * @brief Builds a `gmFlow::Actor` from the common state of a regular actor.
 *
 * Valid for HERO, ALLY_NPC, and MONSTER_INSTANCE.
 *
 * @param store The actor store.
 * @param id    Actor ID.
 * @return      Immutable gmFlow::Actor descriptor.
 * @throws UnknownActorError if `id` is not found.
 * @throws InvalidActorKindError if the actor is a MONSTER_GROUP.
 */
inline gmFlow::Actor make_flow_actor(const ActorStore& store, const ActorId& id)
{
    const ActorStateCommon& c = store.common(id); // throws on MONSTER_GROUP (D1)
    gmFlow::Actor actor(c.actor_id, to_flow_actor_type(c.kind));
    actor.set_display_name(c.display_name);
    return actor;
}

/**
 * @brief Builds a `gmFlow::Actor` from a MonsterGroupState.
 *
 * Monster groups are timeline actors and need a flow descriptor even though
 * they do not have an `ActorStateCommon`.
 *
 * @param group The monster group state.
 * @return      Immutable gmFlow::Actor descriptor.
 */
inline gmFlow::Actor make_flow_actor_from_group(const MonsterGroupState& group)
{
    gmFlow::Actor actor(group.actor_id, gmFlow::ActorType::BOT);
    actor.set_display_name(group.display_name);
    return actor;
}

/**
 * @brief Registers all timeline-eligible actors from `store` into `registry`.
 *
 * Adds heroes, allies (can_act==true), monster groups (enabled), and the
 * mission system (enabled) to the provided gmFlow::ActorRegistry.
 *
 * @param store    Source of actor state.
 * @param registry Destination gmFlow registry.
 */
void populate_flow_registry(const ActorStore& store,
                             gmFlow::ActorRegistry& registry);

} // namespace gmActor

#endif // GMACTOR_ADAPTERS_GMFLOWACTORADAPTER_HPP
