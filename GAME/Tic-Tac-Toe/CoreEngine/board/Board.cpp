/**
 * @file board/Board.cpp
 * @brief Implementation of the gmMap-backed 3x3 board.
 */

#include "Board.hpp"

namespace gmTris
{

Board::Board()
{
	build();
}

gmMap::LocationId Board::location_of(uint8_t row, uint8_t col) const
{
	return static_cast<gmMap::LocationId>((row - 1) * SIZE + col);
}

void Board::build()
{
	_map.clear();
	_map.create_tile(BOARD_TILE);

	for (uint8_t row = 1; row <= SIZE; ++row)
	{
		for (uint8_t col = 1; col <= SIZE; ++col)
		{
			const gmMap::LocationId loc = location_of(row, col);
			_map.create_location(loc);
			_map.assign_to_tile(loc, BOARD_TILE);
			_map.set_location_meta(loc, META_MARK, std::string());
		}
	}

	// Wire orthogonal grid neighbours as bidirectional adjacency edges so the
	// gmMap topology mirrors the physical 3x3 grid.
	for (uint8_t row = 1; row <= SIZE; ++row)
	{
		for (uint8_t col = 1; col <= SIZE; ++col)
		{
			const gmMap::LocationId here = location_of(row, col);
			if (col < SIZE)
			{
				_map.set_adjacent(here, location_of(row, col + 1));
			}
			if (row < SIZE)
			{
				_map.set_adjacent(here, location_of(row + 1, col));
			}
		}
	}
}

void Board::reset()
{
	build();
}

bool Board::in_range(uint8_t row, uint8_t col) const
{
	return row >= 1 && row <= SIZE && col >= 1 && col <= SIZE;
}

Mark Board::at(uint8_t row, uint8_t col) const
{
	const gmMap::MetadataValue& value =
	    _map.get_location_meta(location_of(row, col), META_MARK);
	return mark_from_string(std::get<std::string>(value));
}

void Board::set(uint8_t row, uint8_t col, Mark mark)
{
	_map.set_location_meta(location_of(row, col), META_MARK, mark_to_string(mark));
}

bool Board::is_empty(uint8_t row, uint8_t col) const
{
	return at(row, col) == Mark::EMPTY;
}

bool Board::is_full() const
{
	for (uint8_t row = 1; row <= SIZE; ++row)
	{
		for (uint8_t col = 1; col <= SIZE; ++col)
		{
			if (at(row, col) == Mark::EMPTY)
			{
				return false;
			}
		}
	}
	return true;
}

const gmMap::gmMap<std::string>& Board::map() const
{
	return _map;
}

} // namespace gmTris
