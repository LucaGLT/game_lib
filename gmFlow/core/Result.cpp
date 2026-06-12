/**
 * @file core/Result.cpp
 * @brief Implementations of ValidationResult, ActionResult, and StepResult.
 */

#include "gmFlow/core/Result.hpp"

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// ValidationResult
// ─────────────────────────────────────────────────────────────────────────────

ValidationResult ValidationResult::ok()
{
    ValidationResult r;
    r.valid_   = true;
    r.error_   = ValidationError::NONE;
    return r;
}

ValidationResult ValidationResult::fail(ValidationError error, std::string message)
{
    ValidationResult r;
    r.valid_   = false;
    r.error_   = error;
    r.message_ = std::move(message);
    return r;
}

bool                   ValidationResult::valid()   const { return valid_;   }
ValidationError        ValidationResult::error()   const { return error_;   }
const std::string&     ValidationResult::message() const { return message_; }

// ─────────────────────────────────────────────────────────────────────────────
// ActionResult
// ─────────────────────────────────────────────────────────────────────────────

ActionResult ActionResult::success()
{
    ActionResult r;
    r.succeeded_ = true;
    return r;
}

ActionResult ActionResult::failure(std::string reason)
{
    ActionResult r;
    r.succeeded_ = false;
    r.reason_    = std::move(reason);
    return r;
}

bool               ActionResult::succeeded() const { return succeeded_; }
const std::string& ActionResult::reason()    const { return reason_;    }

// ─────────────────────────────────────────────────────────────────────────────
// StepResult
// ─────────────────────────────────────────────────────────────────────────────

StepResult StepResult::done()
{
    StepResult r;
    r.complete_    = true;
    r.needs_input_ = false;
    r.failed_      = false;
    return r;
}

StepResult StepResult::needs_input(std::string prompt)
{
    StepResult r;
    r.complete_    = false;
    r.needs_input_ = true;
    r.failed_      = false;
    r.prompt_      = std::move(prompt);
    return r;
}

StepResult StepResult::failed(std::string reason)
{
    StepResult r;
    r.complete_    = false;
    r.needs_input_ = false;
    r.failed_      = true;
    r.reason_      = std::move(reason);
    return r;
}

bool               StepResult::complete()    const { return complete_;    }
bool               StepResult::needs_input() const { return needs_input_; }
bool               StepResult::failed()      const { return failed_;      }
const std::string& StepResult::prompt()      const { return prompt_;      }
const std::string& StepResult::reason()      const { return reason_;      }

} // namespace gmFlow
