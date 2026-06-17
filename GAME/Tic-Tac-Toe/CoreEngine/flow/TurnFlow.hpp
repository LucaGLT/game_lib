#ifndef GMTRIS_TURNFLOW_HPP
#define GMTRIS_TURNFLOW_HPP

/**
 * @file flow/TurnFlow.hpp
 * @brief Tracks the current match phase and maps phases to the active mark.
 *
 * @note Phase 1 keeps the phase in a plain member. Phase 2 will drive the phase
 *       transitions through a gmFlow controller / session.
 */

#include "engine/TrisTypes.hpp"

namespace gmTris
{

/**
 * @class TurnFlow
 * @brief Minimal turn state machine: BOOTSTRAP → P1/P2 turns → GAME_OVER.
 */
class TurnFlow
{
  public:
	/// @brief Starts a session with @p first_turn as the active phase.
	void start(Phase first_turn);

	/// @brief Returns the current phase.
	Phase phase() const;

	/// @brief Switches to the given phase.
	void set_phase(Phase phase);

	/// @brief Returns true while the match has not reached GAME_OVER.
	bool is_active() const;

  private:
	Phase _phase = Phase::BOOTSTRAP;
};

} // namespace gmTris

#endif // GMTRIS_TURNFLOW_HPP
