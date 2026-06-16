#ifndef GMRULES_STATUS_STATUSINSTANCE_HPP
#define GMRULES_STATUS_STATUSINSTANCE_HPP

/**
 * @file status/StatusInstance.hpp
 * @brief Mutable runtime instance of a status applied to an actor.
 */

#include "gmRules/core/Ids.hpp"
#include "gmRules/status/Duration.hpp"

#include <string>

namespace gmRules {

/**
 * @brief One active application of a `StatusDefinition` on an actor.
 */
struct StatusInstance
{
    StatusInstanceId instance_id;    ///< Unique runtime identifier for this application
    StatusId         status_id;      ///< References a StatusDefinition
    ActorId          owner_actor_id; ///< Actor that owns this status
    std::string      source_id;      ///< Who applied the status (actor, item, trap, etc.)

    int          stacks = 1;         ///< Current stack count (≥ 1)
    DurationState duration;          ///< Runtime duration tracking
};

} // namespace gmRules

#endif // GMRULES_STATUS_STATUSINSTANCE_HPP
