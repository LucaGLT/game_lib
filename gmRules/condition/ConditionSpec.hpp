#ifndef GMRULES_CONDITION_CONDITIONSPEC_HPP
#define GMRULES_CONDITION_CONDITIONSPEC_HPP

/**
 * @file condition/ConditionSpec.hpp
 * @brief Data structure describing one condition (atomic or composite).
 *
 * If `children` is empty, the spec is atomic and is evaluated by matching
 * `type` against the `RuleContext`.
 *
 * If `children` is non-empty, the spec is composite and is evaluated by
 * applying `op` to the results of evaluating each child.
 */

#include "gmRules/condition/ConditionType.hpp"
#include "gmRules/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Describes a condition that can be evaluated against game state.
 */
struct ConditionSpec
{
    ConditionType type = ConditionType::ALWAYS; ///< Atomic condition type

    std::string subject_id; ///< Primary entity (actor ID, deck ID, location ID, etc.)
    std::string target_id;  ///< Secondary entity (used by LOCATION_IS_ADJACENT, etc.)
    std::string value;      ///< String operand (status ID, tag, zone name, etc.)
    int         amount = 0; ///< Numeric operand (HP threshold, card count, etc.)

    // Composite support
    std::vector<ConditionSpec> children;           ///< Sub-conditions (non-empty = composite)
    CompositeOperator          op = CompositeOperator::ALL_OF; ///< Composite operator
};

} // namespace gmRules

#endif // GMRULES_CONDITION_CONDITIONSPEC_HPP
