#ifndef GMRULES_STATUS_STACKINGPOLICY_HPP
#define GMRULES_STATUS_STACKINGPOLICY_HPP

/**
 * @file status/StackingPolicy.hpp
 * @brief Describes how duplicate status applications are handled.
 */

namespace gmRules {

/**
 * @brief Determines what happens when the same status is applied twice.
 */
enum class StackingMode
{
    REFRESH_DURATION, ///< Reset duration of existing instance; keep stacks at 1
    ADD_STACK,        ///< Increment stacks up to `max_stacks`
    ONE_ONLY,         ///< Discard the new application; keep existing
    REPLACE,          ///< Remove existing instance and apply fresh
    UNIQUE_BY_SOURCE  ///< One instance per source — refresh if same source, add if different
};

/**
 * @brief Policy controlling duplicate application of a status.
 */
struct StackingPolicy
{
    StackingMode mode       = StackingMode::REFRESH_DURATION; ///< Behaviour on re-apply
    int          max_stacks = 1; ///< Maximum stack count (for ADD_STACK mode)
};

} // namespace gmRules

#endif // GMRULES_STATUS_STACKINGPOLICY_HPP
