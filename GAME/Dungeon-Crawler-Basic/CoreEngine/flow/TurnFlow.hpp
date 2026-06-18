#ifndef GMDUNGEONBASIC_TURNFLOW_HPP
#define GMDUNGEONBASIC_TURNFLOW_HPP

/**
 * @file flow/TurnFlow.hpp
 * @brief Turn and round manager for the dungeon session.
 *
 * TurnFlow wraps @c gmFlow to track the current session, the actor order,
 * the current round number and which actor is acting. It enforces a strict
 * hero-then-monsters sequencing within each round and notifies the engine
 * when automatic phase transitions occur.
 *
 * @note Not thread-safe.
 */

#include <string>
#include <vector>

namespace gmDungeonBasic
{

/**
 * @brief Manages the ordered sequence of actor turns within a dungeon session.
 *
 * A session is composed of rounds. Each round visits every actor in the
 * registered order: heroes first, then monsters from lowest to highest
 * priority. The engine calls @ref end_turn() after processing each actor's
 * actions; TurnFlow then advances to the next actor (or increments the round).
 */
class TurnFlow
{
public:
	/// @brief Constructs a TurnFlow in idle state (no session active).
	TurnFlow();

	/**
	 * @brief Starts a new dungeon session.
	 *
	 * Resets round counter to 1 and clears the turn position.
	 * Must be called before @ref start_turn().
	 */
	void start_session();

	/**
	 * @brief Ends the current session (game over or restart).
	 */
	void end_session();

	/**
	 * @brief Returns whether a session is currently active.
	 *
	 * @return  @c true if @ref start_session() has been called and
	 *          @ref end_session() has not.
	 */
	bool is_session_active() const;

	/**
	 * @brief Starts the turn for the specified actor.
	 *
	 * @param actor_id  The actor whose turn begins now.
	 * @throws std::logic_error  If a session is not active.
	 */
	void start_turn(const std::string& actor_id);

	/**
	 * @brief Ends the current actor's turn.
	 *
	 * Advances the internal cursor to the next actor in the order, or
	 * increments the round counter if the last actor has acted.
	 *
	 * @throws std::logic_error  If no turn is active.
	 */
	void end_turn();

	/**
	 * @brief Returns whether a turn is currently in progress.
	 *
	 * @return  @c true between @ref start_turn() and @ref end_turn() calls.
	 */
	bool is_turn_active() const;

	/**
	 * @brief Returns the id of the actor whose turn is currently active.
	 *
	 * @return  Actor id string, or empty string if no turn is active.
	 */
	std::string current_actor_id() const;

	/**
	 * @brief Returns the current round number (1-based).
	 *
	 * @return  Round number, or 0 if no session is active.
	 */
	int current_round() const;

	/**
	 * @brief Sets the ordered list of actor ids for each round.
	 *
	 * @param actor_order  Actor ids in the order they act each round.
	 *                     Heroes should appear before monsters.
	 */
	void set_actor_order(const std::vector<std::string>& actor_order);

	/**
	 * @brief Returns the id of the next actor in the round order.
	 *
	 * Does not advance the cursor; use @ref end_turn() to advance.
	 *
	 * @return  Next actor id, or empty string if the round is complete.
	 */
	std::string next_actor_id() const;

	/// @brief Resets to idle state (no session, no turn).
	void reset();
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_TURNFLOW_HPP
