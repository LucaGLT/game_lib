#ifndef GMACTOR_ACTORS_MONSTERINSTANCESTATE_HPP
#define GMACTOR_ACTORS_MONSTERINSTANCESTATE_HPP

/**
 * @file actors/MonsterInstanceState.hpp
 * @brief Mutable runtime state for an individual monster body on the map.
 *
 * A `MonsterInstanceState` represents one physical miniature or targetable
 * enemy.  It is owned/controlled by a `MonsterGroupState`.
 *
 * ## Monster model
 * | Concept         | Who holds it             |
 * |-----------------|--------------------------|
 * | Turn / timeline | `MonsterGroupState`      |
 * | HP / targeting  | `MonsterInstanceState`   |
 * | Behavior deck   | `MonsterGroupState`      |
 */

#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Mutable runtime state for one monster instance (physical body).
 */
struct MonsterInstanceState {
    ActorStateCommon common; ///< Shared actor state (common.kind == ActorKind::MONSTER_INSTANCE)

    MonsterTypeId  monster_type_id; ///< Archetype / type reference
    MonsterGroupId group_id;        ///< Owning group identifier

    bool elite     = false; ///< Elite variant flag
    bool boss_part = false; ///< True if this instance is part of a boss encounter

    int base_damage   = 1; ///< Default damage before modifiers
    int base_movement = 2; ///< Default movement before modifiers

    std::vector<TraitId> traits; ///< Passive trait IDs
    std::string          loot_ref; ///< Opaque loot table reference (engine-side)
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_MONSTERINSTANCESTATE_HPP
