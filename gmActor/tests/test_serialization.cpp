/**
 * @file tests/test_serialization.cpp
 * @brief Unit tests for to_json / from_json round-trips (serialization/ActorJson.hpp).
 *
 * Build (from game_lib root — requires gmSave for json.hpp):
 *   clang++ -std=c++17 -I. ^
 *       gmActor/stats/Health.cpp ^
 *       gmActor/stats/StatBlock.cpp ^
 *       gmActor/modifiers/Modifier.cpp ^
 *       gmActor/statuses/StatusContainer.cpp ^
 *       gmActor/items/InventoryState.cpp ^
 *       gmActor/items/EquipmentState.cpp ^
 *       gmActor/actors/ActorStore.cpp ^
 *       gmActor/actors/ActorQueries.cpp ^
 *       gmActor/serialization/ActorJson.cpp ^
 *       gmActor/tests/test_serialization.cpp ^
 *       -o bin/exe/test_gmActor_serial.exe
 */

#include "gmActor/serialization/ActorJson.hpp"
#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmSave/json.hpp"

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static HeroState make_hero(const std::string& id)
{
	HeroState h;
	h.common.actor_id    = id;
	h.common.kind        = ActorKind::HERO;
	h.common.display_name = "Hero " + id;
	h.common.faction_id  = "heroes";
	h.common.max_hp      = 10;
	h.common.current_hp  = 8;
	h.common.life_state  = ActorLifeState::ACTIVE;
	h.common.area_id     = "room_1";
	h.common.area_position = AreaPosition::FRONTLINE;
	h.common.tags.push_back("ranger");
	h.level           = 2;
	h.hand_limit      = 5;
	h.memory_limit    = 3;
	h.total_deck_id   = "deck_" + id;
	h.mission_deck_id = "mdeck_" + id;
	h.inventory.add("sword_01");
	h.inventory.add("potion_01");
	h.equipment.equip(EquipmentSlot::MAIN_HAND, "sword_01");
	h.is_ko = false;
	return h;
}

