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
    r._valid   = true;
    r._error   = ValidationError::NONE;
    return r;
}

ValidationResult ValidationResult::fail(ValidationError error, std::string message)
{
    ValidationResult r;
    r._valid   = false;
    r._error   = error;
    r._message = std::move(message);
    return r;
}

bool                   ValidationResult::valid()   const { return _valid;   }
ValidationError        ValidationResult::error()   const { return _error;   }
const std::string&     ValidationResult::message() const { return _message; }

// ─────────────────────────────────────────────────────────────────────────────
// ActionResult
// ─────────────────────────────────────────────────────────────────────────────

ActionResult ActionResult::success()
{
    ActionResult r;
    r._succeeded = true;
    return r;
}

ActionResult ActionResult::failure(std::string reason)
{
    ActionResult r;
    r._succeeded = false;
    r._reason    = std::move(reason);
    return r;
}

bool               ActionResult::succeeded() const { return _succeeded; }
const std::string& ActionResult::reason()    const { return _reason;    }

// ─────────────────────────────────────────────────────────────────────────────
// StepResult
// ─────────────────────────────────────────────────────────────────────────────

StepResult StepResult::done()
{
    StepResult r;
    r._complete    = true;
    r._needs_input = false;
    r._failed      = false;
    return r;
}

StepResult StepResult::needs_input(std::string prompt)
{
    StepResult r;
    r._complete    = false;
    r._needs_input = true;
    r._failed      = false;
    r._prompt      = std::move(prompt);
    return r;
}

StepResult StepResult::failed(std::string reason)
{
    StepResult r;
    r._complete    = false;
    r._needs_input = false;
    r._failed      = true;
    r._reason      = std::move(reason);
    return r;
}

bool               StepResult::complete()    const { return _complete;    }
bool               StepResult::needs_input() const { return _needs_input; }
bool               StepResult::failed()      const { return _failed;      }
const std::string& StepResult::prompt()      const { return _prompt;      }
const std::string& StepResult::reason()      const { return _reason;      }

} // namespace gmFlow
