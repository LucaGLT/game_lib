/**
 * @file tests/test_actor_common.cpp
 * @brief Unit tests for ActorStateCommon fields and basic mutation.
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
 *       gmActor/tests/test_actor_common.cpp ^
 *       -o bin/exe/test_gmActor_common.exe
 */

#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/core/Enums.hpp"
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

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_default_values()
{
	const std::string T = "default_values";
	ActorStateCommon c;
	if (c.kind != ActorKind::HERO)          { fail(T, "kind default"); return; }
	if (!c.enabled)                         { fail(T, "enabled default"); return; }
	if (c.removed)                          { fail(T, "removed default"); return; }
	if (!c.can_act)                         { fail(T, "can_act default"); return; }
	if (!c.can_be_targeted)                 { fail(T, "can_be_targeted default"); return; }
	if (c.life_state != ActorLifeState::ACTIVE) { fail(T, "life_state default"); return; }
	if (c.current_hp != 0)                  { fail(T, "current_hp default"); return; }
	if (c.max_hp != 0)                      { fail(T, "max_hp default"); return; }
	pass(T);
}

static void test_identity_fields()
{
	const std::string T = "identity_fields";
	ActorStateCommon c;
	c.actor_id    = "hero_1";
	c.display_name = "Aria";
	c.faction_id  = "heroes";
	c.kind        = ActorKind::HERO;

	if (c.actor_id != "hero_1")      { fail(T, "actor_id"); return; }
	if (c.display_name != "Aria")    { fail(T, "display_name"); return; }
	if (c.faction_id != "heroes")    { fail(T, "faction_id"); return; }
	if (c.kind != ActorKind::HERO)   { fail(T, "kind"); return; }
	pass(T);
}

static void test_set_location()
{
	const std::string T = "set_location";
	ActorStateCommon c;
	c.area_id      = "room_3";
	c.area_position = AreaPosition::FRONTLINE;

	if (c.area_id != "room_3")                     { fail(T, "area_id"); return; }
	if (c.area_position != AreaPosition::FRONTLINE) { fail(T, "area_position"); return; }
	pass(T);
}

static void test_participation_flags()
{
	const std::string T = "participation_flags";
	ActorStateCommon c;
	c.enabled         = false;
	c.removed         = true;
	c.can_act         = false;
	c.can_be_targeted = false;

	if (c.enabled)         { fail(T, "enabled should be false"); return; }
	if (!c.removed)        { fail(T, "removed should be true"); return; }
	if (c.can_act)         { fail(T, "can_act should be false"); return; }
	if (c.can_be_targeted) { fail(T, "can_be_targeted should be false"); return; }
	pass(T);
}

static void test_life_state()
{
	const std::string T = "life_state";
	ActorStateCommon c;
	c.life_state = ActorLifeState::KO;
	if (c.life_state != ActorLifeState::KO) { fail(T, "life_state KO"); return; }

	c.life_state = ActorLifeState::DEAD;
	if (c.life_state != ActorLifeState::DEAD) { fail(T, "life_state DEAD"); return; }
	pass(T);
}

static void test_timeline_fields()
{
	const std::string T = "timeline_fields";
	ActorStateCommon c;
	c.timeline_position = 3;
	c.tie_break_rank    = 1;

	if (c.timeline_position != 3) { fail(T, "timeline_position"); return; }
	if (c.tie_break_rank != 1)    { fail(T, "tie_break_rank"); return; }
	pass(T);
}

static void test_hp_fields()
{
	const std::string T = "hp_fields";
	ActorStateCommon c;
	c.max_hp     = 10;
	c.current_hp = 7;

	if (c.max_hp != 10)     { fail(T, "max_hp"); return; }
	if (c.current_hp != 7)  { fail(T, "current_hp"); return; }
	pass(T);
}

static void test_tags_add_and_query()
{
	const std::string T = "tags_add_and_query";
	ActorStateCommon c;
	c.tags.push_back("undead");
	c.tags.push_back("elite");

	if (c.tags.size() != 2)       { fail(T, "tag count"); return; }
	if (c.tags[0] != "undead")    { fail(T, "tag[0]"); return; }
	if (c.tags[1] != "elite")     { fail(T, "tag[1]"); return; }
	pass(T);
}

static void test_kind_enum_values()
{
	const std::string T = "kind_enum_all_values";
	ActorStateCommon c;

	c.kind = ActorKind::ALLY_NPC;
	if (c.kind != ActorKind::ALLY_NPC) { fail(T, "ALLY_NPC"); return; }

	c.kind = ActorKind::MONSTER_INSTANCE;
	if (c.kind != ActorKind::MONSTER_INSTANCE) { fail(T, "MONSTER_INSTANCE"); return; }

	c.kind = ActorKind::MONSTER_GROUP;
	if (c.kind != ActorKind::MONSTER_GROUP) { fail(T, "MONSTER_GROUP"); return; }

	c.kind = ActorKind::BOSS;
	if (c.kind != ActorKind::BOSS) { fail(T, "BOSS"); return; }

	c.kind = ActorKind::MISSION_SYSTEM;
	if (c.kind != ActorKind::MISSION_SYSTEM) { fail(T, "MISSION_SYSTEM"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: ActorStateCommon tests ===\n\n";

	test_default_values();
	test_identity_fields();
	test_set_location();
	test_participation_flags();
	test_life_state();
	test_timeline_fields();
	test_hp_fields();
	test_tags_add_and_query();
	test_kind_enum_values();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
