/**
 * @file tests/test_modifier_container.cpp
 * @brief Unit tests for apply_modifiers() evaluator (modifiers/Modifier.hpp).
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmActor/modifiers/Modifier.cpp ^
 *       gmActor/tests/test_modifier_container.cpp ^
 *       -o bin/exe/test_gmActor_modifier.exe
 */

#include "gmActor/modifiers/Modifier.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/core/Ids.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace gmActor;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " -- " << reason << "\n";
	++g_fail;
}

static bool near(double a, double b, double eps = 1e-9)
{
	return std::abs(a - b) < eps;
}

static ModifierInstance make_mod(const std::string& stat_key,
                                 ModifierOperation   op,
                                 double              value)
{
	ModifierInstance m;
	m.id        = "mod_" + stat_key;
	m.stat_key  = stat_key;
	m.operation = op;
	m.value     = value;
	return m;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_no_modifiers_returns_base()
{
	const std::string T = "no_modifiers_returns_base";
	std::vector<ModifierInstance> mods;
	double result = apply_modifiers(10.0, "hp_max", mods);
	if (!near(result, 10.0)) { fail(T, "expected 10.0"); return; }
	pass(T);
}

static void test_add_modifier()
{
	const std::string T = "add_modifier";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::ADD, 3.0)
	};
	double result = apply_modifiers(5.0, "dmg", mods);
	if (!near(result, 8.0)) { fail(T, "expected 5+3=8"); return; }
	pass(T);
}

static void test_subtract_modifier()
{
	const std::string T = "subtract_modifier";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::SUBTRACT, 2.0)
	};
	double result = apply_modifiers(10.0, "dmg", mods);
	if (!near(result, 8.0)) { fail(T, "expected 10-2=8"); return; }
	pass(T);
}

static void test_multiply_modifier()
{
	const std::string T = "multiply_modifier";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::MULTIPLY, 2.0)
	};
	double result = apply_modifiers(5.0, "dmg", mods);
	if (!near(result, 10.0)) { fail(T, "expected 5*2=10"); return; }
	pass(T);
}

static void test_set_modifier_overrides_base()
{
	const std::string T = "set_modifier_overrides_base";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::SET, 7.0)
	};
	double result = apply_modifiers(5.0, "dmg", mods);
	if (!near(result, 7.0)) { fail(T, "expected SET to 7.0"); return; }
	pass(T);
}

static void test_set_last_wins()
{
	// D13: last SET wins
	const std::string T = "set_last_wins";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::SET, 4.0),
		make_mod("dmg", ModifierOperation::SET, 9.0) // last SET wins
	};
	double result = apply_modifiers(5.0, "dmg", mods);
	if (!near(result, 9.0)) { fail(T, "expected last SET=9.0"); return; }
	pass(T);
}

static void test_evaluation_order_set_then_add_then_multiply()
{
	// D13: SET overrides base, then ADD, then MULTIPLY
	// base=10, SET=5, ADD=3, MULTIPLY=2  → (5+3)*2 = 16
	const std::string T = "evaluation_order";
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::MULTIPLY, 2.0),
		make_mod("dmg", ModifierOperation::ADD,      3.0),
		make_mod("dmg", ModifierOperation::SET,      5.0)
	};
	double result = apply_modifiers(10.0, "dmg", mods);
	if (!near(result, 16.0)) { fail(T, "expected (5+3)*2=16, got " + std::to_string(result)); return; }
	pass(T);
}

static void test_modifiers_for_other_stat_ignored()
{
	const std::string T = "other_stat_ignored";
	std::vector<ModifierInstance> mods = {
		make_mod("movement", ModifierOperation::ADD, 99.0)
	};
	double result = apply_modifiers(5.0, "dmg", mods);
	if (!near(result, 5.0)) { fail(T, "modifiers for other stat should be ignored"); return; }
	pass(T);
}

static void test_multiple_add_and_subtract()
{
	const std::string T = "multiple_add_subtract";
	// base=10, +3, -1, +2 → 14
	std::vector<ModifierInstance> mods = {
		make_mod("dmg", ModifierOperation::ADD,      3.0),
		make_mod("dmg", ModifierOperation::SUBTRACT, 1.0),
		make_mod("dmg", ModifierOperation::ADD,      2.0)
	};
	double result = apply_modifiers(10.0, "dmg", mods);
	if (!near(result, 14.0)) { fail(T, "expected 14"); return; }
	pass(T);
}

static void test_mixed_stat_keys()
{
	const std::string T = "mixed_stat_keys";
	// Only "hp_max" mods apply; "dmg" mod is ignored
	std::vector<ModifierInstance> mods = {
		make_mod("hp_max", ModifierOperation::ADD, 5.0),
		make_mod("dmg",    ModifierOperation::ADD, 99.0)
	};
	double hp_result  = apply_modifiers(20.0, "hp_max", mods);
	double dmg_result = apply_modifiers(3.0,  "dmg",    mods);

	if (!near(hp_result, 25.0))  { fail(T, "hp_max: expected 25"); return; }
	if (!near(dmg_result, 102.0)) { fail(T, "dmg: expected 3+99=102"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: apply_modifiers() tests ===\n\n";

	test_no_modifiers_returns_base();
	test_add_modifier();
	test_subtract_modifier();
	test_multiply_modifier();
	test_set_modifier_overrides_base();
	test_set_last_wins();
	test_evaluation_order_set_then_add_then_multiply();
	test_modifiers_for_other_stat_ignored();
	test_multiple_add_and_subtract();
	test_mixed_stat_keys();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
