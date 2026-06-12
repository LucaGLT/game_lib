#ifndef GMFLOW_IACTIONSTEP_HPP
#define GMFLOW_IACTIONSTEP_HPP

/**
 * @file actions/IActionStep.hpp
 * @brief Interface for one sequential step within a multi-step action.
 *
 * IActionStep divides an @ref IAction into discrete sub-operations that may
 * each require player input before proceeding.  Steps are managed by
 * @ref StepBasedAction, which drives them in sequence.
 *
 * ### Implementing a step
 * @code
 *   class ChooseTargetStep : public gmFlow::IActionStep {
 *   public:
 *       gmFlow::StepId id() const override { return "choose_target"; }
 *
 *       bool can_enter(const gmFlow::GameContext&) const override { return true; }
 *
 *       gmFlow::StepResult execute(gmFlow::GameContext& ctx,
 *                                  const gmFlow::StepInput& raw_input) override
 *       {
 *           const auto& input = static_cast<const TargetInput&>(raw_input);
 *           if (!ctx.state().tile_is_valid(input.tile)) {
 *               return gmFlow::StepResult::failed("Invalid tile selected.");
 *           }
 *           chosen_tile_ = input.tile;
 *           return gmFlow::StepResult::done();
 *       }
 *
 *       bool is_complete(const gmFlow::GameContext&) const override {
 *           return chosen_tile_.has_value();
 *       }
 *
 *   private:
 *       std::optional<TileCoord> chosen_tile_;
 *   };
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"

// Forward declaration — full definition in GameContext.hpp.
namespace gmFlow { class GameContext; }

namespace gmFlow {

/**
 * @class IActionStep
 * @brief Pure-virtual interface for one step in a @ref StepBasedAction.
 *
 * Each step is responsible for:
 * 1. Declaring whether it may be entered (`can_enter`).
 * 2. Processing player input and mutating game state (`execute`).
 * 3. Reporting whether it has finished (`is_complete`).
 *
 * Steps are intended to be lightweight objects owned by their parent
 * `StepBasedAction`.
 */
class IActionStep {
public:
    virtual ~IActionStep() = default;

    /// @brief Returns the unique identifier for this step within its parent action.
    virtual StepId id() const = 0;

    /**
     * @brief Returns true if the step's entry preconditions are currently met.
     *
     * The flow engine calls this before invoking `execute()`.  A step that
     * cannot be entered (e.g. because a prerequisite step failed) should
     * return false.
     *
     * @param ctx Read-only session context.
     * @return true if the step may be entered.
     */
    virtual bool can_enter(const GameContext& ctx) const = 0;

    /**
     * @brief Executes the step's logic using the provided player input.
     *
     * May mutate game state and emit events.  Must not advance to the next
     * step — @ref StepBasedAction is responsible for step sequencing.
     *
     * @param ctx       Mutable session context.
     * @param input     Player input for this step.  Downcast to a concrete
     *                  input type if the step requires specific data.
     * @return StepResult::done()        — step complete, advance to next.
     *         StepResult::needs_input() — step blocked, request more input.
     *         StepResult::failed()      — step failed irrecoverably.
     */
    virtual StepResult execute(GameContext& ctx, const StepInput& input) = 0;

    /**
     * @brief Returns true if this step has finished executing.
     *
     * The flow engine queries this after each `execute()` call to decide
     * whether to advance to the next step.
     *
     * @param ctx Read-only session context.
     * @return true if the step is complete.
     */
    virtual bool is_complete(const GameContext& ctx) const = 0;
};

} // namespace gmFlow

#endif // GMFLOW_IACTIONSTEP_HPP
