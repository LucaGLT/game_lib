#include "GmDice.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace gmAlea
{

std::vector<Token> GmDice::_build_tokens(const std::vector<int>& face_values)
{
	if (face_values.empty())
	{
		throw EAleaError("GmDice: face_values must not be empty");
	}

	std::vector<Token> tokens;
	uint32_t face_id = 0;

	for (int v : face_values)
	{
		Token t;
		t.id    = face_id++;
		t.label = std::to_string(v);
		t.value = v;
		tokens.push_back(t);
	}

	return tokens;
}

int GmDice::_aggregate(const std::vector<int>& rolls, DiceAlgo algo)
{
	switch (algo)
	{
		case DiceAlgo::ALGO_SUM:
			return std::accumulate(rolls.begin(), rolls.end(), 0);

		case DiceAlgo::ALGO_MIN:
			return *std::min_element(rolls.begin(), rolls.end());

		case DiceAlgo::ALGO_MAX:
			return *std::max_element(rolls.begin(), rolls.end());

		case DiceAlgo::ALGO_MEAN_ROUND:
		{
			int sum      = std::accumulate(rolls.begin(), rolls.end(), 0);
			double mean  = static_cast<double>(sum) / static_cast<double>(rolls.size());
			return static_cast<int>(std::round(mean));
		}
	}

	return 0;
}

GmDice::GmDice(const std::vector<int>& face_values, std::optional<unsigned int> seed)
	: _faces(_build_tokens(face_values), seed),
	  _faces_count(static_cast<int>(face_values.size()))
{
}

int GmDice::roll_one()
{
	_faces.shuffle();
	return _faces.see_top().value;
}

int GmDice::roll(int num_of_dices, DiceAlgo algo, std::vector<int>* rolled_out)
{
	if (num_of_dices < 1)
	{
		throw EAleaError("GmDice: num_of_dices must be >= 1");
	}

	std::vector<int> results;
	results.reserve(static_cast<size_t>(num_of_dices));

	for (int i = 0; i < num_of_dices; ++i)
	{
		results.push_back(roll_one());
	}

	if (rolled_out != nullptr)
	{
		*rolled_out = results;
	}

	return _aggregate(results, algo);
}

int GmDice::faces_count() const
{
	return _faces_count;
}

void GmDice::reseed(unsigned int seed)
{
	_faces.reseed(seed);
}

} // namespace gmAlea
