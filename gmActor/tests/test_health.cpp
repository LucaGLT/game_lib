/**
 * @file tests/test_health.cpp
 * @brief Unit tests for Health helper functions (stats/Health.hpp).
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmActor/stats/Health.cpp ^
 *       gmActor/stats/StatBlock.cpp ^
 *       gmActor/modifiers/Modifier.cpp ^
 *       gmActor/statuses/StatusContainer.cpp ^
 *       gmActor/items/InventoryState.cpp ^
 *       gmActor/items/EquipmentState.cpp ^
 *       gmActor/actors/ActorStore.cpp ^
 *       gmActor/actors/ActorQueries.cpp ^
 *       gmActor/tests/test_health.cpp ^
 *       -o bin/exe/test_gmActor_health.exe
 */

#include "gmActor/stats/Health.hpp"
#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/core/Enums.hpp"

#include <iostream>
#include <string>

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

static ActorStateCommon make_actor(int current, int max)
{
	ActorStateCommon c;
	c.max_hp     = max;
	c.current_hp = current;
	return c;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_has_health_true()
{
	const std::string T = "has_health_true";
	ActorStateCommon c = make_actor(5, 10);
	if (!has_health(c)) { fail(T, "expected true"); return; }
	pass(T);
}

static void test_has_health_false_when_max_zero()
{
	const std::string T = "has_health_false_max_zero";
	ActorStateCommon c = make_actor(0, 0);
	if (has_health(c)) { fail(T, "expected false for max_hp=0"); return; }
	pass(T);
}

static void test_missing_hp()
{
	const std::string T = "missing_hp";
	ActorStateCommon c = make_actor(3, 10);
	if (missing_hp(c) != 7) { fail(T, "expected 7"); return; }
	pass(T);
}

static void test_missing_hp_full()
{
	const std::string T = "missing_hp_full";
	ActorStateCommon c = make_actor(10, 10);
	if (missing_hp(c) != 0) { fail(T, "expected 0 when full"); return; }
	pass(T);
}

static void test_is_alive()
{
	const std::string T = "is_alive";
	ActorStateCommon c;
	c.life_state = ActorLifeState::ACTIVE;
	if (!is_alive(c)) { fail(T, "ACTIVE should be alive"); return; }

	c.life_state = ActorLifeState::KO;
	if (is_alive(c)) { fail(T, "KO should not be alive"); return; }
	pass(T);
}

static void test_is_ko()
{
	const std::string T = "is_ko";
	ActorStateCommon c;
	c.life_state = ActorLifeState::KO;
	if (!is_ko(c)) { fail(T, "expected KO"); return; }

	c.life_state = ActorLifeState::ACTIVE;
	if (is_ko(c)) { fail(T, "ACTIVE should not be KO"); return; }
	pass(T);
}

static void test_set_hp_clamps_at_max()
{
	const std::string T = "set_hp_clamps_at_max";
	ActorStateCommon c = make_actor(0, 10);
	set_hp(c, 15);
	if (c.current_hp != 10) { fail(T, "expected clamp to max_hp=10"); return; }
	pass(T);
}

static void test_set_hp_clamps_at_zero()
{
	const std::string T = "set_hp_clamps_at_zero";
	ActorStateCommon c = make_actor(5, 10);
	set_hp(c, -3);
	if (c.current_hp != 0) { fail(T, "expected clamp to 0"); return; }
	pass(T);
}

static void test_damage_reduces_hp()
{
	const std::string T = "damage_reduces_hp";
	ActorStateCommon c = make_actor(10, 10);
	damage_hp(c, 4);
	if (c.current_hp != 6) { fail(T, "expected 6"); return; }
	pass(T);
}

static void test_damage_clamps_at_zero()
{
	const std::string T = "damage_clamps_at_zero";
	ActorStateCommon c = make_actor(3, 10);
	damage_hp(c, 100);
	if (c.current_hp != 0) { fail(T, "expected clamp to 0"); return; }
	pass(T);
}

static void test_damage_negative_is_noop()
{
	const std::string T = "damage_negative_is_noop";
	ActorStateCommon c = make_actor(8, 10);
	damage_hp(c, -5);
	if (c.current_hp != 8) { fail(T, "negative damage should be no-op"); return; }
	pass(T);
}

static void test_damage_zero_is_noop()
{
	const std::string T = "damage_zero_is_noop";
	ActorStateCommon c = make_actor(8, 10);
	damage_hp(c, 0);
	if (c.current_hp != 8) { fail(T, "zero damage should be no-op"); return; }
	pass(T);
}

static void test_heal_increases_hp()
{
	const std::string T = "heal_increases_hp";
	ActorStateCommon c = make_actor(5, 10);
	heal_hp(c, 3);
	if (c.current_hp != 8) { fail(T, "expected 8"); return; }
	pass(T);
}

static void test_heal_clamps_at_max()
{
	const std::string T = "heal_clamps_at_max";
	ActorStateCommon c = make_actor(8, 10);
	heal_hp(c, 100);
	if (c.current_hp != 10) { fail(T, "expected clamp to max_hp=10"); return; }
	pass(T);
}

static void test_heal_negative_is_noop()
{
	const std::string T = "heal_negative_is_noop";
	ActorStateCommon c = make_actor(5, 10);
	heal_hp(c, -3);
	if (c.current_hp != 5) { fail(T, "negative heal should be no-op"); return; }
	pass(T);
}

static void test_max_hp_zero_actor_unchanged_by_damage()
{
	const std::string T = "max_hp_zero_unchanged_by_damage";
	ActorStateCommon c = make_actor(0, 0);
	damage_hp(c, 5);
	if (c.current_hp != 0) { fail(T, "expected 0"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: Health helpers tests ===\n\n";

	test_has_health_true();
	test_has_health_false_when_max_zero();
	test_missing_hp();
	test_missing_hp_full();
	test_is_alive();
	test_is_ko();
	test_set_hp_clamps_at_max();
	test_set_hp_clamps_at_zero();
	test_damage_reduces_hp();
	test_damage_clamps_at_zero();
	test_damage_negative_is_noop();
	test_damage_zero_is_noop();
	test_heal_increases_hp();
	test_heal_clamps_at_max();
	test_heal_negative_is_noop();
	test_max_hp_zero_actor_unchanged_by_damage();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
