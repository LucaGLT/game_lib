#ifndef GMFLOW_GAMESESSION_HPP
#define GMFLOW_GAMESESSION_HPP

/**
 * @file session/GameSession.hpp
 * @brief Main entry point for starting and driving a game session.
 *
 * GameSession is the façade of the gmFlow framework.  Game code creates one
 * instance per session (match/scenario), injects its dependencies, and then
 * drives the session by calling `tick()` in a game loop.
 *
 * ### Typical usage
 * @code
 *   // 1. Create game state.
 *   auto state = std::make_unique<MyGameState>();
 *
 *   // 2. Build the dispatcher (gmDispatch) and game session.
 *   auto dispatcher = gmDispatch::DispatcherFactory::create_sync_dispatcher("GameBus");
 *
 *   gmFlow::SessionConfig cfg;
 *   cfg.session_id = "session_001";
 *   cfg.actors     = { gmFlow::Actor("p1", gmFlow::ActorType::PLAYER) };
 *
 *   auto ctrl = std::make_unique<gmFlow::SequentialFlowController>(
 *       build_phases());
 *
 *   gmFlow::GameSession session(cfg, std::move(ctrl), std::move(state), dispatcher);
 *
 *   // 3. Subscribe to events.
 *   session.event_bus().subscribe(gmFlow::EVT_TURN_STARTED, on_turn_started);
 *
 *   // 4. Start and run.
 *   session.start();
 *   while (!session.is_finished()) {
 *       // collect player input …
 *       session.submit_action("p1", std::make_unique<MyAction>(...));
 *       session.tick();
 *   }
 * @endcode
 *
 * ### Pause / Resume
 * @code
 *   session.pause();   // serialises state via gmSave, publishes EVT_SESSION_PAUSED
 *   // … save snapshot to disk …
 *   session.resume();  // restores state, publishes EVT_SESSION_RESUMED
 * @endcode
 */

#include "gmFlow/session/SessionConfig.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/flow/IFlowController.hpp"
#include "gmFlow/actions/ActionQueue.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/core/Ids.hpp"

#include <memory>
#include <stdexcept>

// Forward declarations.
namespace gmDispatch { class GmDispatcher; }

namespace gmFlow {

// Forward declaration.
class IAction;

/**
 * @class EGameSessionError
 * @brief Base exception thrown by GameSession for invalid operations.
 */
class EGameSessionError : public std::runtime_error {
public:
    /// @brief Constructs the error with a descriptive message.
    explicit EGameSessionError(const std::string& message);
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum SessionState
 * @brief Lifecycle state of a GameSession.
 */
enum class SessionState {
    CREATED,    ///< Constructed but not yet started.
    RUNNING,    ///< Active — tick() is being called.
    PAUSED,     ///< Temporarily suspended; state is serialized.
    COMPLETED,  ///< Session ended normally (flow controller reported complete).
    FAILED      ///< Session ended due to an unrecoverable error.
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class GameSession
 * @brief Central façade that orchestrates a single game session.
 *
 * GameSession owns the @ref GameContext, @ref ActionQueue, @ref ActorRegistry,
 * and @ref EventBus.  It delegates flow decisions to the injected
 * @ref IFlowController.
 *
 * **Ownership**:
 * - `flow_controller` — exclusive ownership transferred to GameSession.
 * - `state`           — exclusive ownership transferred to GameSession.
 * - `dispatcher`      — shared ownership (may be reused across sessions).
 *
 * **Thread safety**: GameSession is **not** thread-safe.  All calls must
 * occur on the same thread.
 */
class GameSession {
public:
    /**
     * @brief Constructs a GameSession.
     *
     * @param config          Static session parameters (actors, policies, ID).
     * @param flow_controller The flow orchestrator; must not be null.
     * @param state           Game-specific mutable state; must not be null.
     * @param dispatcher      Shared GmDispatch dispatcher used for the EventBus.
     * @throws std::invalid_argument if flow_controller or state is null.
     */
    GameSession(SessionConfig                            config,
                std::unique_ptr<IFlowController>         flow_controller,
                std::unique_ptr<GameState>               state,
				std::shared_ptr<gmDispatch::GmDispatcher>  dispatcher);

    ~GameSession() = default;

    // Non-copyable, non-movable (owns non-copyable members).
    GameSession(const GameSession&)            = delete;
    GameSession& operator=(const GameSession&) = delete;
    GameSession(GameSession&&)                 = delete;
    GameSession& operator=(GameSession&&)      = delete;

    /**
     * @brief Starts the session.
     *
     * Transitions from CREATED to RUNNING, populates the ActorRegistry,
     * calls `IFlowController::start()`, and publishes @ref EVT_SESSION_STARTED.
     *
     * @throws EGameSessionError if the session is not in the CREATED state.
     */
    void start();

    /**
     * @brief Advances the session by one tick.
     *
     * Calls `IFlowController::process()`, then checks
     * `IFlowController::is_session_complete()`.  If complete, transitions to
     * COMPLETED and publishes @ref EVT_SESSION_COMPLETED.
     *
     * @throws EGameSessionError if the session is not in the RUNNING state.
     */
    void tick();

    /**
     * @brief Pauses the session.
     *
     * Transitions from RUNNING to PAUSED, serialises the current state
     * snapshot via gmSave, and publishes @ref EVT_SESSION_PAUSED.
     * The session loop must not call `tick()` while paused.
     *
     * @throws EGameSessionError if the session is not in the RUNNING state.
     */
    void pause();

    /**
     * @brief Resumes a paused session.
     *
     * Restores the state snapshot and transitions back to RUNNING.
     * Publishes @ref EVT_SESSION_RESUMED.
     *
     * @throws EGameSessionError if the session is not in the PAUSED state.
     */
    void resume();

    /**
     * @brief Validates and enqueues an action submitted by an actor.
     *
     * Performs two-stage validation:
     * 1. `IFlowController::can_actor_act()` — checks turn/window eligibility.
     * 2. `IAction::validate()` — checks game-rule preconditions.
     *
     * If both pass, the action is pushed onto the @ref ActionQueue with the
     * appropriate @ref ActionPriority.
     *
     * @param actor  ID of the actor submitting the action.
     * @param action The action to submit; ownership is transferred on success.
     * @return ValidationResult::ok() if accepted; fail(...) otherwise.
     * @throws EGameSessionError if the session is not in the RUNNING state.
     */
    ValidationResult submit_action(const ActorId&           actor,
                                   std::unique_ptr<IAction> action);

    /// @brief Returns true if the session has completed (COMPLETED or FAILED).
    bool is_finished() const;

    /// @brief Returns true if the session is currently paused.
    bool is_paused() const;

    /// @brief Returns the current lifecycle state.
    SessionState state() const;

    /// @brief Returns a const reference to the runtime context.
    const GameContext& context() const;

    /// @brief Returns a reference to the EventBus for subscribing to events.
    EventBus& event_bus();

    /// @brief Returns the session's unique identifier.
    const SessionId& session_id() const;

private:
    SessionConfig                           _config;
    std::unique_ptr<GameState>              _game_state;
    ActorRegistry                           _actor_registry;
    EventBus                                _event_bus;
    GameContext                             _context;
    std::unique_ptr<IFlowController>        _flow_controller;
    ActionQueue                             _action_queue;
    SessionState                            _session_state = SessionState::CREATED;
};

} // namespace gmFlow

#endif // GMFLOW_GAMESESSION_HPP
