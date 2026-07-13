/**
 * @file alea/Starter.cpp
 * @brief Implementation of the first-player chooser.
 */

#include "Starter.hpp"

namespace gmTris
{

Starter::Starter() : _die(std::vector<int>{1, 2})
{
}

StarterResult Starter::choose(StarterMode mode)
{
	if (mode == StarterMode::DICE_1D2)
	{
		const int value = _die.roll_one();
		StarterResult result;
		result.first     = (value == 1) ? Mark::X : Mark::O;
		result.roll      = value;
		result.used_dice = true;
		return result;
	}

	StarterResult result;
	result.first     = Mark::X;
	result.used_dice = false;
	return result;
}

} // namespace gmTris
