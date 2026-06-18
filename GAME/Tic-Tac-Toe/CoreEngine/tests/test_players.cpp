/**
 * @file tests/test_players.cpp
 * @brief Unit tests for player identity/status tracking and the starter logic.
 *
 * Covers the gmActor-backed Players wrapper (turn ownership and end-game result
 * statuses) and the gmAlea-backed Starter (fixed vs 1d2 first-player choice).
 */

#include "alea/Starter.hpp"
#include "engine/TrisTypes.hpp"
#include "players/Players.hpp"
#include "tests/test_harness.hpp"

#include <string>

using namespace gmTris;
using gmtris_test::check;

namespace
{

void test_players_identity()
{
	Players    players;
	const bool ok = players.actor_id(Mark::X) == "Player_X" &&
	                players.actor_id(Mark::O) == "Player_O" &&
	                players.display_name(Mark::X) == "Player X" &&
	                players.display_name(Mark::O) == "Player O";
	check("players_identity", ok);
}

void test_players_opponent()
{
	const bool ok = Players::opponent(Mark::X) == Mark::O &&
	                Players::opponent(Mark::O) == Mark::X;
	check("players_opponent", ok);
}

void test_players_set_active_moves_turn()
{
	Players players;
	players.set_active(Mark::X);
	const bool x_active = players.active() == Mark::X &&
	                      players.has_status(Mark::X, "ACTIVE_TURN") &&
	                      !players.has_status(Mark::O, "ACTIVE_TURN");

	players.set_active(Mark::O);
	const bool o_active = players.active() == Mark::O &&
	                      players.has_status(Mark::O, "ACTIVE_TURN") &&
	                      !players.has_status(Mark::X, "ACTIVE_TURN");

	check("players_set_active_moves_turn", x_active && o_active);
}

void test_players_mark_winner()
{
	Players players;
	players.set_active(Mark::X);
	players.mark_winner(Mark::X);
	const bool ok = players.has_status(Mark::X, "WINNER") &&
	                !players.has_status(Mark::X, "ACTIVE_TURN") &&
	                !players.has_status(Mark::O, "ACTIVE_TURN");
	check("players_mark_winner", ok);
}

void test_players_mark_draw()
{
	Players players;
	players.set_active(Mark::O);
	players.mark_draw();
	const bool ok = players.has_status(Mark::X, "DRAW") &&
	                players.has_status(Mark::O, "DRAW") &&
	                !players.has_status(Mark::X, "ACTIVE_TURN") &&
	                !players.has_status(Mark::O, "ACTIVE_TURN");
	check("players_mark_draw", ok);
}

void test_players_reset_statuses()
{
	Players players;
	players.set_active(Mark::X);
	players.mark_winner(Mark::X);
	players.reset_statuses();
	const bool ok = !players.has_status(Mark::X, "WINNER") &&
	                !players.has_status(Mark::X, "ACTIVE_TURN") &&
	                !players.has_status(Mark::O, "DRAW");
	check("players_reset_statuses", ok);
}

void test_starter_fixed_x()
{
	Starter             starter;
	const StarterResult result = starter.choose(StarterMode::FIXED_X);
	const bool          ok = result.first == Mark::X && !result.used_dice;
	check("starter_fixed_x", ok);
}

void test_starter_dice_distribution()
{
	Starter starter;
	bool    saw_x        = false;
	bool    saw_o        = false;
	bool    rolls_in_set = true;
	bool    always_dice  = true;

	for (int i = 0; i < 200; ++i)
	{
		const StarterResult result = starter.choose(StarterMode::DICE_1D2);
		if (!result.used_dice)
		{
			always_dice = false;
		}
		if (result.roll != 1 && result.roll != 2)
		{
			rolls_in_set = false;
		}
		// Mapping: 1 -> X, 2 -> O.
		if ((result.roll == 1 && result.first != Mark::X) ||
		    (result.roll == 2 && result.first != Mark::O))
		{
			rolls_in_set = false;
		}
		if (result.first == Mark::X)
		{
			saw_x = true;
		}
		else if (result.first == Mark::O)
		{
			saw_o = true;
		}
	}

	check("starter_dice_distribution",
	      saw_x && saw_o && rolls_in_set && always_dice,
	      "expected both X and O over 200 rolls, all dice in {1,2}");
}

} // namespace

int main()
{
	test_players_identity();
	test_players_opponent();
	test_players_set_active_moves_turn();
	test_players_mark_winner();
	test_players_mark_draw();
	test_players_reset_statuses();

	test_starter_fixed_x();
	test_starter_dice_distribution();

	return gmtris_test::summary("test_players");
}
