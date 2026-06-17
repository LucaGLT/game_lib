#ifndef GMTRIS_STARTER_HPP
#define GMTRIS_STARTER_HPP

/**
 * @file alea/Starter.hpp
 * @brief Chooses which player moves first, optionally via a 1d2 dice roll.
 */

#include "engine/TrisTypes.hpp"

#include "gmAlea/GmDice.hpp"

namespace gmTris
{

/// @brief Result of a starter decision.
struct StarterResult
{
	Mark first;       ///< Mark of the player that moves first.
	int  roll = 0;    ///< Dice value when a roll was used, 0 otherwise.
	bool used_dice = false; ///< True when a 1d2 roll decided the starter.
};

/**
 * @class Starter
 * @brief Decides the first player using a fixed rule or a gmAlea 1d2 die.
 */
class Starter
{
  public:
	Starter();

	/**
	 * @brief Returns the starting mark according to @p mode.
	 *
	 * @param mode FIXED_X always returns X; DICE_1D2 rolls a 1d2 (1→X, 2→O).
	 */
	StarterResult choose(StarterMode mode);

  private:
	gmAlea::GmDice _die; ///< A 1d2 die (faces {1, 2}).
};

} // namespace gmTris

#endif // GMTRIS_STARTER_HPP
