#ifndef GMTRIS_BOARD_HPP
#define GMTRIS_BOARD_HPP

/**
 * @file board/Board.hpp
 * @brief 3x3 Tic-Tac-Toe board state, backed by a gmMap graph.
 *
 * The board is modelled as a @ref gmMap::gmMap with 9 locations (one per cell)
 * grouped into a single tile, with each cell's mark stored as the location
 * metadata key @c "mark". Orthogonal grid neighbours are wired as bidirectional
 * adjacency edges so the topology is a faithful gmMap representation of the
 * 3x3 grid. The public interface keeps 1-based (row, col) coordinates so the
 * rest of the engine is unaffected by the backing store.
 */

#include "engine/TrisTypes.hpp"

#include "gmMap/gmMap.hpp"

#include <cstdint>
#include <string>

namespace gmTris
{

/**
 * @class Board
 * @brief Holds the 3x3 grid in a gmMap and exposes 1-based cell access.
 */
class Board
{
  public:
	/// @brief Number of rows/columns of the grid.
	static constexpr uint8_t SIZE = 3;

	/// @brief Builds the 9-location grid (empty cells).
	Board();

	/// @brief Resets every cell to @ref Mark::EMPTY (rebuilds the grid).
	void reset();

	/**
	 * @brief Returns true if @p row and @p col are within [1, SIZE].
	 * @param row 1-based row index.
	 * @param col 1-based column index.
	 */
	bool in_range(uint8_t row, uint8_t col) const;

	/**
	 * @brief Returns the mark stored at (@p row, @p col).
	 * @pre @ref in_range(row, col) is true.
	 */
	Mark at(uint8_t row, uint8_t col) const;

	/**
	 * @brief Sets the mark at (@p row, @p col).
	 * @pre @ref in_range(row, col) is true.
	 */
	void set(uint8_t row, uint8_t col, Mark mark);

	/// @brief Returns true if the cell at (@p row, @p col) is empty.
	bool is_empty(uint8_t row, uint8_t col) const;

	/// @brief Returns true when no cell is empty.
	bool is_full() const;

	/// @brief Returns the gmMap LocationId for a 1-based (row, col) cell.
	gmMap::LocationId location_of(uint8_t row, uint8_t col) const;

	/// @brief Const access to the backing map (used by the rules adapter).
	const gmMap::gmMap<std::string>& map() const;

  private:
	/// @brief Metadata key under which each cell stores its mark symbol.
	static constexpr const char* META_MARK = "mark";

	/// @brief TileId grouping all board cells.
	static constexpr gmMap::TileId BOARD_TILE = 1;

	/// @brief (Re)creates the 9 locations, the tile, adjacency and empty marks.
	void build();

	gmMap::gmMap<std::string> _map;
};

} // namespace gmTris

#endif // GMTRIS_BOARD_HPP
