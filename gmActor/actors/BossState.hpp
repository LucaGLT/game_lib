#ifndef GMACTOR_ACTORS_BOSSSTATE_HPP
#define GMACTOR_ACTORS_BOSSSTATE_HPP

/**
 * @file actors/BossState.hpp
 * @brief Optional boss extension data.
 *
 * In V1, a boss is modelled as:
 * ```
 * Boss = MonsterGroup (controller) + MonsterInstance (body)
 * ```
 *
 * `BossState` stores the association between the two and any boss-specific
 * runtime data (phase index, rage counter).  It does not replace the group
 * or instance state — both must also exist in the `ActorStore`.
 *
 * @note Boss phases are not implemented in `gmActor`.  The game engine stores
 *       the `phase_index` here and interprets its meaning.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Tags.hpp"

#include <vector>

namespace gmActor {

/**
 * @brief Optional boss-specific state, keyed by `controller_group_id`.
 */
struct BossState {
    MonsterInstanceId body_instance_id;    ///< Physical body (MonsterInstanceState key)
    MonsterGroupId    controller_group_id; ///< Timeline actor (MonsterGroupState key; also ActorStore key)

    int phase_index = 0; ///< Current boss phase (0 = initial phase)
    int rage        = 0; ///< Rage / escalation counter (engine-defined semantics)

    std::vector<ObjectiveId> linked_objectives; ///< Mission objectives linked to this boss
    std::vector<Tag>         tags;              ///< Classification tags
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_BOSSSTATE_HPP
