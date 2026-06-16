/**
 * @file target/TargetResult.cpp
 * @brief Implementation of TargetResult.
 */

#include "gmRules/target/TargetResult.hpp"

namespace gmRules {

TargetResult::TargetResult(bool valid, std::vector<TargetRef> targets, std::string message)
    : valid_(valid), targets_(std::move(targets)), message_(std::move(message))
{}

TargetResult TargetResult::success(std::vector<TargetRef> targets)
{
	return TargetResult(true, std::move(targets), "");
}

TargetResult TargetResult::failure(std::string message)
{
	return TargetResult(false, {}, std::move(message));
}

bool TargetResult::valid() const { return valid_; }
const std::vector<TargetRef>& TargetResult::targets() const { return targets_; }
const std::string& TargetResult::message() const { return message_; }

} // namespace gmRules
