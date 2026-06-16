#ifndef GMRULES_STATUS_DURATION_HPP
#define GMRULES_STATUS_DURATION_HPP

/**
 * @file status/Duration.hpp
 * @brief Duration specification and runtime state for status instances.
 *
 * `DurationSpec` describes when a status should expire.
 * `DurationState` tracks runtime progress against the spec.
 *
 * Games that do not use rounds should prefer activation-based or
 * condition-based durations.
 */

#include "gmRules/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmRules {

// Forward declaration — ConditionSpec is defined in condition/ConditionSpec.hpp
struct ConditionSpec;

/**
 * @brief Classifies how long a status persists.
 */
enum class DurationType
{
    PERMANENT,                 ///< Never expires automatically
    UNTIL_REMOVED,             ///< Expires only when explicitly removed
    FOR_N_ACTIVATIONS,         ///< Decrements each activation of the owner
    UNTIL_NEXT_ACTIVATION,     ///< Expires at the start of the next activation
    UNTIL_TIME_REACHED,        ///< Expires when a numeric time value is reached
    WHILE_IN_LOCATION,         ///< Expires when the actor leaves a location
    WHILE_CONDITION_TRUE,      ///< Expires when the attached condition becomes false
    CUSTOM                     ///< Game-specific expiry logic
};

/**
 * @brief Describes the desired duration of a status.
 */
struct DurationSpec
{
    DurationType type   = DurationType::UNTIL_REMOVED; ///< Expiry rule
    int          amount = 0;                           ///< Activation count or time value
    std::string  value;                                ///< Location ID for WHILE_IN_LOCATION
    // ConditionSpec conditions are intentionally omitted in core structs to
    // avoid circular include chains.  WHILE_CONDITION_TRUE is resolved by
    // StatusEngine using the full condition tree stored in the host game state.
};

/**
 * @brief Runtime progress of a duration on a specific status instance.
 */
struct DurationState
{
    DurationSpec spec;         ///< Original spec (copied at apply time)
    int          remaining = 0;///< Remaining activations/time steps
    bool         expired   = false; ///< Set to true when the status should be removed
};

} // namespace gmRules

#endif // GMRULES_STATUS_DURATION_HPP
