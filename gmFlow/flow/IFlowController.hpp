#ifndef GMFLOW_IFLOWCONTROLLER_HPP
#define GMFLOW_IFLOWCONTROLLER_HPP

/**
 * @file flow/IFlowController.hpp
 * @brief Interface for the central game-flow orchestrator.
 *
 * IFlowController is the brain of gmFlow: it decides phase ordering, turn
 * sequencing, round management, and when a session ends.  One concrete
 * implementation is provided out of the box (@ref SequentialFlowController)
 * for classic sequential-turn games.  Game-specific code may supply a custom
 * implementation for complex flow requirements.
 *
 * The flow controller is injected into @ref GameSession at construction and
 * drives the session lifecycle via two main entry points:
 * - `start()` — called once when the session starts.
 * - `process()` — called every tick (frame) by `GameSession::tick()`.
 *
 * ### Design contract
 * - The controller must never store raw pointers to game state outside a tick.
 * - It communicates all state transitions to the rest of the system through the
 *   @ref EventBus (via @ref GameContext::event_bus()).
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/Result.hpp"

// Forward declarations.
namespace gmFlow {
    class GameContext;
}

namespace gmFlow {

/**
 * @class IFlowController
 * @brief Pure-virtual interface for session flow orchestration.
 */
class IFlowController {
public:
    virtual ~IFlowController() = default;

    /**
     * @brief Initialises the flow controller and starts the first phase.
     *
     * Called once by @ref GameSession::start().  Implementations should:
     * 1. Validate the session configuration.
     * 2. Enter the first phase via `phase->on_enter(ctx)`.
     * 3. Open the first ActionWindow (or Turn).
     * 4. Publish @ref EVT_SESSION_STARTED.
     *
     * @param ctx Mutable session context.
     */
    virtual void start(GameContext& ctx) = 0;

    /**
     * @brief Advances the session by one tick.
     *
     * Called by @ref GameSession::tick().  Implementations should:
     * 1. Process the front of the @ref ActionQueue (if any).
     * 2. Check if the current ActionWindow / Turn is complete.
     * 3. Advance turns, rounds, or phases as appropriate.
     * 4. Open new ActionWindows for the next set of eligible actors.
     *
     * @param ctx Mutable session context.
     */
    virtual void process(GameContext& ctx) = 0;

    /**
     * @brief Returns true if the given actor is currently allowed to submit actions.
     *
     * Queried by @ref GameSession::submit_action() before validation.
     *
     * @param ctx     Read-only session context.
     * @param actor   Actor attempting to submit.
     * @return true if the actor has an open ActionWindow or an active turn.
     */
    virtual bool can_actor_act(const GameContext& ctx,
                               const ActorId& actor) const = 0;

    /**
     * @brief Notifies the controller that an action has finished executing.
     *
     * Called by the session after each `IAction::execute()` completes.
     * The controller may use the result to trigger follow-up effects, close
     * ActionWindows, or advance the turn.
     *
     * @param ctx    Mutable session context.
     * @param result Result of the completed action.
     */
    virtual void on_action_completed(GameContext&         ctx,
                                     const ActionResult&  result) = 0;

    /**
     * @brief Returns true when the session's victory/loss/end condition is met.
     *
     * Queried by @ref GameSession::tick() after each `process()` call.
     * When true, @ref GameSession transitions to COMPLETED.
     *
     * @param ctx Read-only session context.
     * @return true if the session should end.
     */
    virtual bool is_session_complete(const GameContext& ctx) const = 0;
};

} // namespace gmFlow

#endif // GMFLOW_IFLOWCONTROLLER_HPP
