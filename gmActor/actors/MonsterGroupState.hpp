#ifndef GMACTOR_ACTORS_MONSTERGROUPSTATE_HPP
#define GMACTOR_ACTORS_MONSTERGROUPSTATE_HPP

/**
 * @file actors/MonsterGroupState.hpp
 * @brief State for a monster group that acts as a unit on the timeline.
 *
 * ## Design decision D1
 * `MonsterGroupState` does **not** embed `ActorStateCommon`.  Groups do not
 * have HP, area position, or per-instance statuses.  Calling
 * `ActorStore::common(group_id)` throws `InvalidActorKindError`.
 *
 * Groups are actors for timeline purposes only.  Individual bodies are
 * `MonsterInstanceState` entries in the same `ActorStore`.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Tags.hpp"
#include "gmActor/modifiers/Modifier.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Mutable runtime state for a monster group.
 *
 * A group controls one or more `MonsterInstanceState` members and acts as
 * the timeline actor.
 */
struct MonsterGroupState {
    ActorId        actor_id;      ///< Unique group actor ID (matches gmFlow::ActorId)
    MonsterGroupId group_id;      ///< Canonical group identifier
    MonsterTypeId  monster_type_id; ///< Archetype / type reference
    std::string    display_name;  ///< Human-readable label

    // ── Participation ─────────────────────────────────────────────────────────
    bool enabled = true;  ///< False if the group is not yet in play
    bool removed = false; ///< True if the group has been eliminated or withdrawn

    // ── Timeline ordering ─────────────────────────────────────────────────────
    int timeline_position = 0; ///< Primary initiative
    int tie_break_rank    = 0; ///< Secondary ordering for equal positions

    // ── Members ───────────────────────────────────────────────────────────────
    std::vector<MonsterInstanceId> members; ///< IDs of controlled monster instances

    // ── Behavior deck references ──────────────────────────────────────────────
    DeckInstanceId behavior_deck_id;        ///< gmDeck instance for behavior cards
    CardId         active_behavior_card_id; ///< Currently active behavior card
    DeckInstanceId behavior_discard_id;     ///< gmDeck discard instance

    // ── Group-level effects ───────────────────────────────────────────────────
    std::vector<ModifierInstance> active_group_modifiers; ///< Modifiers on the group
    std::vector<Tag>              tags;                   ///< Classification tags
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_MONSTERGROUPSTATE_HPP
