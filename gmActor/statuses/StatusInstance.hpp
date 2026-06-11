#ifndef GMACTOR_STATUSES_STATUSINSTANCE_HPP
#define GMACTOR_STATUSES_STATUSINSTANCE_HPP

/**
 * @file statuses/StatusInstance.hpp
 * @brief Mutable runtime application of a status to an actor.
 *
 * A `StatusInstance` tracks all runtime state for one active status on one
 * actor.  It may carry embedded `ModifierInstance` objects that are evaluated
 * alongside the actor's regular modifiers.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/modifiers/Modifier.hpp"

#include <vector>

namespace gmActor {

/**
 * @brief Mutable runtime state for one active status on an actor.
 *
 * @note Embedded modifiers are informational — the game engine is responsible
 * for applying them via `apply_modifiers()` when evaluating stats.
 */
struct StatusInstance {
    StatusId     id;                                              ///< Matches a StatusDefinition id
    SourceId     source_id;                                       ///< Who applied this status
    int          stacks = 1;                                      ///< Current stack count (≥ 1)
    ModifierDurationKind duration_kind = ModifierDurationKind::MANUAL_REMOVE; ///< Expiry rule
    int          expires_at_time = -1;                            ///< Tick for UNTIL_TIME, else -1
    std::vector<ModifierInstance> modifiers;                      ///< Embedded modifiers
};

} // namespace gmActor

#endif // GMACTOR_STATUSES_STATUSINSTANCE_HPP
