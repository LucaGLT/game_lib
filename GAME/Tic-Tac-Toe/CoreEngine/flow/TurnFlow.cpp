/**
 * @file flow/TurnFlow.cpp
 * @brief Implementation of the minimal turn state machine.
 */

#include "TurnFlow.hpp"

namespace gmTris
{

void TurnFlow::start(Phase first_turn)
{
	_phase = first_turn;
}

Phase TurnFlow::phase() const
{
	return _phase;
}

void TurnFlow::set_phase(Phase phase)
{
	_phase = phase;
}

bool TurnFlow::is_active() const
{
	return _phase == Phase::PLAYER1_TURN || _phase == Phase::PLAYER2_TURN;
}

} // namespace gmTris