static MonsterGroupState make_group(const std::string& id)
{
	MonsterGroupState g;
	g.actor_id         = id;
	g.group_id         = id;
	g.monster_type_id  = "goblin";
	g.display_name     = "Goblin Group";
	g.enabled          = true;
	g.removed          = false;
	g.timeline_position = 2;
	g.tie_break_rank   = 0;
	g.members.push_back("goblin_1");
	g.members.push_back("goblin_2");
	g.behavior_deck_id = "deck_goblin";
	g.tags.push_back("undead");
	return g;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_hero_round_trip()
{
	const std::string T = "hero_round_trip";
	HeroState original = make_hero("hero_1");

	nlohmann::json j;
	to_json(j, original);

	HeroState restored;
	from_json(j, restored);

	if (restored.common.actor_id != "hero_1")             { fail(T, "actor_id"); return; }
	if (restored.common.display_name != "Hero hero_1")    { fail(T, "display_name"); return; }
	if (restored.common.max_hp != 10)                     { fail(T, "max_hp"); return; }
	if (restored.common.current_hp != 8)                  { fail(T, "current_hp"); return; }
	if (restored.common.life_state != ActorLifeState::ACTIVE) { fail(T, "life_state"); return; }
	if (restored.common.area_id != "room_1")              { fail(T, "area_id"); return; }
	if (restored.common.area_position != AreaPosition::FRONTLINE) { fail(T, "area_position"); return; }
	if (restored.common.tags.empty())                     { fail(T, "tags empty"); return; }
	if (restored.level != 2)                              { fail(T, "level"); return; }
	if (restored.hand_limit != 5)                         { fail(T, "hand_limit"); return; }
	if (restored.total_deck_id != "deck_hero_1")          { fail(T, "total_deck_id"); return; }
	if (!restored.inventory.contains("sword_01"))         { fail(T, "inventory sword_01"); return; }
	if (!restored.inventory.contains("potion_01"))        { fail(T, "inventory potion_01"); return; }
	if (!restored.equipment.has_equipped(EquipmentSlot::MAIN_HAND)) { fail(T, "equipment slot"); return; }
	pass(T);
}

static void test_monster_group_round_trip()
{
	const std::string T = "monster_group_round_trip";
	MonsterGroupState original = make_group("grp_1");

	nlohmann::json j;
	to_json(j, original);

	MonsterGroupState restored;
	from_json(j, restored);

	if (restored.actor_id != "grp_1")          { fail(T, "actor_id"); return; }
	if (restored.monster_type_id != "goblin")  { fail(T, "monster_type_id"); return; }
	if (restored.display_name != "Goblin Group") { fail(T, "display_name"); return; }
	if (restored.timeline_position != 2)       { fail(T, "timeline_position"); return; }
	if (restored.members.size() != 2)          { fail(T, "members count"); return; }
	if (restored.behavior_deck_id != "deck_goblin") { fail(T, "behavior_deck_id"); return; }
	if (restored.tags.empty())                 { fail(T, "tags empty"); return; }
	pass(T);
}

static void test_actor_store_round_trip()
{
	const std::string T = "actor_store_round_trip";
	ActorStore original;
	original.add_hero(make_hero("hero_1"));
	original.add_hero(make_hero("hero_2"));
	original.add_monster_group(make_group("grp_goblins"));

	MissionSystemState sys;
	sys.actor_id    = "system_mission";
	sys.enabled     = true;
	original.set_mission_system(sys);

	nlohmann::json j;
	to_json(j, original);

	ActorStore restored;
	from_json(j, restored);

	if (!restored.has_actor("hero_1"))       { fail(T, "hero_1 missing"); return; }
	if (!restored.has_actor("hero_2"))       { fail(T, "hero_2 missing"); return; }
	if (!restored.has_actor("grp_goblins"))  { fail(T, "group missing"); return; }
	if (restored.actor_kind("hero_1") != ActorKind::HERO)
		{ fail(T, "hero kind"); return; }
	if (restored.actor_kind("grp_goblins") != ActorKind::MONSTER_GROUP)
		{ fail(T, "group kind"); return; }

	// Mission system
	if (!restored.mission_system_opt().has_value()) { fail(T, "mission system missing"); return; }
	if (restored.mission_system().actor_id != "system_mission") { fail(T, "system actor_id"); return; }
	pass(T);
}

static void test_store_round_trip_preserves_hp()
{
	const std::string T = "store_round_trip_hp";
	ActorStore original;
	HeroState h = make_hero("hero_1");
	h.common.current_hp = 3;
	h.common.max_hp     = 10;
	original.add_hero(h);

	nlohmann::json j;
	to_json(j, original);
	ActorStore restored;
	from_json(j, restored);

	const ActorStateCommon& c = restored.common("hero_1");
	if (c.current_hp != 3)  { fail(T, "current_hp not preserved"); return; }
	if (c.max_hp != 10)     { fail(T, "max_hp not preserved"); return; }
	pass(T);
}

static void test_modifier_instance_round_trip()
{
	const std::string T = "modifier_instance_round_trip";
	ModifierInstance m;
	m.id        = "mod_01";
	m.source_id = "item_ring";
	m.stat_key  = "base_damage";
	m.operation = ModifierOperation::ADD;
	m.value     = 2.5;
	m.duration_kind = ModifierDurationKind::MANUAL_REMOVE;
	m.expires_at_time = -1;

	nlohmann::json j;
	to_json(j, m);
	ModifierInstance r;
	from_json(j, r);

	if (r.id != "mod_01")                      { fail(T, "id"); return; }
	if (r.source_id != "item_ring")            { fail(T, "source_id"); return; }
	if (r.stat_key != "base_damage")           { fail(T, "stat_key"); return; }
	if (r.operation != ModifierOperation::ADD) { fail(T, "operation"); return; }
	if (r.value != 2.5)                        { fail(T, "value"); return; }
	pass(T);
}

static void test_status_instance_round_trip()
{
	const std::string T = "status_instance_round_trip";
	StatusInstance s;
	s.id          = "burn";
	s.source_id   = "trap_fire";
	s.stacks      = 3;
	s.duration_kind = ModifierDurationKind::UNTIL_TIME;
	s.expires_at_time = 5;

	nlohmann::json j;
	to_json(j, s);
	StatusInstance r;
	from_json(j, r);

	if (r.id != "burn")         { fail(T, "id"); return; }
	if (r.source_id != "trap_fire") { fail(T, "source_id"); return; }
	if (r.stacks != 3)          { fail(T, "stacks"); return; }
	if (r.expires_at_time != 5) { fail(T, "expires_at_time"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: Serialization round-trip tests ===\n\n";

	test_hero_round_trip();
	test_monster_group_round_trip();
	test_actor_store_round_trip();
	test_store_round_trip_preserves_hp();
	test_modifier_instance_round_trip();
	test_status_instance_round_trip();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
