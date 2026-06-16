#ifndef GMALEA_STDDICE_HPP
#define GMALEA_STDDICE_HPP

/**
 * @file  StdDice.hpp
 * @brief Convenience facade for standard numeric range dice (d4, d6, d10, d100, …).
 */

#include "GmDice.hpp"

#include <optional>
#include <vector>

namespace gmAlea
{

/**
 * @class StdDice
 * @brief Standard die with contiguous integer faces [min..max].
 *
 * StdDice is a convenience facade on GmDice for the common case of a die
 * with sequential integer faces. The default constructor produces a d6.
 *
 * Constructor forms:
 *   - @ref StdDice(int, std::optional) for [1..max] range (e.g. d6, d10, d100).
 *   - @ref StdDice(int, int, std::optional) for custom [min..max] range.
 *
 * Examples:
 * @code
 *   StdDice d6;               // faces: 1,2,3,4,5,6
 *   StdDice d10(10);          // faces: 1,2,...,10
 *   StdDice fudge(-1, 1);     // faces: -1, 0, +1
 *   StdDice d100(1, 100, 42); // faces: 1..100, seeded
 * @endcode
 *
 * @note Not thread-safe.
 */
class StdDice
{
public:
	/**
	 * @brief Constructs a standard die with faces [1..max].
	 *
	 * @param max   Maximum face value (must be >= 1).
	 * @param seed  Optional seed for deterministic rolling.
	 *
	 * @throws EAleaError if @p max < 1.
	 */
	explicit StdDice(int max = 6,
					 std::optional<unsigned int> seed = std::nullopt);

	/**
	 * @brief Constructs a custom-range die with faces [min..max].
	 *
	 * @param min   Minimum face value.
	 * @param max   Maximum face value (must be >= @p min).
	 * @param seed  Optional seed for deterministic rolling.
	 *
	 * @throws EAleaError if @p max < @p min.
	 */
	StdDice(int min, int max,
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
	 * @param num_of_dices  Number of independent rolls (must be >= 1).
	 * @param algo          Aggregation algorithm (default: ALGO_SUM).
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
	 * @brief Returns the minimum face value.
	 *
	 * @return Minimum face value.
	 */
	int min_face() const;

	/**
	 * @brief Returns the maximum face value.
	 *
	 * @return Maximum face value.
	 */
	int max_face() const;

	/**
	 * @brief Returns the number of distinct faces (max - min + 1).
	 *
	 * @return Face count.
	 */
	int faces_count() const;

	/**
	 * @brief Reseeds the internal RNG without altering the face pool.
	 *
	 * @param seed  New random seed.
	 */
	void reseed(unsigned int seed);

private:
	// Underlying custom die
	GmDice _die;

	// Stored range bounds for min_face() and max_face()
	int _min;
	int _max;

	// Build sequential face list [min..max]; throws if max < min
	static std::vector<int> _build_faces(int min, int max);
};

} // namespace gmAlea

#endif // GMALEA_STDDICE_HPP
