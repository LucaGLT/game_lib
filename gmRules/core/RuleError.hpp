#ifndef GMRULES_CORE_RULEERROR_HPP
#define GMRULES_CORE_RULEERROR_HPP

/**
 * @file core/RuleError.hpp
 * @brief Error code enumeration for rule operations.
 *
 * `RuleError` is used in `RuleResult` and `EffectResult` to distinguish
 * the kind of failure without requiring exception propagation.
 */

namespace gmRules {

/**
 * @brief Classifies the kind of failure in a rule operation.
 */
enum class RuleError
{
    NONE,                  ///< No error — operation succeeded
    UNKNOWN_ACTOR,         ///< Referenced actor not found in context
    UNKNOWN_LOCATION,      ///< Referenced location not found in context
    UNKNOWN_CARD,          ///< Referenced card not found
    UNKNOWN_DECK,          ///< Referenced deck not found
    UNKNOWN_ITEM,          ///< Referenced item not found
    UNKNOWN_STATUS,        ///< Referenced status not found
    INVALID_TARGET,        ///< Target spec resolved to zero valid targets
    CONDITION_FAILED,      ///< A required condition was not satisfied
    EFFECT_FAILED,         ///< An effect could not be applied
    UNSUPPORTED_EFFECT,    ///< Effect type not yet implemented
    UNSUPPORTED_CONDITION, ///< Condition type not yet implemented
    RULE_VIOLATION,        ///< An explicit rule constraint was violated
    CONTEXT_ERROR,         ///< RuleContext reported an internal error
    CUSTOM_ERROR           ///< Game-specific or unclassified error
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEERROR_HPP
