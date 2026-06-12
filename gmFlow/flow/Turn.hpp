#ifndef GMFLOW_TURN_HPP
#define GMFLOW_TURN_HPP

/**
 * @file flow/Turn.hpp
 * @brief Represents a single turn within a game round.
 *
 * A Turn owns a list of actors that are currently eligible to submit actions.
 * For sequential games there is exactly one active actor; for simultaneous
 * games (e.g. Gloomhaven) all actors are active at the same time.
 */

#include "gmFlow/core/Ids.hpp"

#include <vector>

namespace gmFlow {

/**
 * @class Turn
 * @brief A time slice within which one or more actors may submit actions.
 *
 * Turn objects are created and managed by the @ref IFlowController
 * implementation.  They are passed to the EventBus as part of
 * @ref TurnStartedEvent and @ref TurnEndedEvent payloads.
 *
 * @code
 *   gmFlow::Turn t("round_1_turn_2");
 *   t.add_active_actor("player_1");
 *   t.add_active_actor("player_2");  // simultaneous turn
 * @endcode
 */
class Turn {
public:
    /**
     * @brief Constructs a Turn with the given identifier.
     * @param id Unique identifier for this turn (e.g. "round_3_turn_1").
     */
    explicit Turn(TurnId id);

    /// @brief Returns the unique identifier for this turn.
    const TurnId& id() const;

    /**
     * @brief Adds an actor to the set of active actors for this turn.
     * @param actor_id The ActorId of the actor that may act during this turn.
     */
    void add_active_actor(const ActorId& actor_id);

    /// @brief Returns all actors that are active (eligible to act) this turn.
    const std::vector<ActorId>& active_actors() const;

    /**
     * @brief Returns true if the given actor is active during this turn.
     * @param actor_id Actor to check.
     * @return true if the actor appears in the active_actors list.
     */
    bool is_actor_active(const ActorId& actor_id) const;

private:
    TurnId                id_;
    std::vector<ActorId>  active_actors_;
};

} // namespace gmFlow

#endif // GMFLOW_TURN_HPP
