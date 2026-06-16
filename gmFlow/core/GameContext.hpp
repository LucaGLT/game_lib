#ifndef GMFLOW_GAMECONTEXT_HPP
#define GMFLOW_GAMECONTEXT_HPP

/**
 * @file core/GameContext.hpp
 * @brief Runtime context passed to every IAction, IPhase, and IFlowController method.
 *
 * GameContext is the "fat pointer" of the gmFlow framework.  Instead of passing
 * individual references to GameState, EventBus, ActorRegistry, and session
 * metadata separately, every interface method receives a single `GameContext&`
 * (or `const GameContext&` for read-only operations).
 *
 * GameContext itself holds no game-specific state.  It is a stable reference
 * bundle owned by @ref GameSession for the duration of the session.
 *
 * ### Pattern inside IAction::execute()
 * @code
 *   gmFlow::ActionResult MyMoveAction::execute(gmFlow::GameContext& ctx) override
 *   {
 *       auto& state = static_cast<MyGameState&>(ctx.state());
 *       state.move_actor(owner_, target_tile_);
 *
 *       gmFlow::ActionCompletedEvent ev;
 *       ev.action_id = id_;
 *       ev.actor_id  = owner_;
 *       ctx.event_bus().publish(ev);
 *
 *       return gmFlow::ActionResult::success();
 *   }
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"

// Forward declaration to avoid the EventBus → Dispatcher header chain
// in every file that includes GameContext.
namespace gmFlow { class EventBus; }

namespace gmFlow {

/**
 * @class GameContext
 * @brief Bundle of mutable and immutable session-level services.
 *
 * One GameContext is owned by each @ref GameSession and is passed by reference
 * to all framework callbacks throughout the session lifetime.  Callers must
 * not store the reference beyond the scope of the callback.
 */
class GameContext {
public:
    /**
     * @brief Constructs a GameContext referencing the given session services.
     *
     * All references must remain valid for the entire session lifetime.
     *
     * @param session_id Unique identifier for the running session.
     * @param state      Mutable game state; ownership stays with GameSession.
     * @param registry   Actor registry populated at session start.
     * @param event_bus  EventBus for publishing lifecycle events.
     */
    GameContext(SessionId          session_id,
                GameState&         state,
                ActorRegistry&     registry,
                EventBus&          event_bus);

    // Non-copyable, non-movable — held by reference, lifetime tied to GameSession.
    GameContext(const GameContext&)            = delete;
    GameContext& operator=(const GameContext&) = delete;
    GameContext(GameContext&&)                 = delete;
    GameContext& operator=(GameContext&&)      = delete;

    /// @brief Returns the unique identifier for the running session.
    const SessionId& session_id() const;

    /// @brief Returns a mutable reference to the game-specific state container.
    GameState& state();

    /// @brief Returns a const reference to the game-specific state container.
    const GameState& state() const;

    /// @brief Returns the actor registry for this session.
    ActorRegistry& actor_registry();

    /// @brief Returns a const view of the actor registry.
    const ActorRegistry& actor_registry() const;

    /// @brief Returns the EventBus for publishing and subscribing to events.
    EventBus& event_bus();

    /// @brief Returns the session's current phase ID (empty before first phase starts).
    const PhaseId& current_phase_id() const;

    /// @brief Returns the session's current round ID (empty if rounds are disabled).
    const RoundId& current_round_id() const;

    /// @brief Returns the session's current turn ID (empty between turns).
    const TurnId& current_turn_id() const;

    /**
     * @brief Updates the current phase ID.
     *
     * Called by @ref IFlowController when a phase transition occurs.
     * Must not be called from within IAction or IPhase callbacks.
     *
     * @param id New current phase ID.
     */
    void set_current_phase_id(PhaseId id);

    /**
     * @brief Updates the current round ID.
     * @param id New current round ID.
     */
    void set_current_round_id(RoundId id);

    /**
     * @brief Updates the current turn ID.
     * @param id New current turn ID.
     */
    void set_current_turn_id(TurnId id);

private:
    SessionId      _session_id;
    GameState&     _state;
    ActorRegistry& _registry;
    EventBus&      _event_bus;

    PhaseId        _current_phase_id;
    RoundId        _current_round_id;
    TurnId         _current_turn_id;
};

} // namespace gmFlow

#endif // GMFLOW_GAMECONTEXT_HPP
