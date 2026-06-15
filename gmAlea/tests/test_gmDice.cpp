/**
 * @file tests/test_gmDice.cpp
 * @brief Unit tests for GmDice custom dice facade.
 *
 * Build (from game_lib root):
 *   g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/SimpleDeck.cpp gmAlea/GmDice.cpp \
 *       gmAlea/tests/test_gmDice.cpp -o test_gmDice && ./test_gmDice
 */

#include "../GmDice.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace gmAlea;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " - " << reason << "\n";
	++g_fail;
}

static bool all_values_in_set(const std::vector<int>& values, const std::vector<int>& allowed)
{
	for (int value : values)
	{
		if (std::find(allowed.begin(), allowed.end(), value) == allowed.end())
		{
			return false;
		}
	}
	return true;
}

static int mean_round(const std::vector<int>& values)
{
	int sum = std::accumulate(values.begin(), values.end(), 0);
	double mean = static_cast<double>(sum) / static_cast<double>(values.size());
	return static_cast<int>(std::round(mean));
}

static void test_constructor_empty_faces_throws()
{
	const std::string test_name = "constructor_empty_faces_throws";
	try
	{
		GmDice dice({});
		(void)dice;
		fail(test_name, "no exception thrown");
	}
	catch (const EAleaError&)
	{
		pass(test_name);
	}
	catch (...)
	{
		fail(test_name, "wrong exception type");
	}
}

static void test_roll_one_value_from_face_set()
{
	const std::string test_name = "roll_one_value_from_face_set";
	try
	{
		const std::vector<int> faces = {-1, 0, 1, 3, 3};
		GmDice dice(faces, 42);

		std::vector<int> rolled;
		for (int i = 0; i < 40; ++i)
		{
			rolled.push_back(dice.roll_one());
		}

		if (all_values_in_set(rolled, {-1, 0, 1, 3}))
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "found values outside face set");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_invalid_count_throws()
{
	const std::string test_name = "roll_invalid_count_throws";
	try
	{
		GmDice dice({1, 2, 3}, 7);
		int result = dice.roll(0);
		(void)result;
		fail(test_name, "no exception thrown");
	}
	catch (const EAleaError&)
	{
		pass(test_name);
	}
	catch (...)
	{
		fail(test_name, "wrong exception type");
	}
}

static void test_roll_sum_matches_rolled_out()
{
	const std::string test_name = "roll_sum_matches_rolled_out";
	try
	{
		GmDice dice({1, 2, 3, 4, 5, 6}, 100);
		std::vector<int> rolled;
		int result = dice.roll(12, DiceAlgo::ALGO_SUM, &rolled);
		int expected = std::accumulate(rolled.begin(), rolled.end(), 0);

		if (static_cast<int>(rolled.size()) == 12 && result == expected)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "sum mismatch");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_min_matches_rolled_out()
{
	const std::string test_name = "roll_min_matches_rolled_out";
	try
	{
		GmDice dice({-3, -1, 2, 4}, 101);
		std::vector<int> rolled;
		int result = dice.roll(15, DiceAlgo::ALGO_MIN, &rolled);
		int expected = *std::min_element(rolled.begin(), rolled.end());

		if (static_cast<int>(rolled.size()) == 15 && result == expected)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "min mismatch");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_max_matches_rolled_out()
{
	const std::string test_name = "roll_max_matches_rolled_out";
	try
	{
		GmDice dice({-3, -1, 2, 4}, 102);
		std::vector<int> rolled;
		int result = dice.roll(15, DiceAlgo::ALGO_MAX, &rolled);
		int expected = *std::max_element(rolled.begin(), rolled.end());

		if (static_cast<int>(rolled.size()) == 15 && result == expected)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "max mismatch");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_mean_round_matches_rolled_out()
{
	const std::string test_name = "roll_mean_round_matches_rolled_out";
	try
	{
		GmDice dice({1, 2, 3, 4, 5, 6}, 103);
		std::vector<int> rolled;
		int result = dice.roll(11, DiceAlgo::ALGO_MEAN_ROUND, &rolled);
		int expected = mean_round(rolled);

		if (static_cast<int>(rolled.size()) == 11 && result == expected)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "mean_round mismatch");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_with_null_output_pointer()
{
	const std::string test_name = "roll_with_null_output_pointer";
	try
	{
		GmDice dice({1, 2, 3, 4, 5, 6}, 104);
		int result = dice.roll(8, DiceAlgo::ALGO_SUM, nullptr);

		if (result >= 8 && result <= 48)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "sum out of expected bounds");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_faces_count_includes_duplicates()
{
	const std::string test_name = "faces_count_includes_duplicates";
	try
	{
		GmDice dice({1, 1, 1, 2, 3}, 105);
		if (dice.faces_count() == 5)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "expected faces_count == 5");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_same_seed_same_sequence()
{
	const std::string test_name = "same_seed_same_sequence";
	try
	{
		GmDice d1({1, 2, 3, 4, 5, 6}, 200);
		GmDice d2({1, 2, 3, 4, 5, 6}, 200);

		bool equal = true;
		for (int i = 0; i < 40; ++i)
		{
			int a = d1.roll_one();
			int b = d2.roll_one();
			if (a != b)
			{
				equal = false;
				break;
			}
		}

		equal ? pass(test_name) : fail(test_name, "sequences differ with same seed");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_reseed_same_state_same_future_sequence()
{
	const std::string test_name = "reseed_same_state_same_future_sequence";
	try
	{
		GmDice d1({1, 2, 3, 4, 5, 6}, 300);
		GmDice d2({1, 2, 3, 4, 5, 6}, 300);

		for (int i = 0; i < 10; ++i)
		{
			(void)d1.roll_one();
			(void)d2.roll_one();
		}

		d1.reseed(999);
		d2.reseed(999);

		bool equal = true;
		for (int i = 0; i < 30; ++i)
		{
			int a = d1.roll_one();
			int b = d2.roll_one();
			if (a != b)
			{
				equal = false;
				break;
			}
		}

		equal ? pass(test_name) : fail(test_name, "sequences differ after reseed");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_weighted_faces_are_representable()
{
	const std::string test_name = "weighted_faces_are_representable";
	try
	{
		GmDice dice({1, 1, 1, 2, 3}, 400);
		std::vector<int> rolled;
		for (int i = 0; i < 50; ++i)
		{
			rolled.push_back(dice.roll_one());
		}

		if (dice.faces_count() == 5 && all_values_in_set(rolled, {1, 2, 3}))
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "invalid results for weighted die");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

int main()
{
	std::cout << "=== GmDice unit tests ===\n\n";

	test_constructor_empty_faces_throws();
	test_roll_one_value_from_face_set();
	test_roll_invalid_count_throws();
	test_roll_sum_matches_rolled_out();
	test_roll_min_matches_rolled_out();
	test_roll_max_matches_rolled_out();
	test_roll_mean_round_matches_rolled_out();
	test_roll_with_null_output_pointer();
	test_faces_count_includes_duplicates();
	test_same_seed_same_sequence();
	test_reseed_same_state_same_future_sequence();
	test_weighted_faces_are_representable();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
