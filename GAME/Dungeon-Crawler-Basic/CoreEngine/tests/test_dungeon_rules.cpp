/**
 * @file tests/test_dungeon_rules.cpp
 * @brief Unit tests for DungeonRuleAdapter + ActionV1 v1 action validation.
 */

#include "actors/ActorRoster.hpp"
#include "actions/ActionV1.hpp"
#include "engine/DungeonTypes.hpp"
#include "rules/DungeonRuleAdapter.hpp"
#include "world/DungeonMap.hpp"

#include <cassert>
#include <iostream>

using gmDungeonBasic::ActorInfo;
using gmDungeonBasic::ActorRoster;
using gmDungeonBasic::ActionV1;
using gmDungeonBasic::DungeonActorKind;
using gmDungeonBasic::DungeonMap;
using gmDungeonBasic::DungeonRuleAdapter;

// ── Helper fixtures ───────────────────────────────────────────────────────────

struct Fixture
{
	DungeonMap         map;
	ActorRoster        roster;
	DungeonRuleAdapter rules{map, roster};
	ActionV1           actions{map, roster, rules};

	Fixture()
	{
		map.create_room("room_start");
		map.create_room("room_2");
		map.create_room("room_3");
		map.add_connection("room_start", "room_2");
		// room_3 is disconnected from room_start

		ActorInfo hero;
		hero.id = "hero";
		hero.kind = DungeonActorKind::HERO;
		hero.hp = hero.max_hp = 10;
		hero.location = "room_start";
		hero.tags = {"has_potion", "bigword_available"};
		roster.add_actor(hero);

		ActorInfo monster;
		monster.id = "m1";
		monster.kind = DungeonActorKind::MONSTER;
		monster.hp = monster.max_hp = 5;
		monster.location = "room_2";
		roster.add_actor(monster);
	}
};

// ── Move tests ────────────────────────────────────────────────────────────────

static void test_move_valid_adjacent()
{
	Fixture fx;
	assert(fx.rules.can_move("hero", "room_2"));
	assert(fx.actions.execute_move("hero", "room_2"));
	assert(fx.roster.get_actor("hero").location == "room_2");
	std::cout << "  [OK] test_move_valid_adjacent\n";
}

static void test_move_rejected_not_adjacent()
{
	Fixture fx;
	assert(!fx.rules.can_move("hero", "room_3"));
	assert(!fx.rules.rejection_reason().empty());
	assert(!fx.actions.execute_move("hero", "room_3"));
	std::cout << "  [OK] test_move_rejected_not_adjacent\n";
}

static void test_move_rejected_unknown_dest()
{
	Fixture fx;
	assert(!fx.rules.can_move("hero", "nonexistent_room"));
	assert(!fx.actions.execute_move("hero", "nonexistent_room"));
	std::cout << "  [OK] test_move_rejected_unknown_dest\n";
}

static void test_move_rejected_if_stunned()
{
	Fixture fx;
	fx.roster.add_status("hero", "stunned");
	assert(!fx.rules.can_move("hero", "room_2"));
	std::cout << "  [OK] test_move_rejected_if_stunned\n";
}

// ── Heal tests ────────────────────────────────────────────────────────────────

static void test_heal_self_with_potion()
{
	Fixture fx;
	fx.roster.set_hp("hero", 5);
	assert(fx.rules.can_heal("hero", "hero"));
	assert(fx.actions.execute_heal("hero", "hero"));
	// HP should have increased (by 3, capped at max_hp=10)
	const int hp = fx.roster.get_actor("hero").hp;
	assert(hp == 8);
	// has_potion tag removed
	assert(!fx.roster.has_tag("hero", "has_potion"));
	std::cout << "  [OK] test_heal_self_with_potion\n";
}

static void test_heal_rejected_no_potion()
{
	Fixture fx;
	fx.roster.remove_tag("hero", "has_potion");
	assert(!fx.rules.can_heal("hero", "hero"));
	assert(!fx.actions.execute_heal("hero", "hero"));
	std::cout << "  [OK] test_heal_rejected_no_potion\n";
}

// ── Equip tests ───────────────────────────────────────────────────────────────

static void test_equip_weapon()
{
	Fixture fx;
	assert(!fx.roster.has_tag("hero", "equipped_weapon"));
	assert(fx.rules.can_equip("hero", "bigword_available"));
	assert(fx.actions.execute_equip("hero", "bigword_available"));
	assert(fx.roster.has_tag("hero", "equipped_weapon"));
	assert(!fx.roster.has_tag("hero", "bigword_available"));
	std::cout << "  [OK] test_equip_weapon\n";
}

static void test_equip_rejected_no_item()
{
	Fixture fx;
	fx.roster.remove_tag("hero", "bigword_available");
	assert(!fx.rules.can_equip("hero", "bigword_available"));
	assert(!fx.actions.execute_equip("hero", "bigword_available"));
	std::cout << "  [OK] test_equip_rejected_no_item\n";
}

static void test_equip_rejected_already_equipped()
{
	Fixture fx;
	fx.roster.add_tag("hero", "equipped_weapon");
	assert(!fx.rules.can_equip("hero", "bigword_available"));
	assert(!fx.actions.execute_equip("hero", "bigword_available"));
	std::cout << "  [OK] test_equip_rejected_already_equipped\n";
}

int main()
{
	std::cout << "=== DungeonRules (RuleAdapter + ActionV1) unit tests ===\n";
	test_move_valid_adjacent();
	test_move_rejected_not_adjacent();
	test_move_rejected_unknown_dest();
	test_move_rejected_if_stunned();
	test_heal_self_with_potion();
	test_heal_rejected_no_potion();
	test_equip_weapon();
	test_equip_rejected_no_item();
	test_equip_rejected_already_equipped();
	std::cout << "All DungeonRules tests PASSED.\n";
	return 0;
}
