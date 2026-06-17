#ifndef GMFLOW_RESULT_HPP
#define GMFLOW_RESULT_HPP

/**
 * @file core/Result.hpp
 * @brief Result and error types returned by action validation and execution.
 *
 * This header declares four value types:
 * - @ref ValidationError  — enumerated reason codes for validation failures.
 * - @ref ValidationResult — returned by IAction::validate().
 * - @ref ActionResult     — returned by IAction::execute().
 * - @ref StepResult       — returned by IActionStep::execute().
 * - @ref StepInput        — opaque input bundle passed to IActionStep::execute().
 */

#include <string>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// ValidationError
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ValidationError
 * @brief Reason codes for a failed @ref ValidationResult.
 */
enum class ValidationError {
    NONE,                    ///< No error (used in successful results).
    NOT_ACTOR_TURN,          ///< The submitting actor is not allowed to act now.
    PHASE_DOES_NOT_ALLOW,    ///< The current phase does not permit this action.
    ACTION_WINDOW_CLOSED,    ///< The target ActionWindow is already closed.
    INVALID_TARGET,          ///< The action references a non-existent or illegal target.
    NOT_ENOUGH_RESOURCES,    ///< The actor lacks the resources required by the action.
    RULE_VIOLATION,          ///< Generic game-rule violation.
    ACTION_ALREADY_SUBMITTED ///< The same action ID was already submitted this turn.
};

// ─────────────────────────────────────────────────────────────────────────────
// ValidationResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class ValidationResult
 * @brief Outcome of an IAction::validate() call.
 *
 * Use the static factory methods to construct results:
 * @code
 *   return gmFlow::ValidationResult::ok();
 *   return gmFlow::ValidationResult::fail(
 *       gmFlow::ValidationError::NOT_ACTOR_TURN, "It is not your turn.");
 * @endcode
 */
class ValidationResult {
public:
    /**
     * @brief Constructs a successful validation result.
     * @return ValidationResult with valid() == true.
     */
    static ValidationResult ok();

    /**
     * @brief Constructs a failed validation result.
     * @param error  Reason code for the failure.
     * @param message Human-readable description (used in logs and UI feedback).
     * @return ValidationResult with valid() == false.
     */
    static ValidationResult fail(ValidationError error, std::string message);

    /// @brief Returns true if the action passed validation.
    bool valid() const;

    /// @brief Returns the reason code; NONE if the result is valid.
    ValidationError error() const;

    /// @brief Returns the human-readable failure message; empty if valid.
    const std::string& message() const;

private:
    bool            _valid   = true;
    ValidationError _error   = ValidationError::NONE;
    std::string     _message;
};

// ─────────────────────────────────────────────────────────────────────────────
// ActionResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class ActionResult
 * @brief Outcome of an IAction::execute() call.
 *
 * @code
 *   return gmFlow::ActionResult::success();
 *   return gmFlow::ActionResult::failure("Cannot move — path blocked.");
 * @endcode
 */
class ActionResult {
public:
    /**
     * @brief Constructs a successful action result.
     * @return ActionResult with succeeded() == true.
     */
    static ActionResult success();

    /**
     * @brief Constructs a failed action result.
     * @param reason Human-readable explanation of the failure.
     * @return ActionResult with succeeded() == false.
     */
    static ActionResult failure(std::string reason);

    /// @brief Returns true if the action executed successfully.
    bool succeeded() const;

    /// @brief Returns the failure reason; empty if the action succeeded.
    const std::string& reason() const;

private:
    bool        _succeeded = true;
    std::string _reason;
};

// ─────────────────────────────────────────────────────────────────────────────
// StepInput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct StepInput
 * @brief Opaque input bundle passed to IActionStep::execute().
 *
 * Game-specific subclasses of @ref StepBasedAction may downcast this to a
 * concrete input type to extract player choices, selected targets, etc.
 * The base struct is intentionally empty so that steps with no input
 * requirement can accept a default-constructed StepInput.
 */
struct StepInput {
    virtual ~StepInput() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// StepResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class StepResult
 * @brief Outcome of an IActionStep::execute() call.
 *
 * @code
 *   return gmFlow::StepResult::done();
 *   return gmFlow::StepResult::needs_input("Choose a target tile.");
 *   return gmFlow::StepResult::failed("Target tile is occupied.");
 * @endcode
 */
class StepResult {
public:
    /**
     * @brief Constructs a result indicating the step completed successfully.
     * @return StepResult with complete() == true and failed() == false.
     */
    static StepResult done();

    /**
     * @brief Constructs a result indicating the step is waiting for player input.
     * @param prompt Human-readable prompt shown to the player.
     * @return StepResult with complete() == false, needs_input() == true.
     */
    static StepResult needs_input(std::string prompt);

    /**
     * @brief Constructs a result indicating the step failed irrecoverably.
     * @param reason Human-readable explanation of the failure.
     * @return StepResult with complete() == false, failed() == true.
     */
    static StepResult failed(std::string reason);

    /// @brief Returns true if this step has finished executing.
    bool complete() const;

    /// @brief Returns true if the step is waiting for player input.
    bool needs_input() const;

    /// @brief Returns true if the step encountered an irrecoverable error.
    bool failed() const;

    /// @brief Returns the prompt string (populated when needs_input() == true).
    const std::string& prompt() const;

    /// @brief Returns the failure reason (populated when failed() == true).
    const std::string& reason() const;

private:
    bool        _complete    = false;
    bool        _needs_input = false;
    bool        _failed      = false;
    std::string _prompt;
    std::string _reason;
};

} // namespace gmFlow

#endif // GMFLOW_RESULT_HPP
