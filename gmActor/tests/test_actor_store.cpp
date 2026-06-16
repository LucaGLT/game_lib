/**
 * @file tests/test_actor_store.cpp
 * @brief Unit tests for ActorStore registration, accessors, and queries.
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
 *       gmActor/tests/test_actor_store.cpp ^
 *       -o bin/exe/test_gmActor_store.exe
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/ActorQueries.hpp"
#include "gmActor/core/Errors.hpp"
#include "gmActor/core/Enums.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static HeroState make_hero(const std::string& id, const std::string& area = "")
{
	HeroState h;
	h.common.actor_id    = id;
	h.common.kind        = ActorKind::HERO;
	h.common.display_name = id;
	h.common.area_id     = area;
	h.common.life_state  = ActorLifeState::ACTIVE;
	h.common.can_act     = true;
	h.common.can_be_targeted = true;
	return h;
}

static AllyState make_ally(const std::string& id, bool can_act = true)
{
	AllyState a;
	a.common.actor_id = id;
	a.common.kind     = ActorKind::ALLY_NPC;
	a.common.can_act  = can_act;
	return a;
}

static MonsterInstanceState make_monster(const std::string& id, const std::string& area = "")
{
	MonsterInstanceState m;
	m.common.actor_id  = id;
	m.common.kind      = ActorKind::MONSTER_INSTANCE;
	m.common.area_id   = area;
	m.common.life_state = ActorLifeState::ACTIVE;
	m.common.can_be_targeted = true;
	return m;
}

static MonsterGroupState make_group(const std::string& id, bool enabled = true)
{
	MonsterGroupState g;
	g.actor_id  = id;
	g.group_id  = id;
	g.enabled   = enabled;
	g.removed   = false;
	return g;
}

static bool contains(const std::vector<ActorId>& v, const ActorId& id)
{
	return std::find(v.begin(), v.end(), id) != v.end();
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_add_hero()
{
	const std::string T = "add_hero";
	ActorStore store;
	store.add_hero(make_hero("hero_1"));

	if (!store.has_actor("hero_1"))           { fail(T, "has_actor"); return; }
	if (store.actor_kind("hero_1") != ActorKind::HERO) { fail(T, "kind"); return; }
	if (store.hero("hero_1").common.actor_id != "hero_1") { fail(T, "accessor"); return; }
	pass(T);
}

static void test_add_monster_instance()
{
	const std::string T = "add_monster_instance";
	ActorStore store;
	store.add_monster_instance(make_monster("goblin_1"));

	if (!store.has_actor("goblin_1")) { fail(T, "has_actor"); return; }
	if (store.actor_kind("goblin_1") != ActorKind::MONSTER_INSTANCE) { fail(T, "kind"); return; }
	pass(T);
}

static void test_add_monster_group()
{
	const std::string T = "add_monster_group";
	ActorStore store;
	store.add_monster_group(make_group("goblin_group"));

	if (!store.has_actor("goblin_group")) { fail(T, "has_actor"); return; }
	if (store.actor_kind("goblin_group") != ActorKind::MONSTER_GROUP) { fail(T, "kind"); return; }
	pass(T);
}

static void test_duplicate_actor_throws()
{
	const std::string T = "duplicate_actor_throws";
	ActorStore store;
	store.add_hero(make_hero("hero_1"));
	try
	{
		store.add_hero(make_hero("hero_1"));
		fail(T, "expected DuplicateActorError");
	}
	catch (const DuplicateActorError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

static void test_unknown_actor_throws()
{
	const std::string T = "unknown_actor_throws";
	ActorStore store;
	try
	{
		store.actor_kind("not_here");
		fail(T, "expected UnknownActorError");
	}
	catch (const UnknownActorError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

static void test_common_on_group_throws()
{
	// D1: common() on MONSTER_GROUP throws InvalidActorKindError
	const std::string T = "common_on_group_throws";
	ActorStore store;
	store.add_monster_group(make_group("grp_1"));
	try
	{
		store.common("grp_1");
		fail(T, "expected InvalidActorKindError");
	}
	catch (const InvalidActorKindError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

static void test_timeline_includes_heroes()
{
	const std::string T = "timeline_includes_heroes";
	ActorStore store;
	store.add_hero(make_hero("hero_1"));
	store.add_hero(make_hero("hero_2"));

	auto ids = store.timeline_actor_ids();
	if (!contains(ids, "hero_1")) { fail(T, "missing hero_1"); return; }
	if (!contains(ids, "hero_2")) { fail(T, "missing hero_2"); return; }
	pass(T);
}

static void test_timeline_includes_enabled_groups()
{
	const std::string T = "timeline_includes_enabled_groups";
	ActorStore store;
	store.add_monster_group(make_group("grp_enabled", true));
	store.add_monster_group(make_group("grp_disabled", false));

	auto ids = store.timeline_actor_ids();
	if (!contains(ids, "grp_enabled"))  { fail(T, "missing enabled group"); return; }
	if (contains(ids, "grp_disabled"))  { fail(T, "disabled group should be excluded"); return; }
	pass(T);
}

static void test_timeline_ally_only_if_can_act()
{
	const std::string T = "timeline_ally_can_act";
	ActorStore store;
	store.add_ally(make_ally("ally_active",   true));
	store.add_ally(make_ally("ally_inactive", false));

	auto ids = store.timeline_actor_ids();
	if (!contains(ids, "ally_active"))   { fail(T, "active ally missing"); return; }
	if (contains(ids, "ally_inactive"))  { fail(T, "inactive ally should be excluded"); return; }
	pass(T);
}

static void test_timeline_mission_system_included()
{
	const std::string T = "timeline_mission_system";
	ActorStore store;
	MissionSystemState sys;
	sys.actor_id = "system_mission";
	sys.enabled  = true;
	store.set_mission_system(sys);

	auto ids = store.timeline_actor_ids();
	if (!contains(ids, "system_mission")) { fail(T, "mission system should be on timeline"); return; }
	pass(T);
}

static void test_actors_in_area()
{
	const std::string T = "actors_in_area";
	ActorStore store;
	store.add_hero(make_hero("hero_1", "room_1"));
	store.add_hero(make_hero("hero_2", "room_2"));
	store.add_monster_instance(make_monster("goblin_1", "room_1"));

	auto in_room1 = store.actors_in_area("room_1");
	if (!contains(in_room1, "hero_1"))    { fail(T, "hero_1 missing from room_1"); return; }
	if (!contains(in_room1, "goblin_1"))  { fail(T, "goblin_1 missing from room_1"); return; }
	if (contains(in_room1, "hero_2"))     { fail(T, "hero_2 should not be in room_1"); return; }
	pass(T);
}

static void test_targetable_in_area_excludes_ko()
{
	const std::string T = "targetable_excludes_ko";
	ActorStore store;
	HeroState h1 = make_hero("hero_1", "room_1");
	HeroState h2 = make_hero("hero_2", "room_1");
	h2.common.life_state = ActorLifeState::KO;

	store.add_hero(h1);
	store.add_hero(h2);

	auto targetable = store.targetable_actors_in_area("room_1");
	if (!contains(targetable, "hero_1")) { fail(T, "hero_1 should be targetable"); return; }
	if (contains(targetable, "hero_2"))  { fail(T, "KO hero_2 should not be targetable"); return; }
	pass(T);
}

static void test_actor_queries_is_hero()
{
	const std::string T = "queries_is_hero";
	ActorStore store;
	store.add_hero(make_hero("hero_1"));
	store.add_monster_group(make_group("grp_1"));

	if (!is_hero(store, "hero_1"))  { fail(T, "should be hero"); return; }
	if (is_hero(store, "grp_1"))    { fail(T, "group should not be hero"); return; }
	pass(T);
}

static void test_actor_queries_living_heroes()
{
	const std::string T = "queries_living_heroes";
	ActorStore store;
	HeroState h_alive = make_hero("hero_alive");
	HeroState h_dead  = make_hero("hero_dead");
	h_dead.common.life_state = ActorLifeState::DEAD;

	store.add_hero(h_alive);
	store.add_hero(h_dead);

	auto living = living_heroes(store);
	if (!contains(living, "hero_alive")) { fail(T, "alive hero missing"); return; }
	if (contains(living, "hero_dead"))   { fail(T, "dead hero should not appear"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: ActorStore tests ===\n\n";

	test_add_hero();
	test_add_monster_instance();
	test_add_monster_group();
	test_duplicate_actor_throws();
	test_unknown_actor_throws();
	test_common_on_group_throws();
	test_timeline_includes_heroes();
	test_timeline_includes_enabled_groups();
	test_timeline_ally_only_if_can_act();
	test_timeline_mission_system_included();
	test_actors_in_area();
	test_targetable_in_area_excludes_ko();
	test_actor_queries_is_hero();
	test_actor_queries_living_heroes();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
