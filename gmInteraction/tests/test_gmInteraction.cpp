/**
 * @file tests/test_gmInteraction.cpp
 * @brief Unit tests for the gmInteraction library (store + bridge + snapshot).
 */

#include "gmInteraction.hpp"
#include "bridges/MapInteractionBridge.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

namespace
{

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const char* what)
{
	if (condition)
	{
		++g_passed;
	}
	else
	{
		++g_failed;
		std::printf("  [FAIL] %s\n", what);
	}
}

template <typename Fn>
void expect_throw(Fn fn, const char* what)
{
	bool threw = false;
	try
	{
		fn();
	}
	catch (const gmInteraction::EInteractionError&)
	{
		threw = true;
	}
	check(threw, what);
}

void test_create_and_query()
{
	gmInteraction::InteractableObjectStore store;
	store.create(1, "chest");
	store.create(2, "lever", gmInteraction::InteractionState::LOCKED);

	check(store.count() == 2, "T01 count after create");
	check(store.has(1), "T02 has created id");
	check(!store.has(99), "T03 missing id absent");
	check(store.type_of(1) == "chest", "T04 type_of");
	check(store.state_of(2) == gmInteraction::InteractionState::LOCKED, "T05 initial state");

	expect_throw([&]() { store.create(1, "dup"); }, "T06 duplicate create throws");
	expect_throw([&]() { store.get(99); }, "T07 unknown get throws");
}

void test_state_and_meta()
{
	gmInteraction::InteractableObjectStore store;
	store.create(10, "door");

	store.set_state(10, gmInteraction::InteractionState::USED);
	check(store.state_of(10) == gmInteraction::InteractionState::USED, "T08 set_state");

	store.set_meta(10, "color", "red");
	check(store.has_meta(10, "color"), "T09 has_meta");
	check(store.get_meta(10, "color") == "red", "T10 get_meta");
	store.remove_meta(10, "color");
	check(!store.has_meta(10, "color"), "T11 remove_meta");

	expect_throw([&]() { store.get_meta(10, "missing"); }, "T12 unknown meta key throws");
	expect_throw([&]() { store.set_state(77, gmInteraction::InteractionState::IDLE); },
		"T13 set_state unknown throws");
}

void test_remove_and_clear()
{
	gmInteraction::InteractableObjectStore store;
	store.create(1, "a");
	store.create(2, "b");
	store.remove(1);
	check(!store.has(1), "T14 remove drops id");
	check(store.count() == 1, "T15 count after remove");
	expect_throw([&]() { store.remove(1); }, "T16 remove unknown throws");

	store.clear();
	check(store.count() == 0, "T17 clear empties store");
}

void test_state_string_roundtrip()
{
	using gmInteraction::InteractionState;
	const InteractionState states[] = {
		InteractionState::IDLE, InteractionState::ACTIVE, InteractionState::USED,
		InteractionState::LOCKED, InteractionState::DISABLED};
	bool ok = true;
	for (InteractionState s : states)
	{
		const std::string text = gmInteraction::interaction_state_to_string(s);
		ok = ok && (gmInteraction::interaction_state_from_string(text) == s);
	}
	check(ok, "T18 state string round-trip");
	expect_throw([]() { gmInteraction::interaction_state_from_string("NOPE"); },
		"T19 bad state string throws");
}

void test_bridge_with_gmmap()
{
	gmInteraction::InteractableObjectStore store;
	gmMap::gmMap<std::string> map;
	map.create_location(100);
	map.create_location(101);

	gmInteraction::spawn_object(store, map, 100, 5000ULL, "chest");
	gmInteraction::spawn_object(store, map, 100, 5001ULL, "lever",
		gmInteraction::InteractionState::LOCKED);
	gmInteraction::spawn_object(store, map, 101, 5002ULL, "door");

	const std::vector<gmInteraction::InteractableObject> at100 =
		gmInteraction::objects_at(store, map, 100);
	check(at100.size() == 2, "T20 bridge objects_at count");
	check(map.has_interactable(100, 5000ULL), "T21 bridge placed on map");
	check(store.has(5000ULL), "T22 bridge created in store");

	gmInteraction::despawn_object(store, map, 100, 5000ULL);
	check(!map.has_interactable(100, 5000ULL), "T23 despawn removes from map");
	check(!store.has(5000ULL), "T24 despawn removes from store");
	check(gmInteraction::objects_at(store, map, 100).size() == 1, "T25 objects_at after despawn");
}

void test_snapshot_roundtrip()
{
	const std::string path = "test_gmInteraction_snapshot.json";

	gmInteraction::InteractableObjectStore store;
	store.create(1, "chest", gmInteraction::InteractionState::IDLE);
	store.create(2, "lever", gmInteraction::InteractionState::LOCKED);
	store.set_meta(1, "loot", "gold");
	store.export_snapshot_json(path);

	gmInteraction::InteractableObjectStore reloaded;
	reloaded.import_snapshot_json(path);

	check(reloaded.count() == 2, "T26 snapshot count");
	check(reloaded.type_of(1) == "chest", "T27 snapshot type");
	check(reloaded.state_of(2) == gmInteraction::InteractionState::LOCKED, "T28 snapshot state");
	check(reloaded.has_meta(1, "loot") && reloaded.get_meta(1, "loot") == "gold",
		"T29 snapshot meta");

	std::remove(path.c_str());
}

} // namespace

int main()
{
	std::printf("=== gmInteraction tests ===\n");
	test_create_and_query();
	test_state_and_meta();
	test_remove_and_clear();
	test_state_string_roundtrip();
	test_bridge_with_gmmap();
	test_snapshot_roundtrip();

	std::printf("=== gmInteraction summary: %d/%d passed ===\n",
		g_passed, g_passed + g_failed);
	return g_failed == 0 ? 0 : 1;
}
