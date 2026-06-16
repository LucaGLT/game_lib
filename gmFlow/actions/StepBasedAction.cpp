/**
 * @file actions/StepBasedAction.cpp
 * @brief Implementation of gmFlow::StepBasedAction.
 */

#include "gmFlow/actions/StepBasedAction.hpp"
#include "gmFlow/core/GameContext.hpp"

#include <stdexcept>

namespace gmFlow {

bool StepBasedAction::is_async()      const { return false; }
bool StepBasedAction::requires_turn() const { return true;  }
bool StepBasedAction::is_multi_step() const { return true;  }

ValidationResult StepBasedAction::validate(const GameContext& ctx) const
{
    if (_steps.empty()) {
        return ValidationResult::fail(
            ValidationError::RULE_VIOLATION,
            "StepBasedAction has no registered steps.");
    }
    if (_current_step >= _steps.size()) {
        return ValidationResult::ok();  // All steps done — nothing left to validate.
    }
    if (!_steps[_current_step]->can_enter(ctx)) {
        return ValidationResult::fail(
            ValidationError::RULE_VIOLATION,
            "Step '" + _steps[_current_step]->id() + "' cannot be entered.");
    }
    return ValidationResult::ok();
}

ActionResult StepBasedAction::execute(GameContext& ctx)
{
    return execute(ctx, StepInput{});
}

ActionResult StepBasedAction::execute(GameContext& ctx, const StepInput& input)
{
    // TODO: Phase 4.5 — emit EVT_ACTION_STARTED on first call; emit per-step events.
    if (_current_step >= _steps.size()) {
        return ActionResult::success();  // Already complete.
    }

    IActionStep& step = *_steps[_current_step];

    if (!step.can_enter(ctx)) {
        return ActionResult::failure(
            "Step '" + step.id() + "' cannot be entered.");
    }

    StepResult result = step.execute(ctx, input);

    if (result.failed()) {
        return ActionResult::failure(result.reason());
    }

    if (result.complete()) {
        ++_current_step;
        if (_current_step >= _steps.size()) {
            // All steps done.
            return ActionResult::success();
        }
    }

    // Step not yet complete (needs_input or ongoing) — not an error.
    return ActionResult::success();
}

std::size_t StepBasedAction::current_step_index() const
{
    return _current_step;
}

std::size_t StepBasedAction::step_count() const
{
    return _steps.size();
}

void StepBasedAction::add_step(std::unique_ptr<IActionStep> step)
{
    if (!step) {
        throw std::invalid_argument("StepBasedAction::add_step(): step must not be null");
    }
    _steps.push_back(std::move(step));
}

} // namespace gmFlow
