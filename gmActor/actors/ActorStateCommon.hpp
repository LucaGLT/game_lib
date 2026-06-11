#ifndef GMACTOR_ACTORS_ACTORSTATECOMMON_HPP
#define GMACTOR_ACTORS_ACTORSTATECOMMON_HPP

/**
 * @file actors/ActorStateCommon.hpp
 * @brief Mutable runtime state shared by all targetable and activatable actors.
 *
 * `ActorStateCommon` is embedded inside every concrete actor state struct
 * (HeroState, AllyState, MonsterInstanceState) except MonsterGroupState,
 * which has its own timeline fields.
 *
 * @invariant `current_hp` is always in `[0, max_hp]` when modified through
 *            the helpers in `stats/Health.hpp`.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/core/Tags.hpp"
#include "gmActor/statuses/StatusInstance.hpp"
#include "gmActor/modifiers/Modifier.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Mutable runtime state common to all actor-like entities.
 *
 * This structure stores only actor-related state.  It does not own flow,
 * deck, map, or rule-resolution logic.
 *
 * ## Field groups
 * | Group          | Fields                                                    |
 * |----------------|-----------------------------------------------------------|
 * | Identity       | `actor_id`, `kind`, `display_name`, `faction_id`          |
 * | Participation  | `enabled`, `removed`, `can_act`, `can_be_targeted`        |
 * | Lifecycle      | `life_state`                                              |
 * | Timeline       | `timeline_position`, `tie_break_rank`                     |
 * | Location       | `area_id`, `area_position`                                |
 * | Health         | `current_hp`, `max_hp`                                    |
 * | Effects        | `statuses`, `active_modifiers`, `tags`                    |
 */
struct ActorStateCommon {
    // ── Identity ──────────────────────────────────────────────────────────────
    ActorId      actor_id;                           ///< Matches a gmFlow::ActorId
    ActorKind    kind = ActorKind::HERO;             ///< Actor classification
    std::string  display_name;                       ///< Human-readable label
    FactionId    faction_id;                         ///< Faction / allegiance group

    // ── Participation flags ───────────────────────────────────────────────────
    bool enabled          = true;  ///< False if the actor is inactive (not yet in play)
    bool removed          = false; ///< True if the actor has been removed from the scenario
    bool can_act          = true;  ///< False if the actor cannot take activations
    bool can_be_targeted  = true;  ///< False if the actor cannot be targeted by attacks/effects

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    ActorLifeState life_state = ActorLifeState::ACTIVE; ///< Coarse life state

    // ── Timeline ordering ─────────────────────────────────────────────────────
    int timeline_position = 0; ///< Primary initiative (lower = earlier)
    int tie_break_rank    = 0; ///< Secondary ordering for equal positions

    // ── Location ──────────────────────────────────────────────────────────────
    AreaId       area_id;                               ///< Current area (gmMap reference)
    AreaPosition area_position = AreaPosition::NONE;    ///< Front/back rank within area

    // ── Health ────────────────────────────────────────────────────────────────
    int current_hp = 0; ///< Current hit points (clamped to [0, max_hp])
    int max_hp     = 0; ///< Maximum hit points (0 = no health mechanic)

    // ── Active effects ────────────────────────────────────────────────────────
    std::vector<StatusInstance>   statuses;          ///< Active status effects
    std::vector<ModifierInstance> active_modifiers;  ///< Active modifiers (not via status)
    std::vector<Tag>              tags;              ///< Runtime classification tags
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_ACTORSTATECOMMON_HPP
