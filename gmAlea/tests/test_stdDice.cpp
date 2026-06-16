/**
 * @file tests/test_stdDice.cpp
 * @brief Unit tests for StdDice convenience facade.
 *
 * Build (from game_lib root):
 *   g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/SimpleDeck.cpp gmAlea/GmDice.cpp \
 *       gmAlea/StdDice.cpp gmAlea/tests/test_stdDice.cpp -o test_stdDice && ./test_stdDice
 */

#include "../StdDice.hpp"

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

static int mean_round(const std::vector<int>& values)
{
	int sum = std::accumulate(values.begin(), values.end(), 0);
	double mean = static_cast<double>(sum) / static_cast<double>(values.size());
	return static_cast<int>(std::round(mean));
}

static void test_default_constructor_is_d6()
{
	const std::string test_name = "default_constructor_is_d6";
	try
	{
		StdDice d6;
		bool ok = d6.min_face() == 1 && d6.max_face() == 6 && d6.faces_count() == 6;
		ok ? pass(test_name) : fail(test_name, "expected [1..6]");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_single_param_constructor_builds_1_to_max()
{
	const std::string test_name = "single_param_constructor_builds_1_to_max";
	try
	{
		StdDice d10(10);
		bool ok = d10.min_face() == 1 && d10.max_face() == 10 && d10.faces_count() == 10;
		ok ? pass(test_name) : fail(test_name, "expected [1..10]");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_range_constructor_builds_min_to_max()
{
	const std::string test_name = "range_constructor_builds_min_to_max";
	try
	{
		StdDice fudge(-1, 1);
		bool ok = fudge.min_face() == -1 && fudge.max_face() == 1 && fudge.faces_count() == 3;
		ok ? pass(test_name) : fail(test_name, "expected [-1..1]");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_single_param_invalid_max_throws()
{
	const std::string test_name = "single_param_invalid_max_throws";
	try
	{
		StdDice invalid(0);
		(void)invalid;
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

static void test_range_invalid_bounds_throws()
{
	const std::string test_name = "range_invalid_bounds_throws";
	try
	{
		StdDice invalid(5, 3);
		(void)invalid;
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

static void test_roll_one_within_range()
{
	const std::string test_name = "roll_one_within_range";
	try
	{
		StdDice d20(20, std::optional<unsigned int>(77));
		bool in_range = true;
		for (int i = 0; i < 40; ++i)
		{
			int v = d20.roll_one();
			if (v < 1 || v > 20)
			{
				in_range = false;
				break;
			}
		}

		in_range ? pass(test_name) : fail(test_name, "value out of [1..20]");
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

static void test_roll_sum_matches_rolled_out()
{
	const std::string test_name = "roll_sum_matches_rolled_out";
	try
	{
		StdDice d6(6, 120);
		std::vector<int> rolled;
		int result = d6.roll(10, DiceAlgo::ALGO_SUM, &rolled);
		int expected = std::accumulate(rolled.begin(), rolled.end(), 0);

		if (static_cast<int>(rolled.size()) == 10 && result == expected)
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
		StdDice d100(1, 100, 121);
		std::vector<int> rolled;
		int result = d100.roll(15, DiceAlgo::ALGO_MIN, &rolled);
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
		StdDice d100(1, 100, 122);
		std::vector<int> rolled;
		int result = d100.roll(15, DiceAlgo::ALGO_MAX, &rolled);
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
		StdDice d12(12, 123);
		std::vector<int> rolled;
		int result = d12.roll(9, DiceAlgo::ALGO_MEAN_ROUND, &rolled);
		int expected = mean_round(rolled);

		if (static_cast<int>(rolled.size()) == 9 && result == expected)
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

static void test_same_seed_same_sequence()
{
	const std::string test_name = "same_seed_same_sequence";
	try
	{
		StdDice d1(6, std::optional<unsigned int>(500));
		StdDice d2(6, std::optional<unsigned int>(500));

		bool equal = true;
		for (int i = 0; i < 40; ++i)
		{
			if (d1.roll_one() != d2.roll_one())
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
		StdDice d1(-1, 1, 600);
		StdDice d2(-1, 1, 600);

		for (int i = 0; i < 12; ++i)
		{
			(void)d1.roll_one();
			(void)d2.roll_one();
		}

		d1.reseed(777);
		d2.reseed(777);

		bool equal = true;
		for (int i = 0; i < 30; ++i)
		{
			if (d1.roll_one() != d2.roll_one())
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

static void test_roll_ref_overload()
{
	const std::string test_name = "roll_ref_overload";
	try
	{
		StdDice d12(12, std::optional<unsigned int>(600));
		std::vector<int> rolled;
		int result = d12.roll(6, DiceAlgo::ALGO_MAX, rolled);
		int expected = *std::max_element(rolled.begin(), rolled.end());

		if (static_cast<int>(rolled.size()) == 6 && result == expected)
		{
			pass(test_name);
		}
		else
		{
			fail(test_name, "ref overload: max mismatch or wrong size");
		}
	}
	catch (...)
	{
		fail(test_name, "unexpected exception");
	}
}

int main()
{
	std::cout << "=== StdDice unit tests ===\n\n";

	test_default_constructor_is_d6();
	test_single_param_constructor_builds_1_to_max();
	test_range_constructor_builds_min_to_max();
	test_single_param_invalid_max_throws();
	test_range_invalid_bounds_throws();
	test_roll_one_within_range();
	test_roll_sum_matches_rolled_out();
	test_roll_min_matches_rolled_out();
	test_roll_max_matches_rolled_out();
	test_roll_mean_round_matches_rolled_out();
	test_same_seed_same_sequence();
	test_reseed_same_state_same_future_sequence();
	test_roll_ref_overload();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
