#ifndef GMTRIS_BOARD_HPP
#define GMTRIS_BOARD_HPP

/**
 * @file board/Board.hpp
 * @brief 3x3 Tic-Tac-Toe board state.
 *
 * @note Phase 1 stores the grid in a plain fixed array. Phase 2 will back the
 *       board with a gmMap instance (9 locations + a "mark" metadata field).
 */

#include "engine/TrisTypes.hpp"

#include <array>
#include <cstdint>

namespace gmTris
{

/**
 * @class Board
 * @brief Holds the 3x3 grid and exposes cell read/write with 1-based coords.
 */
class Board
{
  public:
	/// @brief Number of rows/columns of the grid.
	static constexpr uint8_t SIZE = 3;

	/// @brief Resets every cell to @ref Mark::EMPTY.
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

  private:
	/// @brief Converts 1-based coords to a flat 0-based index.
	std::size_t index(uint8_t row, uint8_t col) const;

	std::array<Mark, SIZE * SIZE> _cells{};
};

} // namespace gmTris

#endif // GMTRIS_BOARD_HPP
