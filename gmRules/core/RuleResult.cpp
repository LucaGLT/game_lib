/**
 * @file core/RuleResult.cpp
 * @brief Implementation of RuleResult factory methods and accessors.
 */

#include "gmRules/core/RuleResult.hpp"

namespace gmRules {

RuleResult::RuleResult(bool valid, bool warning, RuleError error, std::string message)
    : valid_(valid), warning_(warning), error_(error), message_(std::move(message))
{}

RuleResult RuleResult::ok()
{
	return RuleResult(true, false, RuleError::NONE, "");
}

RuleResult RuleResult::fail(RuleError error, std::string message)
{
	return RuleResult(false, false, error, std::move(message));
}

RuleResult RuleResult::warning(std::string message)
{
	return RuleResult(true, true, RuleError::NONE, std::move(message));
}

bool RuleResult::valid() const      { return valid_; }
bool RuleResult::has_warning() const{ return warning_; }
RuleError RuleResult::error() const { return error_; }
const std::string& RuleResult::message() const { return message_; }

} // namespace gmRules
