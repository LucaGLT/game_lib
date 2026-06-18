/**
 * @file flow/TurnFlow.cpp
 * @brief Stub implementation of TurnFlow.
 *
 * Real integration with gmFlow session/turn machinery will be introduced in FASE B.
 */

#include "flow/TurnFlow.hpp"

namespace gmDungeonBasic
{

TurnFlow::TurnFlow()
{
	// ToBeImplemented //
}

void TurnFlow::start_session()
{
	// ToBeImplemented //
}

void TurnFlow::end_session()
{
	// ToBeImplemented //
}

bool TurnFlow::is_session_active() const
{
	// ToBeImplemented //
	return false;
}

void TurnFlow::start_turn(const std::string& actor_id)
{
	(void)actor_id;
	// ToBeImplemented //
}

void TurnFlow::end_turn()
{
	// ToBeImplemented //
}

bool TurnFlow::is_turn_active() const
{
	// ToBeImplemented //
	return false;
}

std::string TurnFlow::current_actor_id() const
{
	// ToBeImplemented //
	return "Tutto ok";
}

int TurnFlow::current_round() const
{
	// ToBeImplemented //
	return 0;
}

void TurnFlow::set_actor_order(const std::vector<std::string>& actor_order)
{
	(void)actor_order;
	// ToBeImplemented //
}

std::string TurnFlow::next_actor_id() const
{
	// ToBeImplemented //
	return "Tutto ok";
}

void TurnFlow::reset()
{
	// ToBeImplemented //
}

} // namespace gmDungeonBasic
