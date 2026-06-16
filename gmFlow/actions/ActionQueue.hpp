#ifndef GMFLOW_ACTIONQUEUE_HPP
#define GMFLOW_ACTIONQUEUE_HPP

/**
 * @file actions/ActionQueue.hpp
 * @brief Priority queue of pending actions awaiting execution.
 *
 * ActionQueue holds all @ref IAction objects submitted to a @ref GameSession
 * that have passed initial validation and are waiting to be executed by the
 * @ref IFlowController.
 *
 * Actions are ordered by @ref ActionPriority (IMMEDIATE first, DEFERRED last).
 * Within the same priority level, order is FIFO.
 *
 * ### Typical flow
 * @code
 *   // GameSession::submit_action() calls:
 *   queue.push(std::move(action), gmFlow::ActionPriority::NORMAL);
 *
 *   // IFlowController::process() calls:
 *   while (!queue.empty()) {
 *       gmFlow::IAction& next = queue.front();
 *       next.execute(ctx);
 *       queue.pop();
 *   }
 * @endcode
 */

#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/actions/ActionPriority.hpp"

#include <memory>
#include <vector>

namespace gmFlow {

/**
 * @class ActionQueue
 * @brief Priority-ordered queue of actions waiting to be executed.
 *
 * ActionQueue is non-copyable.  It is owned by @ref GameSession and not
 * accessible to game-specific code directly.
 */
class ActionQueue {
public:
    ActionQueue() = default;

    // Non-copyable.
    ActionQueue(const ActionQueue&)            = delete;
    ActionQueue& operator=(const ActionQueue&) = delete;
    ActionQueue(ActionQueue&&)                 = default;
    ActionQueue& operator=(ActionQueue&&)      = default;

    /**
     * @brief Pushes an action onto the queue with the given priority.
     *
     * The action is inserted so that higher-priority actions always precede
     * lower-priority ones.  Among equal priorities, insertion order is
     * preserved (FIFO).
     *
     * @param action   Action to enqueue; ownership is transferred.
     * @param priority Processing priority for this action.
     */
    void push(std::unique_ptr<IAction> action, ActionPriority priority);

    /**
     * @brief Returns a reference to the highest-priority pending action.
     *
     * @return Reference to the front action.
     * @pre !empty()
     */
    IAction& front();

    /**
     * @brief Returns a const reference to the highest-priority pending action.
     * @return Const reference to the front action.
     * @pre !empty()
     */
    const IAction& front() const;

    /**
     * @brief Removes the highest-priority action from the queue.
     * @pre !empty()
     */
    void pop();

    /// @brief Returns true if there are no pending actions.
    bool empty() const;

    /// @brief Returns the number of pending actions.
    std::size_t size() const;

    /**
     * @brief Removes all pending actions from the queue.
     *
     * Used by GameSession::pause() before serialising the session state.
     */
    void clear();

private:
    /// @brief Internal entry combining an action with its assigned priority.
    struct Entry {
        std::unique_ptr<IAction> action;
        ActionPriority           priority;
    };

    /// Backing store — sorted highest priority first.
    std::vector<Entry> _entries;
};

} // namespace gmFlow

#endif // GMFLOW_ACTIONQUEUE_HPP
