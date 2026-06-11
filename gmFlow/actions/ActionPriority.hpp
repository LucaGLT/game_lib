#ifndef GMFLOW_ACTIONPRIORITY_HPP
#define GMFLOW_ACTIONPRIORITY_HPP

/**
 * @file actions/ActionPriority.hpp
 * @brief Priority levels used to order actions in the ActionQueue.
 */

namespace gmFlow {

/**
 * @enum ActionPriority
 * @brief Controls the processing order of pending actions in the @ref ActionQueue.
 *
 * Actions with a higher priority are dequeued and processed first.
 * Ordering (highest to lowest): IMMEDIATE > REACTION > NORMAL > DEFERRED.
 *
 * Example — a defensive reaction card:
 * @code
 *   class BlockAction : public gmFlow::IAction {
 *       gmFlow::ActionPriority priority() const override {
 *           return gmFlow::ActionPriority::REACTION;
 *       }
 *   };
 * @endcode
 */
enum class ActionPriority {
    IMMEDIATE, ///< Interrupts and cancel requests — processed before anything else.
    REACTION,  ///< Responses inside an open ActionWindow (e.g. defensive card play).
    NORMAL,    ///< Standard player turn action.
    DEFERRED   ///< End-of-phase effects, automatic cleanup actions.
};

} // namespace gmFlow

#endif // GMFLOW_ACTIONPRIORITY_HPP
