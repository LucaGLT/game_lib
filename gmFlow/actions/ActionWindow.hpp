#ifndef GMFLOW_ACTIONWINDOW_HPP
#define GMFLOW_ACTIONWINDOW_HPP

/**
 * @file actions/ActionWindow.hpp
 * @brief A time-bounded opportunity for actors to submit actions.
 *
 * ActionWindow is the central mechanism for simultaneous turns, reactions,
 * and free out-of-turn actions.  All these scenarios share the same type;
 * the @ref CompletionPolicy determines when the window closes.
 *
 * | Scenario | Policy |
 * | -------- | ------ |
 * | Normal sequential turn | MANUAL_CLOSE (controller closes after actor acts) |
 * | Simultaneous turn | ALL_SUBMITTED (everyone must submit) |
 * | Reaction window | ANY_SUBMITTED (first reaction closes the window) |
 * | Pass-based end-of-turn | UNTIL_ALL_PASSED |
 * | Card stack resolution | PRIORITY_RESOLVED |
 *
 * ### Usage inside a flow controller
 * @code
 *   gmFlow::ActionWindow win({"player_1", "player_2"},
 *                            gmFlow::CompletionPolicy::ALL_SUBMITTED);
 *
 *   // When a player submits:
 *   auto result = win.submit("player_1", std::move(action));
 *
 *   // After the session tick:
 *   if (win.is_complete(ctx)) {
 *       win.resolve(ctx);  // executes all collected actions in priority order
 *   }
 * @endcode
 *
 * @note TIMEOUT_EXPIRED is deliberately absent from V1.  Timer support is
 *       deferred to V2.
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/actions/IAction.hpp"

#include <memory>
#include <vector>

// Forward declaration.
namespace gmFlow { class GameContext; }

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// CompletionPolicy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum CompletionPolicy
 * @brief Determines the condition under which an ActionWindow closes.
 *
 * @note TIMEOUT_EXPIRED is **not** available in V1. Timer-based expiry is
 *       deferred to V2.
 */
enum class CompletionPolicy {
    ALL_SUBMITTED,    ///< Window closes when every eligible actor has submitted.
    ANY_SUBMITTED,    ///< Window closes after the first submission.
    MANUAL_CLOSE,     ///< Window stays open until explicitly closed by the controller.
    UNTIL_ALL_PASSED, ///< Window closes when every eligible actor passes.
    PRIORITY_RESOLVED ///< Actions sorted by priority, then all resolved in order.
};

// ─────────────────────────────────────────────────────────────────────────────
// ActionWindow
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class ActionWindow
 * @brief A bounded opportunity for a set of actors to submit actions.
 *
 * ActionWindow is created by the @ref IFlowController and lives for the
 * duration of its open period.  Actors submit actions via `submit()`; the
 * controller polls `is_complete()` each tick and calls `resolve()` once
 * the window closes.
 */
class ActionWindow {
public:
    /**
     * @brief Constructs an ActionWindow with the given eligible actors and policy.
     *
     * @param eligible_actors Actors that are allowed to submit to this window.
     * @param policy          The condition under which the window closes.
     */
    ActionWindow(std::vector<ActorId> eligible_actors,
                 CompletionPolicy     policy);

    // Non-copyable.
    ActionWindow(const ActionWindow&)            = delete;
    ActionWindow& operator=(const ActionWindow&) = delete;
    ActionWindow(ActionWindow&&)                 = default;
    ActionWindow& operator=(ActionWindow&&)      = default;

    /**
     * @brief Returns true if the given actor may submit to this window.
     *
     * @param actor_id Actor to check.
     * @return true if the actor is in the eligible list and has not yet
     *         submitted (or the policy allows multiple submissions).
     */
    bool can_submit(const ActorId& actor_id) const;

    /**
     * @brief Submits an action to this window on behalf of the given actor.
     *
     * @param actor_id Actor submitting the action.
     * @param action   Action to submit; ownership is transferred.
     * @return ValidationResult::ok() on success,
     *         ValidationResult::fail(...) if the actor cannot submit.
     */
    ValidationResult submit(const ActorId& actor_id,
                            std::unique_ptr<IAction> action);

    /**
     * @brief Records that the given actor has passed (chosen not to act).
     *
     * Relevant when CompletionPolicy is UNTIL_ALL_PASSED.
     *
     * @param actor_id Actor that is passing.
     */
    void pass(const ActorId& actor_id);

    /**
     * @brief Returns true if the window's completion condition is satisfied.
     *
     * @param ctx Read-only session context.
     * @return true if the window should close.
     */
    bool is_complete(const GameContext& ctx) const;

    /**
     * @brief Closes the window and executes all submitted actions.
     *
     * Actions are executed in descending priority order.  Must only be
     * called when `is_complete()` returns true.
     *
     * @param ctx Mutable session context.
     */
    void resolve(GameContext& ctx);

    /**
     * @brief Explicitly closes the window without executing pending actions.
     *
     * Used with CompletionPolicy::MANUAL_CLOSE to end a window early.
     */
    void force_close();

    /// @brief Returns true if the window has been closed (resolved or force-closed).
    bool is_closed() const;

    /// @brief Returns the list of eligible actors for this window.
    const std::vector<ActorId>& eligible_actors() const;

    /// @brief Returns the number of actors that have submitted so far.
    std::size_t submission_count() const;

private:
    /// @brief Bundles an action with the actor that submitted it.
    struct Submission {
        ActorId                  actor_id;
        std::unique_ptr<IAction> action;
    };

    std::vector<ActorId>    eligible_actors_;
    std::vector<ActorId>    passed_actors_;
    std::vector<Submission> submissions_;
    CompletionPolicy        policy_;
    bool                    closed_ = false;
};

} // namespace gmFlow

#endif // GMFLOW_ACTIONWINDOW_HPP
