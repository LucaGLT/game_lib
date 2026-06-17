/**
 * @file tests/test_rules.cpp
 * @brief Unit tests for the move mechanics (Board) and win/draw rules (WinRules).
 *
 * Covers the Phase 4 requirement "Unit test C++ regole mossa/win/draw" at the
 * level of the pure, network-free wrappers:
 *   - Board: coordinate range, set/at, emptiness, fullness, gmMap LocationId map.
 *   - WinRules: all 8 winning lines (for X and O), draw on a full board, and the
 *     in-progress (NONE) outcome.
 *
 * Engine-level move validation edge cases (wrong turn, game over, occupied cell,
 * out of range, restart) are exercised by the Python end-to-end test against the
 * real engine executable.
 */

#include "board/Board.hpp"
#include "engine/TrisTypes.hpp"
#include "rules/WinRules.hpp"
#include "tests/test_harness.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace gmTris;
using gmtris_test::check;

namespace
{

using Cell = std::pair<uint8_t, uint8_t>;

/// @brief The 8 winning lines as (line id, three cells), mirroring WinRules.
const std::vector<std::pair<std::string, std::array<Cell, 3>>> WINNING_LINES = {
    {"row_1", {Cell{1, 1}, Cell{1, 2}, Cell{1, 3}}},
    {"row_2", {Cell{2, 1}, Cell{2, 2}, Cell{2, 3}}},
    {"row_3", {Cell{3, 1}, Cell{3, 2}, Cell{3, 3}}},
    {"col_1", {Cell{1, 1}, Cell{2, 1}, Cell{3, 1}}},
    {"col_2", {Cell{1, 2}, Cell{2, 2}, Cell{3, 2}}},
    {"col_3", {Cell{1, 3}, Cell{2, 3}, Cell{3, 3}}},
    {"diag_main", {Cell{1, 1}, Cell{2, 2}, Cell{3, 3}}},
    {"diag_anti", {Cell{1, 3}, Cell{2, 2}, Cell{3, 1}}},
};

void test_board_starts_empty()
{
	Board board;
	bool  all_empty = true;
	for (uint8_t row = 1; row <= Board::SIZE; ++row)
	{
		for (uint8_t col = 1; col <= Board::SIZE; ++col)
		{
			if (!board.is_empty(row, col) || board.at(row, col) != Mark::EMPTY)
			{
				all_empty = false;
			}
		}
	}
	check("board_starts_empty", all_empty && !board.is_full());
}

void test_board_in_range()
{
	Board board;
	const bool inside = board.in_range(1, 1) && board.in_range(3, 3) &&
	                    board.in_range(2, 2);
	const bool outside = !board.in_range(0, 1) && !board.in_range(1, 0) &&
	                     !board.in_range(4, 1) && !board.in_range(1, 4);
	check("board_in_range", inside && outside);
}

void test_board_set_and_at()
{
	Board board;
	board.set(1, 1, Mark::X);
	board.set(2, 3, Mark::O);
	const bool ok = board.at(1, 1) == Mark::X && !board.is_empty(1, 1) &&
	                board.at(2, 3) == Mark::O && board.is_empty(2, 2);
	check("board_set_and_at", ok);
}

void test_board_location_ids()
{
	Board      board;
	const bool ok = board.location_of(1, 1) == 1 && board.location_of(1, 2) == 2 &&
	                board.location_of(1, 3) == 3 && board.location_of(2, 1) == 4 &&
	                board.location_of(3, 3) == 9;
	check("board_location_ids", ok, "expected LocationId = (row-1)*3 + col");
}

void test_board_reset()
{
	Board board;
	board.set(1, 1, Mark::X);
	board.set(3, 3, Mark::O);
	board.reset();
	const bool ok = board.is_empty(1, 1) && board.is_empty(3, 3) && !board.is_full();
	check("board_reset", ok);
}

void test_board_is_full()
{
	Board board;
	for (uint8_t row = 1; row <= Board::SIZE; ++row)
	{
		for (uint8_t col = 1; col <= Board::SIZE; ++col)
		{
			board.set(row, col, Mark::X);
		}
	}
	check("board_is_full", board.is_full());
}

void test_rules_none_on_empty_board()
{
	Board    board;
	WinRules rules;
	check("rules_none_on_empty_board", rules.evaluate(board).outcome == Outcome::NONE);
}

void test_rules_all_lines_win()
{
	WinRules rules;
	for (Mark mark : {Mark::X, Mark::O})
	{
		for (const auto& line : WINNING_LINES)
		{
			Board board;
			for (const Cell& cell : line.second)
			{
				board.set(cell.first, cell.second, mark);
			}
			const Evaluation eval = rules.evaluate(board);
			const bool       ok   = eval.outcome == Outcome::WIN &&
			                  eval.winner == mark && eval.line == line.first;
			check("rules_win_" + line.first + "_" + mark_to_string(mark), ok,
			      "expected WIN on line " + line.first);
		}
	}
}

void test_rules_draw_on_full_board()
{
	// A full board with no three-in-a-row:
	//   X O X
	//   X O O
	//   O X X
	Board board;
	board.set(1, 1, Mark::X);
	board.set(1, 2, Mark::O);
	board.set(1, 3, Mark::X);
	board.set(2, 1, Mark::X);
	board.set(2, 2, Mark::O);
	board.set(2, 3, Mark::O);
	board.set(3, 1, Mark::O);
	board.set(3, 2, Mark::X);
	board.set(3, 3, Mark::X);

	WinRules         rules;
	const Evaluation eval = rules.evaluate(board);
	check("rules_draw_on_full_board",
	      eval.outcome == Outcome::DRAW && board.is_full());
}

void test_rules_none_in_progress()
{
	// Partially filled, no win yet.
	Board board;
	board.set(1, 1, Mark::X);
	board.set(2, 2, Mark::O);
	board.set(1, 2, Mark::X);

	WinRules rules;
	check("rules_none_in_progress", rules.evaluate(board).outcome == Outcome::NONE);
}

} // namespace

int main()
{
	test_board_starts_empty();
	test_board_in_range();
	test_board_set_and_at();
	test_board_location_ids();
	test_board_reset();
	test_board_is_full();

	test_rules_none_on_empty_board();
	test_rules_all_lines_win();
	test_rules_draw_on_full_board();
	test_rules_none_in_progress();

	return gmtris_test::summary("test_rules");
}
