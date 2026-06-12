#ifndef GMFLOW_GAMESTATE_HPP
#define GMFLOW_GAMESTATE_HPP

/**
 * @file core/GameState.hpp
 * @brief Abstract base class for mutable game state.
 *
 * GameState is the central mutable data store for a running game session.
 * It is intentionally opaque to the flow engine: gmFlow never reads or writes
 * game-specific data directly.  Instead, @ref IAction::execute() receives a
 * @ref GameContext whose `state()` method exposes this object, and game-specific
 * actions downcast it to their concrete state type.
 *
 * ### Subclassing example
 * @code
 *   class DungeonState : public gmFlow::GameState {
 *   public:
 *       std::unordered_map<gmFlow::ActorId, int> hero_hp;
 *       std::vector<TileCoord>                   monster_positions;
 *   };
 * @endcode
 *
 * ### Serialization
 * If the game needs session snapshots (pause/resume via gmSave), the concrete
 * subclass must also implement serialization using the gmSave API.
 */

#include "gmFlow/core/Ids.hpp"

namespace gmFlow {

/**
 * @class GameState
 * @brief Abstract base for all game-specific mutable state containers.
 *
 * The flow engine holds a pointer to GameState but never accesses its contents.
 * All read/write access is done by game-specific @ref IAction implementations
 * after a `static_cast` or `dynamic_cast` to the concrete subclass.
 */
class GameState {
public:
    virtual ~GameState() = default;

    /**
     * @brief Returns the session identifier this state belongs to.
     * @return SessionId string; may be empty until the session is started.
     */
    virtual const SessionId& session_id() const = 0;

    /**
     * @brief Signals the state that a new session is beginning.
     *
     * Called once by @ref GameSession::start() before any phase or action
     * processing.  Implementations should perform initial setup here.
     *
     * @param id The SessionId assigned to this session.
     */
    virtual void on_session_started(const SessionId& id) = 0;

    /**
     * @brief Signals the state that the session has completed.
     *
     * Called once by @ref GameSession when @ref IFlowController::is_session_complete()
     * returns true.  Implementations may persist final results here.
     */
    virtual void on_session_completed() = 0;
};

} // namespace gmFlow

#endif // GMFLOW_GAMESTATE_HPP
