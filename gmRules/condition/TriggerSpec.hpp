#ifndef GMRULES_CONDITION_TRIGGERSPEC_HPP
#define GMRULES_CONDITION_TRIGGERSPEC_HPP

/**
 * @file condition/TriggerSpec.hpp
 * @brief Event-based activation specification for reactions and status effects.
 *
 * `TriggerSpec` describes what kind of game event must occur for a
 * rule to activate.  It is intentionally event-system-agnostic.
 * A game-specific adapter translates `gmFlow`, `gmDispatch`, or custom
 * events into trigger checks.
 */

#include "gmRules/condition/ConditionSpec.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Classifies what kind of event activates a trigger.
 */
enum class TriggerType
{
    ON_ACTION_SUBMITTED,  ///< An action was submitted to the flow controller
    ON_ACTION_COMPLETED,  ///< An action finished executing
    ON_CARD_PLAYED,       ///< A card was played from hand
    ON_ACTOR_DAMAGED,     ///< An actor received damage
    ON_ACTOR_MOVED,       ///< An actor moved to a new location
    ON_STATUS_APPLIED,    ///< A status was applied to an actor
    ON_TIME_REACHED,      ///< A specific time value was reached
    ON_LOCATION_ENTERED,  ///< An actor entered a location
    CUSTOM                ///< Game-specific trigger
};

/**
 * @brief Describes when a rule should fire automatically.
 */
struct TriggerSpec
{
    TriggerType                type       = TriggerType::CUSTOM; ///< Event type
    std::vector<ConditionSpec> conditions; ///< Additional conditions that must hold
};

} // namespace gmRules

#endif // GMRULES_CONDITION_TRIGGERSPEC_HPP
