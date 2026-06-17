/**
 * @file players/Players.cpp
 * @brief Implementation of player identity and active-turn tracking.
 */

#include "Players.hpp"

namespace gmTris
{

std::string Players::actor_id(Mark mark) const
{
	return (mark == Mark::O) ? "Player_O" : "Player_X";
}

std::string Players::display_name(Mark mark) const
{
	return (mark == Mark::O) ? "Player O" : "Player X";
}

void Players::set_active(Mark mark)
{
	_active = mark;
}

Mark Players::active() const
{
	return _active;
}

Mark Players::opponent(Mark mark)
{
	return (mark == Mark::X) ? Mark::O : Mark::X;
}

} // namespace gmTris
