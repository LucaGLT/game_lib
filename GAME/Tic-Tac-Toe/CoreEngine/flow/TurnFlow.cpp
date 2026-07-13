/**
 * @file flow/TurnFlow.cpp
 * @brief Turn state machine backed by gmFlow Turn/Round/ActorRegistry.
 */

#include "TurnFlow.hpp"

#include "gmFlow/actors/Actor.hpp"

namespace gmTris
{

const std::string TurnFlow::ACTOR_X = "Player_X";
const std::string TurnFlow::ACTOR_O = "Player_O";

TurnFlow::TurnFlow()
{
	gmFlow::Actor player_x(ACTOR_X, gmFlow::ActorType::PLAYER);
	player_x.set_display_name("Player X");
	gmFlow::Actor player_o(ACTOR_O, gmFlow::ActorType::PLAYER);
	player_o.set_display_name("Player O");
	_registry.add(std::move(player_x));
	_registry.add(std::move(player_o));
}

std::string TurnFlow::actor_for(Phase phase)
{
	if (phase == Phase::PLAYER1_TURN)
	{
		return ACTOR_X;
	}
	if (phase == Phase::PLAYER2_TURN)
	{
		return ACTOR_O;
	}
	return std::string();
}

void TurnFlow::open_turn(Phase phase)
{
	const std::string actor = actor_for(phase);
	if (actor.empty())
	{
		_turn.reset();
		return;
	}

	// A new round begins whenever PLAYER1 (X) is about to act.
	if (phase == Phase::PLAYER1_TURN || !_round)
	{
		++_round_index;
		_round = std::unique_ptr<gmFlow::Round>(
		    new gmFlow::Round("round_" + std::to_string(_round_index), _round_index));
	}

	++_turn_counter;
	_turn = std::unique_ptr<gmFlow::Turn>(
	    new gmFlow::Turn("turn_" + std::to_string(_turn_counter)));
	_turn->add_active_actor(actor);
}

void TurnFlow::start(Phase first_turn)
{
	_round_index  = 0;
	_turn_counter = 0;
	_round.reset();
	_turn.reset();
	_phase = first_turn;
	open_turn(first_turn);
}

Phase TurnFlow::phase() const
{
	return _phase;
}

void TurnFlow::set_phase(Phase phase)
{
	_phase = phase;
	open_turn(phase);
}

bool TurnFlow::is_active() const
{
	return _phase == Phase::PLAYER1_TURN || _phase == Phase::PLAYER2_TURN;
}

int TurnFlow::round_index() const
{
	return _round_index;
}

std::string TurnFlow::active_actor() const
{
	if (!_turn || _turn->active_actors().empty())
	{
		return std::string();
	}
	return _turn->active_actors().front();
}

const gmFlow::ActorRegistry &TurnFlow::registry() const
{
	return _registry;
}

} // namespace gmTris
