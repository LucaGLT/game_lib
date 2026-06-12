#ifndef GMFLOW_STEPBASEDACTION_HPP
#define GMFLOW_STEPBASEDACTION_HPP

/**
 * @file actions/StepBasedAction.hpp
 * @brief Default skeleton for multi-step actions composed of IActionStep objects.
 *
 * Inherit from StepBasedAction to build actions that require multiple rounds
 * of player input (e.g. "choose a card → choose a target → confirm").
 *
 * StepBasedAction handles step sequencing; subclasses add their concrete
 * steps in the constructor and implement `id()`, `owner()`, and `status()`.
 *
 * ### Subclassing example
 * @code
 *   class MoveAction : public gmFlow::StepBasedAction {
 *   public:
 *       MoveAction(gmFlow::ActorId actor)
 *           : owner_(std::move(actor))
 *           , id_("move_" + owner_)
 *       {
 *           add_step(std::make_unique<ChooseDestinationStep>());
 *           add_step(std::make_unique<ConfirmMoveStep>());
 *       }
 *
 *       gmFlow::ActionId     id()     const override { return id_; }
 *       gmFlow::ActorId      owner()  const override { return owner_; }
 *       gmFlow::ActionStatus status() const override { return status_; }
 *
 *   private:
 *       gmFlow::ActorId      owner_;
 *       gmFlow::ActionId     id_;
 *       gmFlow::ActionStatus status_ = gmFlow::ActionStatus::CREATED;
 *   };
 * @endcode
 */

#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/actions/IActionStep.hpp"

#include <memory>
#include <vector>

namespace gmFlow {

/**
 * @class StepBasedAction
 * @brief Abstract base class for actions with ordered sequential steps.
 *
 * The flow engine drives a StepBasedAction by calling `execute()` repeatedly.
 * Each call advances the current step.  When the last step completes,
 * `execute()` returns ActionResult::success() and the action moves to COMPLETED.
 *
 * `is_multi_step()` returns true unconditionally.
 */
class StepBasedAction : public IAction {
public:
    ~StepBasedAction() override = default;

    // IAction contract — is_async() and requires_turn() default implementations.

    /// @brief Returns false — multi-step actions are synchronous by default.
    bool is_async()      const override;

    /// @brief Returns true — multi-step actions require an active turn by default.
    bool requires_turn() const override;

    /// @brief Returns true — this class always uses the step-based execution model.
    bool is_multi_step() const override;

    /**
     * @brief Validates by delegating to the current step's `can_enter()` check.
     *
     * Subclasses may override this to add action-level preconditions before
     * the step-level check.
     *
     * @param ctx Read-only session context.
     * @return ValidationResult::ok() if the current step can be entered.
     */
    ValidationResult validate(const GameContext& ctx) const override;

    /**
     * @brief Advances the current step using a default-constructed StepInput.
     *
     * Call this overload when the current step requires no explicit input.
     * For steps that need player data, use `execute(ctx, input)` instead.
     *
     * @param ctx Mutable session context.
     * @return ActionResult::success() if all steps completed,
     *         ActionResult::failure() if the current step failed.
     */
    ActionResult execute(GameContext& ctx) override;

    /**
     * @brief Advances the current step with the provided player input.
     *
     * This overload allows game code to inject player choices at each step.
     *
     * @param ctx   Mutable session context.
     * @param input Player input for the current step.
     * @return ActionResult::success() if all steps completed,
     *         ActionResult::failure() if the current step failed.
     */
    ActionResult execute(GameContext& ctx, const StepInput& input);

    /// @brief Returns the 0-based index of the step currently being executed.
    std::size_t current_step_index() const;

    /// @brief Returns the total number of steps registered in this action.
    std::size_t step_count() const;

protected:
    /**
     * @brief Registers a step to be executed in sequence.
     *
     * Must be called from the subclass constructor, in order.
     *
     * @param step Step to append to the execution sequence.
     */
    void add_step(std::unique_ptr<IActionStep> step);

private:
    std::vector<std::unique_ptr<IActionStep>> steps_;
    std::size_t                               current_step_ = 0;
};

} // namespace gmFlow

#endif // GMFLOW_STEPBASEDACTION_HPP
