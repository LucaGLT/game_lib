/**
 * @file rules/WinRules.cpp
 * @brief Implementation of Tic-Tac-Toe win/draw evaluation.
 */

#include "WinRules.hpp"

#include <array>
#include <cstdint>

namespace gmTris
{

namespace
{

/// @brief A winning line as three (row, col) 1-based coordinate pairs plus id.
struct Line
{
	uint8_t     r1, c1, r2, c2, r3, c3;
	const char* id;
};

/// @brief The 8 winning lines of Tic-Tac-Toe.
constexpr std::array<Line, 8> WINNING_LINES = {{
    {1, 1, 1, 2, 1, 3, "row_1"},
    {2, 1, 2, 2, 2, 3, "row_2"},
    {3, 1, 3, 2, 3, 3, "row_3"},
    {1, 1, 2, 1, 3, 1, "col_1"},
    {1, 2, 2, 2, 3, 2, "col_2"},
    {1, 3, 2, 3, 3, 3, "col_3"},
    {1, 1, 2, 2, 3, 3, "diag_main"},
    {1, 3, 2, 2, 3, 1, "diag_anti"},
}};

} // namespace

Evaluation WinRules::evaluate(const Board& board) const
{
	for (const Line& line : WINNING_LINES)
	{
		const Mark a = board.at(line.r1, line.c1);
		const Mark b = board.at(line.r2, line.c2);
		const Mark c = board.at(line.r3, line.c3);

		if (a != Mark::EMPTY && a == b && b == c)
		{
			return Evaluation{Outcome::WIN, a, line.id};
		}
	}

	if (board.is_full())
	{
		return Evaluation{Outcome::DRAW, Mark::EMPTY, ""};
	}

	return Evaluation{Outcome::NONE, Mark::EMPTY, ""};
}

} // namespace gmTris
