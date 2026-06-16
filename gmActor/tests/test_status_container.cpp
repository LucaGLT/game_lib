/**
 * @file tests/test_status_container.cpp
 * @brief Unit tests for StatusContainer (statuses/StatusContainer.hpp).
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
 *       gmActor/tests/test_status_container.cpp ^
 *       -o bin/exe/test_gmActor_status.exe
 */

#include "gmActor/statuses/StatusContainer.hpp"
#include "gmActor/statuses/StatusInstance.hpp"
#include "gmActor/core/Ids.hpp"

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

static StatusInstance make_status(const StatusId& id, int stacks = 1)
{
	StatusInstance s;
	s.id     = id;
	s.stacks = stacks;
	return s;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_empty_container()
{
	const std::string T = "empty_container";
	StatusContainer sc;
	if (!sc.all().empty()) { fail(T, "should start empty"); return; }
	if (sc.has("burn"))    { fail(T, "has() on empty"); return; }
	pass(T);
}

static void test_add_non_stackable_status()
{
	const std::string T = "add_non_stackable";
	StatusContainer sc;
	sc.add(make_status("burn"), /*stackable=*/false);

	if (!sc.has("burn"))          { fail(T, "has() after add"); return; }
	if (sc.all().size() != 1)     { fail(T, "size should be 1"); return; }
	pass(T);
}

static void test_add_non_stackable_replaces_existing()
{
	// D11: non-stackable re-add replaces the instance; stacks stays 1
	const std::string T = "non_stackable_replace";
	StatusContainer sc;
	sc.add(make_status("burn", 1), false);
	sc.add(make_status("burn", 3), false); // re-add

	if (sc.all().size() != 1)          { fail(T, "should still have 1 entry"); return; }
	auto opt = sc.get("burn");
	if (!opt.has_value())              { fail(T, "get() returned nullopt"); return; }
	if (opt->stacks != 1)              { fail(T, "stacks should be reset to 1 after replace"); return; }
	pass(T);
}

static void test_add_stackable_increments_stacks()
{
	// D12: stackable re-add increments stacks
	const std::string T = "stackable_increments";
	StatusContainer sc;
	sc.add(make_status("poison", 1), true);
	sc.add(make_status("poison", 2), true); // re-add

	auto opt = sc.get("poison");
	if (!opt.has_value())        { fail(T, "get() returned nullopt"); return; }
	if (opt->stacks != 3)        { fail(T, "stacks should be 1+2=3"); return; }
	pass(T);
}

static void test_add_multiple_different_statuses()
{
	const std::string T = "add_multiple_different";
	StatusContainer sc;
	sc.add(make_status("burn"),   false);
	sc.add(make_status("stun"),   false);
	sc.add(make_status("poison"), true);

	if (sc.all().size() != 3) { fail(T, "expected 3 statuses"); return; }
	if (!sc.has("burn"))      { fail(T, "missing burn"); return; }
	if (!sc.has("stun"))      { fail(T, "missing stun"); return; }
	if (!sc.has("poison"))    { fail(T, "missing poison"); return; }
	pass(T);
}

static void test_remove_existing_status()
{
	const std::string T = "remove_existing";
	StatusContainer sc;
	sc.add(make_status("burn"), false);
	sc.add(make_status("stun"), false);
	sc.remove("burn");

	if (sc.has("burn"))       { fail(T, "burn should be gone"); return; }
	if (!sc.has("stun"))      { fail(T, "stun should remain"); return; }
	if (sc.all().size() != 1) { fail(T, "size should be 1"); return; }
	pass(T);
}

static void test_remove_nonexistent_is_noop()
{
	const std::string T = "remove_nonexistent_noop";
	StatusContainer sc;
	sc.add(make_status("burn"), false);
	sc.remove("stun"); // doesn't exist

	if (sc.all().size() != 1) { fail(T, "size should still be 1"); return; }
	pass(T);
}

static void test_clear()
{
	const std::string T = "clear";
	StatusContainer sc;
	sc.add(make_status("burn"),   false);
	sc.add(make_status("poison"), true);
	sc.clear();

	if (!sc.all().empty()) { fail(T, "should be empty after clear"); return; }
	if (sc.has("burn"))    { fail(T, "burn should be gone after clear"); return; }
	pass(T);
}

static void test_get_returns_correct_instance()
{
	const std::string T = "get_returns_correct";
	StatusContainer sc;
	StatusInstance s = make_status("burn", 2);
	s.source_id = "item_torch";
	sc.add(s, false);

	auto opt = sc.get("burn");
	if (!opt.has_value())           { fail(T, "expected value"); return; }
	if (opt->id != "burn")          { fail(T, "wrong id"); return; }
	if (opt->stacks != 2)           { fail(T, "wrong stacks"); return; }
	if (opt->source_id != "item_torch") { fail(T, "wrong source_id"); return; }
	pass(T);
}

static void test_get_returns_nullopt_when_missing()
{
	const std::string T = "get_nullopt_when_missing";
	StatusContainer sc;
	auto opt = sc.get("notexist");
	if (opt.has_value()) { fail(T, "expected nullopt"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: StatusContainer tests ===\n\n";

	test_empty_container();
	test_add_non_stackable_status();
	test_add_non_stackable_replaces_existing();
	test_add_stackable_increments_stacks();
	test_add_multiple_different_statuses();
	test_remove_existing_status();
	test_remove_nonexistent_is_noop();
	test_clear();
	test_get_returns_correct_instance();
	test_get_returns_nullopt_when_missing();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
