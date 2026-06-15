#ifndef GMALEA_GMDICE_HPP
#define GMALEA_GMDICE_HPP

/**
 * @file  GmDice.hpp
 * @brief Facade for custom dice with arbitrary face values and weights.
 */

#include "SimpleDeck.hpp"

#include <optional>
#include <vector>

namespace gmAlea
{

/**
 * @brief Aggregation algorithm applied when rolling multiple dice.
 */
enum class DiceAlgo
{
	ALGO_SUM,        ///< Sum of all rolled values.
	ALGO_MIN,        ///< Minimum value among all rolls.
	ALGO_MAX,        ///< Maximum value among all rolls.
	ALGO_MEAN_ROUND  ///< Arithmetic mean, rounded to nearest integer.
};

/**
 * @class GmDice
 * @brief Custom die whose faces carry any integer values, including repeats.
 *
 * GmDice is a domain facade built on SimpleDeck. Each face of the die is
 * stored as a Token. Rolling is simulated by shuffling the face pool and
 * reading the top face without removal, producing an independent,
 * uniformly-weighted draw per roll.
 *
 * Repeating a face value in the constructor increases its probability:
 * {1,1,1,2,3} gives face "1" a 3/5 probability, simulating a biased die.
 *
 * Ownership model:
 *   - The face pool is owned by this instance.
 *   - The face pool contents never change after construction.
 *
 * @note Not thread-safe.
 */
class GmDice
{
public:
	/**
	 * @brief Constructs a GmDice from a list of face values.
	 *
	 * Faces may contain duplicates to represent weighted probability.
	 * Order of values does not affect roll distribution.
	 *
	 * @param face_values  List of integer face values (must not be empty).
	 * @param seed         Optional seed for deterministic rolling.
	 *
	 * @throws EAleaError if @p face_values is empty.
	 */
	explicit GmDice(const std::vector<int>& face_values,
					std::optional<unsigned int> seed = std::nullopt);

	/**
	 * @brief Rolls the die once and returns the face value.
	 *
	 * @return The rolled integer face value.
	 */
	int roll_one();

	/**
	 * @brief Rolls the die @p num_of_dices times and returns an aggregated result.
	 *
	 * Each roll is independent (with replacement). The aggregated result is
	 * computed according to @p algo.
	 *
	 * @param num_of_dices  Number of independent rolls (must be >= 1).
	 * @param algo          Aggregation algorithm applied to all results.
	 * @param rolled_out    If not null, receives each individual roll value.
	 *
	 * @return Aggregated result according to @p algo.
	 * @throws EAleaError if @p num_of_dices < 1.
	 */
	int roll(int num_of_dices = 1,
			 DiceAlgo algo = DiceAlgo::ALGO_SUM,
			 std::vector<int>* rolled_out = nullptr);

	/**
	 * @brief Rolls the die @p num_of_dices times and writes each result into @p rolled_out.
	 *
	 * Overload that accepts a reference instead of a pointer — preferred for new code.
	 * @p rolled_out is cleared and replaced with the individual roll values.
	 *
	 * @param num_of_dices  Number of independent rolls (must be >= 1).
	 * @param algo          Aggregation algorithm applied to all results.
	 * @param rolled_out    Vector that receives each individual roll value.
	 *
	 * @return Aggregated result according to @p algo.
	 * @throws EAleaError if @p num_of_dices < 1.
	 */
	int roll(int num_of_dices,
			 DiceAlgo algo,
			 std::vector<int>& rolled_out);

	/**
	 * @brief Returns the number of faces in this die, including duplicate entries.
	 *
	 * @return Face count.
	 */
	int faces_count() const;

	/**
	 * @brief Reseeds the internal RNG without altering the face pool.
	 *
	 * Useful for reproducible test sequences or replays.
	 *
	 * @param seed  New random seed.
	 */
	void reseed(unsigned int seed);

private:
	// Face pool; shuffled on every roll_one() call, never drawn from
	SimpleDeck _faces;

	// Total face count (including duplicate entries)
	int _faces_count;

	// Build Token vector from raw face values, assigning sequential unique IDs
	static std::vector<Token> _build_tokens(const std::vector<int>& face_values);

	// Compute the aggregate result from a non-empty list of individual rolls
	static int _aggregate(const std::vector<int>& rolls, DiceAlgo algo);
};

} // namespace gmAlea

#endif // GMALEA_GMDICE_HPP
