#ifndef GMACTOR_ACTORS_ALLYSTATE_HPP
#define GMACTOR_ACTORS_ALLYSTATE_HPP

/**
 * @file actors/AllyState.hpp
 * @brief Mutable runtime state for an allied non-player actor.
 *
 * Allies are non-player actors that cooperate with heroes.  They participate
 * in the timeline if `common.can_act == true`.
 *
 * V1 keeps this structure minimal.  Deck references, behavior trees, and
 * advanced equipment can be added in later versions.
 */

#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/core/Ids.hpp"

#include <vector>

namespace gmActor {

/**
 * @brief Mutable runtime state for an allied NPC.
 */
struct AllyState {
    ActorStateCommon common; ///< Shared actor state (common.kind == ActorKind::ALLY_NPC)

    std::vector<TraitId>        traits;        ///< Passive trait IDs
    std::vector<ItemInstanceId> carried_items; ///< Items carried by the ally
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_ALLYSTATE_HPP
