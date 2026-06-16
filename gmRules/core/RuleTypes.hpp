#ifndef GMRULES_CORE_RULETYPES_HPP
#define GMRULES_CORE_RULETYPES_HPP

/**
 * @file core/RuleTypes.hpp
 * @brief Exception hierarchy for gmRules.
 *
 * Exceptions are thrown only for programming-level errors (e.g. calling an
 * unimplemented method, constructing an invalid spec at load time).
 * For runtime rule failures, use `RuleResult` or `EffectResult` instead.
 */

#include <stdexcept>
#include <string>

namespace gmRules {

/** @brief Base exception for all gmRules errors. */
class ERulesError : public std::runtime_error
{
public:
    explicit ERulesError(const std::string& msg)
        : std::runtime_error("gmRules: " + msg) {}
};

/** @brief Thrown when target resolution encounters a structural problem. */
class ETargetError : public ERulesError
{
public:
    explicit ETargetError(const std::string& msg) : ERulesError(msg) {}
};

/** @brief Thrown when a condition spec is structurally invalid. */
class EConditionError : public ERulesError
{
public:
    explicit EConditionError(const std::string& msg) : ERulesError(msg) {}
};

/** @brief Thrown when an effect spec is structurally invalid. */
class EEffectError : public ERulesError
{
public:
    explicit EEffectError(const std::string& msg) : ERulesError(msg) {}
};

/** @brief Thrown when a status spec is structurally invalid. */
class EStatusError : public ERulesError
{
public:
    explicit EStatusError(const std::string& msg) : ERulesError(msg) {}
};

} // namespace gmRules

#endif // GMRULES_CORE_RULETYPES_HPP
