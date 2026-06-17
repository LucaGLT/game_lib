#ifndef GMTRIS_WINRULES_HPP
#define GMTRIS_WINRULES_HPP

/**
 * @file rules/WinRules.hpp
 * @brief Win/draw evaluation for a Tic-Tac-Toe board.
 *
 * @note Phase 1 evaluates the 8 winning lines directly. Phase 2 will express
 *       the same conditions through a gmRules engine instance.
 */

#include "board/Board.hpp"
#include "engine/TrisTypes.hpp"

#include <string>

namespace gmTris
{

/// @brief Outcome of evaluating a board position.
enum class Outcome
{
	NONE, ///< No winner yet and the board is not full.
	WIN,  ///< A player completed a line (see Evaluation::winner).
	DRAW  ///< The board is full with no winner.
};

/// @brief Result of a board evaluation.
struct Evaluation
{
	Outcome     outcome = Outcome::NONE; ///< Win, draw or none.
	Mark        winner  = Mark::EMPTY;   ///< Winning mark when outcome == WIN.
	std::string line;                    ///< Winning line id when outcome == WIN.
};

/**
 * @class WinRules
 * @brief Stateless evaluator of Tic-Tac-Toe terminal conditions.
 */
class WinRules
{
  public:
	/// @brief Evaluates @p board for a win or a draw.
	Evaluation evaluate(const Board& board) const;
};

} // namespace gmTris

#endif // GMTRIS_WINRULES_HPP
