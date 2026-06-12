#ifndef GMFLOW_SESSIONDEFINITION_HPP
#define GMFLOW_SESSIONDEFINITION_HPP

/**
 * @file campaign/SessionDefinition.hpp
 * @brief Metadata and unlock conditions for a campaign session.
 *
 * A SessionDefinition is a static descriptor — it never changes during a
 * campaign run.  The @ref Campaign uses it to decide which sessions are
 * available and to display metadata to the player before starting a session.
 *
 * ### Example
 * @code
 *   gmFlow::SessionDefinition def;
 *   def.session_id      = "scenario_3";
 *   def.display_name    = "The Witch Lord's Lair";
 *   def.description     = "Final confrontation — defeat the Witch Lord to win.";
 *   def.unlock_requires = {"scenario_2"};  // unlocked after completing scenario 2
 *   def.initial_unlock  = false;
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmFlow {

/**
 * @struct SessionDefinition
 * @brief Static metadata describing one session in a campaign.
 */
struct SessionDefinition {
    /// @brief Unique identifier; must match the SessionId used when running the session.
    SessionId session_id;

    /// @brief Human-readable session name shown in the campaign menu.
    std::string display_name;

    /// @brief Optional flavour text or objective summary shown before the session starts.
    std::string description;

    /// @brief Session IDs that must be completed before this session unlocks.
    /// An empty list means the session has no completion prerequisites.
    std::vector<SessionId> unlock_requires;

    /// @brief If true, the session is unlocked at campaign start regardless of unlock_requires.
    bool initial_unlock = false;
};

} // namespace gmFlow

#endif // GMFLOW_SESSIONDEFINITION_HPP
