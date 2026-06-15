#include "StdDice.hpp"

namespace gmAlea
{

std::vector<int> StdDice::_build_faces(int min, int max)
{
	if (max < min)
	{
		throw EAleaError("StdDice: max must be >= min");
	}

	std::vector<int> faces;

	for (int v = min; v <= max; ++v)
	{
		faces.push_back(v);
	}

	return faces;
}

StdDice::StdDice(int max, std::optional<unsigned int> seed)
	: StdDice(1, max, seed)
{
}

StdDice::StdDice(int min, int max, std::optional<unsigned int> seed)
	: _die(_build_faces(min, max), seed),
	  _min(min),
	  _max(max)
{
}

int StdDice::roll_one()
{
	return _die.roll_one();
}

int StdDice::roll(int num_of_dices, DiceAlgo algo, std::vector<int>* rolled_out)
{
	return _die.roll(num_of_dices, algo, rolled_out);
}

int StdDice::roll(int num_of_dices, DiceAlgo algo, std::vector<int>& rolled_out)
{
	return _die.roll(num_of_dices, algo, rolled_out);
}

int StdDice::min_face() const
{
	return _min;
}

int StdDice::max_face() const
{
	return _max;
}

int StdDice::faces_count() const
{
	return _die.faces_count();
}

void StdDice::reseed(unsigned int seed)
{
	_die.reseed(seed);
}

} // namespace gmAlea
