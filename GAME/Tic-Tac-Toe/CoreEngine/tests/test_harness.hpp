#ifndef GMTRIS_TEST_HARNESS_HPP
#define GMTRIS_TEST_HARNESS_HPP

/**
 * @file tests/test_harness.hpp
 * @brief Minimal header-only assertion harness shared by the Tris unit tests.
 *
 * Mirrors the lightweight pass/fail style already used by the gmAlea tests so
 * the Tic-Tac-Toe suite stays dependency-free (no GoogleTest). A test process
 * runs a sequence of checks and returns @ref gmtris_test::summary as its exit
 * code (0 = all passed, 1 = at least one failure).
 */

#include <iostream>
#include <string>

namespace gmtris_test
{

/// @brief Running count of passed checks for the current process.
inline int& pass_count()
{
	static int count = 0;
	return count;
}

/// @brief Running count of failed checks for the current process.
inline int& fail_count()
{
	static int count = 0;
	return count;
}

/// @brief Records a passing check named @p name.
inline void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++pass_count();
}

/// @brief Records a failing check named @p name with an explanatory @p reason.
inline void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " - " << reason << "\n";
	++fail_count();
}

/// @brief Records a pass when @p condition holds, otherwise a failure.
inline void check(const std::string& name, bool condition, const std::string& reason = "")
{
	if (condition)
	{
		pass(name);
	}
	else
	{
		fail(name, reason.empty() ? "condition was false" : reason);
	}
}

/// @brief Prints a summary line for @p suite and returns the process exit code.
inline int summary(const std::string& suite)
{
	std::cout << "\n"
	          << suite << ": " << pass_count() << " passed, " << fail_count()
	          << " failed\n";
	return fail_count() == 0 ? 0 : 1;
}

} // namespace gmtris_test

#endif // GMTRIS_TEST_HARNESS_HPP
