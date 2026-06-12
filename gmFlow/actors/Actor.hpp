#ifndef GMFLOW_ACTOR_HPP
#define GMFLOW_ACTOR_HPP

/**
 * @file actors/Actor.hpp
 * @brief Represents an entity that can submit actions within a game session.
 *
 * An Actor is any participant recognised by the flow engine: a human player,
 * an AI bot, the game system itself, a team, or a game master.  Actors are
 * identified by their @ref ActorId and registered in the @ref ActorRegistry.
 */

#include "gmFlow/core/Ids.hpp"

#include <string>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// ActorType
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ActorType
 * @brief Classifies the nature of an Actor within the session.
 */
enum class ActorType {
    PLAYER,      ///< Human player (local or remote).
    BOT,         ///< AI-controlled participant.
    SYSTEM,      ///< Automated game-system actor (triggers automatic effects).
    TEAM,        ///< A group of players acting as a collective unit.
    GAME_MASTER  ///< Human referee with elevated privileges (scenario games).
};

// ─────────────────────────────────────────────────────────────────────────────
// Actor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class Actor
 * @brief Immutable descriptor of a session participant.
 *
 * Actors are value-typed descriptors; they carry no mutable game state.
 * Mutable per-actor state (e.g. health, resources) belongs in @ref GameState.
 *
 * @code
 *   gmFlow::Actor hero("player_1", gmFlow::ActorType::PLAYER);
 *   gmFlow::Actor villain("goblin_boss", gmFlow::ActorType::BOT);
 * @endcode
 */
class Actor {
public:
    /**
     * @brief Constructs an Actor with the given ID and type.
     * @param id   Unique identifier for this actor within the session.
     * @param type Classification of the actor (player, bot, system, etc.).
     */
    explicit Actor(ActorId id, ActorType type);

    /// @brief Returns the actor's unique identifier.
    const ActorId& id() const;

    /// @brief Returns the actor's type classification.
    ActorType type() const;

    /// @brief Returns a human-readable display name (defaults to id if not set).
    const std::string& display_name() const;

    /**
     * @brief Sets an optional human-readable display name.
     * @param name Display name shown in logs and UI (e.g. "Player One", "Boss Goblin").
     */
    void set_display_name(std::string name);

private:
    ActorId     id_;
    ActorType   type_;
    std::string display_name_;
};

} // namespace gmFlow

#endif // GMFLOW_ACTOR_HPP
