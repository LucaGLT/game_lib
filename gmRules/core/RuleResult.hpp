#ifndef GMRULES_CORE_RULERESULT_HPP
#define GMRULES_CORE_RULERESULT_HPP

/**
 * @file core/RuleResult.hpp
 * @brief Generic result type for non-effect rule operations.
 *
 * Used by `ConditionEvaluator`, `TargetResolver` non-list operations,
 * `StatusEngine`, and `gmRulesEngine` methods that do not produce events.
 *
 * A result is either:
 * - **ok** — operation succeeded (possibly with warnings)
 * - **fail** — operation failed with a specific `RuleError`
 */

#include "gmRules/core/RuleError.hpp"

#include <string>

namespace gmRules {

/**
 * @brief Result of a rule operation that does not produce events.
 */
class RuleResult
{
public:
    // ── Factory methods ───────────────────────────────────────────────────────

    /** @brief Constructs a successful result with no message. */
    static RuleResult ok();

    /**
     * @brief Constructs a failure result.
     * @param error   Error code.
     * @param message Human-readable description.
     */
    static RuleResult fail(RuleError error, std::string message);

    /**
     * @brief Constructs a successful result with a warning message.
     * @param message Warning text.
     */
    static RuleResult warning(std::string message);

    // ── Queries ───────────────────────────────────────────────────────────────

    /** @brief Returns `true` if the operation succeeded (ok or warning). */
    bool valid() const;

    /** @brief Returns `true` if the result carries a warning message. */
    bool has_warning() const;

    /** @brief Returns the error code (`NONE` if the result is valid). */
    RuleError error() const;

    /** @brief Returns the error or warning message (may be empty). */
    const std::string& message() const;

private:
    bool        valid_     = true;
    bool        warning_   = false;
    RuleError   error_     = RuleError::NONE;
    std::string message_;

    RuleResult(bool valid, bool warning, RuleError error, std::string message);
};

} // namespace gmRules

#endif // GMRULES_CORE_RULERESULT_HPP
