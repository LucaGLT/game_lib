#ifndef GMFLOW_ACTIONSTATUS_HPP
#define GMFLOW_ACTIONSTATUS_HPP

/**
 * @file actions/ActionStatus.hpp
 * @brief Lifecycle states for a single game action.
 */

namespace gmFlow {

/**
 * @enum ActionStatus
 * @brief Describes the lifecycle state of an @ref IAction instance.
 *
 * Allowed transitions:
 * @code
 *  CREATED
 *    └─► SUBMITTED
 *          └─► VALIDATING
 *                ├─► WAITING_FOR_INPUT ──────────► EXECUTING
 *                ├─► WAITING_FOR_REACTION ────────► EXECUTING
 *                └─► EXECUTING
 *                       ├─► COMPLETED
 *                       └─► FAILED
 *  any ──────────────────────────────────────────► CANCELLED
 * @endcode
 *
 * @note Actions are **atomic** in V1: there is no SUSPENDED state.
 *       Session-level pause/resume is handled by GameSession::pause(),
 *       which serialises the full session snapshot via gmSave.
 */
enum class ActionStatus {
    CREATED,              ///< Action object constructed, not yet submitted.
    SUBMITTED,            ///< Submitted to the session; awaiting validation.
    VALIDATING,           ///< Flow controller is checking prerequisites.
    WAITING_FOR_INPUT,    ///< Multi-step action waiting for UI or player input.
    WAITING_FOR_REACTION, ///< An ActionWindow is open; awaiting responses.
    EXECUTING,            ///< Body logic is running.
    COMPLETED,            ///< Action finished successfully.
    FAILED,               ///< Could not complete (rule violation, invalid state).
    CANCELLED             ///< Explicitly cancelled before completion.
};

} // namespace gmFlow

#endif // GMFLOW_ACTIONSTATUS_HPP
