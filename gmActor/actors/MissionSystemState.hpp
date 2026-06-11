#ifndef GMACTOR_ACTORS_MISSIONSYSTEMSTATE_HPP
#define GMACTOR_ACTORS_MISSIONSYSTEMSTATE_HPP

/**
 * @file actors/MissionSystemState.hpp
 * @brief State for the mission/system scripted actor.
 *
 * The mission system actor represents scenario logic, environmental effects,
 * scripted events, traps, and other non-physical sources.  It may appear as
 * `source_id` for statuses, modifiers, damage, and events.
 *
 * A mission system actor normally has:
 * - no HP
 * - no area
 * - no inventory or equipment
 * - no deck zones
 *
 * It may or may not appear on the timeline (use `enabled` to control this).
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Tags.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Minimal state for the mission/system scripted actor.
 */
struct MissionSystemState {
    ActorId     actor_id     = "system_mission"; ///< Default ID; override per scenario
    std::string display_name = "Mission System"; ///< Human-readable label

    bool enabled = true; ///< False if the system actor is not active this scenario

    std::vector<Tag> tags; ///< Classification tags
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_MISSIONSYSTEMSTATE_HPP
