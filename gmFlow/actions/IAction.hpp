#ifndef GMFLOW_IACTION_HPP
#define GMFLOW_IACTION_HPP

/**
 * @file actions/IAction.hpp
 * @brief Interface for all game actions submitted to a session.
 *
 * IAction is the primary plug-in point for game-specific code.  Every
 * discrete game action — moving a pawn, playing a card, attacking, passing —
 * is represented as a concrete class that implements this interface.
 *
 * ### Minimal implementation
 * @code
 *   class PassAction : public gmFlow::IAction {
 *   public:
 *       PassAction(gmFlow::ActorId actor)
 *           : id_("pass_" + actor), owner_(std::move(actor)) {}
 *
 *       gmFlow::ActionId     id()     const override { return id_; }
 *       gmFlow::ActorId      owner()  const override { return owner_; }
 *       gmFlow::ActionStatus status() const override { return status_; }
 *
 *       gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override {
 *           return gmFlow::ValidationResult::ok();
 *       }
 *       gmFlow::ActionResult execute(gmFlow::GameContext&) override {
 *           status_ = gmFlow::ActionStatus::COMPLETED;
 *           return gmFlow::ActionResult::success();
 *       }
 *
 *       bool is_async()      const override { return false; }
 *       bool requires_turn() const override { return true;  }
 *       bool is_multi_step() const override { return false; }
 *
 *   private:
 *       gmFlow::ActionId     id_;
 *       gmFlow::ActorId      owner_;
 *       gmFlow::ActionStatus status_ = gmFlow::ActionStatus::CREATED;
 *   };
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/actions/ActionStatus.hpp"

// Forward declaration: IAction methods receive GameContext by reference.
// Full definition is in gmFlow/core/GameContext.hpp.
namespace gmFlow { class GameContext; }

namespace gmFlow {

/**
 * @class IAction
 * @brief Pure-virtual interface for all game actions.
 *
 * The flow engine never instantiates IAction directly.  It receives
 * `std::unique_ptr<IAction>` objects from game code via
 * `GameSession::submit_action()`.
 *
 * **Ownership rule**: after `submit_action()` accepts an action, the session
 * takes exclusive ownership.  The submitter must not retain the pointer.
 *
 * **Atomicity**: actions are atomic in V1.  Once `execute()` is called, it
 * runs to completion.  There is no `ActionStatus::SUSPENDED`.  Use
 * `is_multi_step()` and @ref IActionStep for actions that require player
 * input at each step.
 */
class IAction {
public:
    virtual ~IAction() = default;

    /// @brief Returns the unique identifier for this action instance.
    virtual ActionId id() const = 0;

    /// @brief Returns the ActorId of the actor who submitted this action.
    virtual ActorId owner() const = 0;

    /// @brief Returns the current lifecycle status of this action.
    virtual ActionStatus status() const = 0;

    /**
     * @brief Validates whether this action is currently legal.
     *
     * Called by the flow engine before `execute()`.  Must be const and
     * side-effect-free — it must not mutate game state or emit events.
     *
     * @param ctx Read-only view of the current session context.
     * @return ValidationResult::ok() if the action may proceed,
     *         ValidationResult::fail(...) with a reason code otherwise.
     */
    virtual ValidationResult validate(const GameContext& ctx) const = 0;

    /**
     * @brief Executes the action, mutating game state and publishing events.
     *
     * Called by the flow engine after successful validation.
     * Implementations must update their own `status_` to COMPLETED or FAILED
     * before returning.
     *
     * @param ctx Mutable session context (access to GameState and EventBus).
     * @return ActionResult::success() or ActionResult::failure(reason).
     */
    virtual ActionResult execute(GameContext& ctx) = 0;

    /**
     * @brief Returns true if this action may execute asynchronously.
     *
     * Async actions may be enqueued and executed out-of-turn by the flow
     * controller when an open ActionWindow permits it.
     *
     * @return true if the action supports async execution.
     */
    virtual bool is_async() const = 0;

    /**
     * @brief Returns true if this action requires the actor to have an active turn.
     *
     * Free-action and reaction types may return false to indicate they can be
     * submitted during another actor's turn (inside an open ActionWindow).
     *
     * @return true if a turn is required.
     */
    virtual bool requires_turn() const = 0;

    /**
     * @brief Returns true if this action is composed of sequential IActionSteps.
     *
     * Multi-step actions must also inherit from @ref StepBasedAction.
     * When true, the flow engine calls `execute()` once per step, passing
     * the player's input via the step mechanism.
     *
     * @return true if the action uses the step-based execution model.
     */
    virtual bool is_multi_step() const = 0;
};

} // namespace gmFlow

#endif // GMFLOW_IACTION_HPP
