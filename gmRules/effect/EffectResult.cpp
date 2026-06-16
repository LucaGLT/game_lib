/**
 * @file effect/EffectResult.cpp
 * @brief Implementation of EffectResult.
 */

#include "gmRules/effect/EffectResult.hpp"

namespace gmRules {

EffectResult::EffectResult(bool succeeded, bool partial, std::string message)
    : succeeded_(succeeded), partial_(partial), message_(std::move(message))
{}

EffectResult EffectResult::success()
{
	return EffectResult(true, false, "");
}

EffectResult EffectResult::partial(std::vector<std::string> warnings)
{
	EffectResult r(true, true, "");
	r.warnings_ = std::move(warnings);
	return r;
}

EffectResult EffectResult::failure(std::string message)
{
	return EffectResult(false, false, std::move(message));
}

bool EffectResult::succeeded() const       { return succeeded_; }
bool EffectResult::partial_success() const { return partial_; }
const std::vector<RuleEvent>&   EffectResult::events()   const { return events_; }
const std::vector<std::string>& EffectResult::warnings() const { return warnings_; }
const std::string& EffectResult::message() const { return message_; }

void EffectResult::add_event(RuleEvent event)
{
	events_.push_back(std::move(event));
}

void EffectResult::add_warning(std::string warning)
{
	warnings_.push_back(std::move(warning));
	partial_ = true;
}

} // namespace gmRules
