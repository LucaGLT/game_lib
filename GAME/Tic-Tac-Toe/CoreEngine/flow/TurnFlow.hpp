#ifndef GMTRIS_TURNFLOW_HPP
#define GMTRIS_TURNFLOW_HPP

/**
 * @file flow/TurnFlow.hpp
 * @brief Tracks the current match phase and maps phases to the active mark.
 *
 * Phase 2 drives the turn/round bookkeeping through gmFlow value types: the two
 * players are registered in a gmFlow::ActorRegistry and each phase transition
 * opens a gmFlow::Turn (active actor) inside the current gmFlow::Round. The
 * gmTris::Phase remains the authoritative state queried by the engine so the
 * GUI event contract is unchanged.
 */

#include "engine/TrisTypes.hpp"

#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/flow/Round.hpp"
#include "gmFlow/flow/Turn.hpp"

#include <memory>
#include <string>

namespace gmTris
{

/**
 * @class TurnFlow
 * @brief Minimal turn state machine backed by gmFlow Turn/Round/ActorRegistry.
 */
class TurnFlow
{
  public:
	/// @brief Registers the two players in the gmFlow actor registry.
	TurnFlow();

	/// @brief Starts a session with @p first_turn as the active phase (round 1).
	void start(Phase first_turn);

	/// @brief Returns the current phase.
	Phase phase() const;

	/// @brief Switches to the given phase, updating the gmFlow turn/round.
	void set_phase(Phase phase);

	/// @brief Returns true while the match has not reached GAME_OVER.
	bool is_active() const;

	/// @brief Returns the 1-based round index (0 before the first turn).
	int round_index() const;

	/// @brief Returns the active actor id, or empty when no turn is open.
	std::string active_actor() const;

	/// @brief Read-only access to the backing gmFlow actor registry.
	const gmFlow::ActorRegistry &registry() const;

  private:
	/// @brief Actor id of the PLAYER1 (X) participant.
	static const std::string ACTOR_X;
	/// @brief Actor id of the PLAYER2 (O) participant.
	static const std::string ACTOR_O;

	/// @brief Returns the actor id owning @p phase, or empty for non-turn phases.
	static std::string actor_for(Phase phase);

	/// @brief Opens a new gmFlow::Turn for @p phase inside the current round.
	void open_turn(Phase phase);

	gmFlow::ActorRegistry _registry;
	std::unique_ptr<gmFlow::Round> _round;
	std::unique_ptr<gmFlow::Turn> _turn;
	int _round_index = 0;
	int _turn_counter = 0;
	Phase _phase = Phase::BOOTSTRAP;
};

} // namespace gmTris

#endif // GMTRIS_TURNFLOW_HPP
