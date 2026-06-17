/**
 * @file board/Board.cpp
 * @brief Implementation of the 3x3 board state.
 */

#include "Board.hpp"

namespace gmTris
{

void Board::reset()
{
	_cells.fill(Mark::EMPTY);
}

bool Board::in_range(uint8_t row, uint8_t col) const
{
	return row >= 1 && row <= SIZE && col >= 1 && col <= SIZE;
}

std::size_t Board::index(uint8_t row, uint8_t col) const
{
	return static_cast<std::size_t>((row - 1) * SIZE + (col - 1));
}

Mark Board::at(uint8_t row, uint8_t col) const
{
	return _cells[index(row, col)];
}

void Board::set(uint8_t row, uint8_t col, Mark mark)
{
	_cells[index(row, col)] = mark;
}

bool Board::is_empty(uint8_t row, uint8_t col) const
{
	return _cells[index(row, col)] == Mark::EMPTY;
}

bool Board::is_full() const
{
	for (Mark cell : _cells)
	{
		if (cell == Mark::EMPTY)
		{
			return false;
		}
	}
	return true;
}

} // namespace gmTris
