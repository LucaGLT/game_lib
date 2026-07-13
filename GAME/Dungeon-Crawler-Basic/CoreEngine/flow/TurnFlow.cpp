/**
 * @file flow/TurnFlow.cpp
 * @brief TurnFlow implementation backed by gmFlow round/turn primitives.
 */

#include "flow/TurnFlow.hpp"

#include <stdexcept>

namespace gmDungeonBasic
{

TurnFlow::TurnFlow()
{
	reset();
}

void TurnFlow::start_session()
{
	_session_active = true;
	_turn_active = false;
	_round_index = 1;
	_turn_index = 0;
	_current_actor_id.clear();
	_current_round = std::make_unique<gmFlow::Round>("round_1", 1);
	_current_turn.reset();
}

void TurnFlow::end_session()
{
	reset();
}

bool TurnFlow::is_session_active() const
{
	return _session_active;
}

void TurnFlow::start_turn(const std::string& actor_id)
{
	if (!_session_active)
	{
		throw std::logic_error("Cannot start turn without active session");
	}

	_current_actor_id = actor_id;
	_turn_active = true;

	const std::string turn_id =
		"round_" + std::to_string(_round_index) + "_turn_" + std::to_string(_turn_index + 1);
	_current_turn = std::make_unique<gmFlow::Turn>(turn_id);
	_current_turn->add_active_actor(actor_id);
}

void TurnFlow::end_turn()
{
	if (!_turn_active)
	{
		throw std::logic_error("Cannot end turn because no turn is active");
	}

	_turn_active = false;
	_current_turn.reset();

	if (_actor_order.empty())
	{
		_current_actor_id.clear();
		return;
	}

	if (_turn_index + 1 < _actor_order.size())
	{
		++_turn_index;
	}
	else
	{
		++_round_index;
		_turn_index = 0;
		const std::string round_id = "round_" + std::to_string(_round_index);
		_current_round = std::make_unique<gmFlow::Round>(round_id, _round_index);
	}

	_current_actor_id.clear();
}

bool TurnFlow::is_turn_active() const
{
	return _turn_active;
}

std::string TurnFlow::current_actor_id() const
{
	if (!_turn_active)
	{
		return "";
	}
	return _current_actor_id;
}

int TurnFlow::current_round() const
{
	if (!_session_active)
	{
		return 0;
	}
	return _round_index;
}

void TurnFlow::set_actor_order(const std::vector<std::string>& actor_order)
{
	_actor_order = actor_order;
	if (_session_active && !_turn_active && _turn_index >= _actor_order.size())
	{
		_turn_index = 0;
	}
}

std::string TurnFlow::next_actor_id() const
{
	if (!_session_active || _actor_order.empty())
	{
		return "";
	}

	if (_turn_active)
	{
		if (_turn_index + 1 < _actor_order.size())
		{
			return _actor_order[_turn_index + 1];
		}
		return "";
	}

	if (_turn_index < _actor_order.size())
	{
		return _actor_order[_turn_index];
	}

	return "";
}

void TurnFlow::reset()
{
	_session_active = false;
	_turn_active = false;
	_round_index = 0;
	_turn_index = 0;
	_actor_order.clear();
	_current_actor_id.clear();
	_current_round.reset();
	_current_turn.reset();
}

} // namespace gmDungeonBasic
