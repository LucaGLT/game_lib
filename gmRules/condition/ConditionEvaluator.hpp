#ifndef GMRULES_CONDITION_CONDITIONEVALUATOR_HPP
#define GMRULES_CONDITION_CONDITIONEVALUATOR_HPP

/**
 * @file condition/ConditionEvaluator.hpp
 * @brief Evaluates `ConditionSpec` trees against game state.
 *
 * `ConditionEvaluator` is **side-effect-free** — it never mutates
 * `RuleContext`.  It only calls `const` methods.
 */

#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleContext.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Evaluates condition trees against a read-only `RuleContext`.
 *
 * Stateless — all methods are `const`.
 */
class ConditionEvaluator
{
public:
    /**
     * @brief Evaluates a single condition (atomic or composite).
     *
     * @param condition Condition to evaluate.
     * @param ctx       Read-only game state.
     * @return          `RuleResult::ok()` if the condition holds,
     *                  `RuleResult::fail(CONDITION_FAILED, ...)` otherwise.
     */
    RuleResult evaluate(const ConditionSpec& condition,
                        const RuleContext& ctx) const;

    /**
     * @brief Evaluates all conditions as an implicit ALL_OF.
     *
     * Returns the first failure encountered.
     *
     * @param conditions Conditions to evaluate.
     * @param ctx        Read-only game state.
     * @return           `RuleResult::ok()` only if all conditions pass.
     */
    RuleResult evaluate_all(const std::vector<ConditionSpec>& conditions,
                            const RuleContext& ctx) const;
};

} // namespace gmRules

#endif // GMRULES_CONDITION_CONDITIONEVALUATOR_HPP
