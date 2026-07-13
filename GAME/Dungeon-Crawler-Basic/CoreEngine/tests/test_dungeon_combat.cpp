/**
 * @file tests/test_dungeon_combat.cpp
 * @brief Unit tests for Phase 4 combat: attack declaration + reactive defense.
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
using gmDungeonBasic::DefenseChoice;
using gmDungeonBasic::DungeonActorKind;
using gmDungeonBasic::DungeonMap;
using gmDungeonBasic::DungeonRuleAdapter;

// ── Helper fixture ────────────────────────────────────────────────────────────

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
		map.create_room("room_far");
		map.add_connection("room_start", "room_2");
		// room_far is disconnected from room_start.

		ActorInfo hero;
		hero.id = "hero";
		hero.kind = DungeonActorKind::HERO;
		hero.hp = hero.max_hp = 10;
		hero.attack = 4;
		hero.location = "room_start";
		roster.add_actor(hero);

		ActorInfo monster;
		monster.id = "m1";
		monster.kind = DungeonActorKind::MONSTER;
		monster.hp = monster.max_hp = 10;
		monster.attack = 3;
		monster.location = "room_2";
		roster.add_actor(monster);
	}
};

// ── declare_attack validation ─────────────────────────────────────────────────

static void test_declare_attack_valid()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base));
	assert(base == 4); // attack stat only.
	std::cout << "  [OK] test_declare_attack_valid\n";
}

static void test_declare_attack_with_card_modifier()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 2, base));
	assert(base == 6); // 4 (stat) + 2 (card).
	std::cout << "  [OK] test_declare_attack_with_card_modifier\n";
}

static void test_declare_attack_rejected_not_enemy()
{
	Fixture fx;
	int base = 0;
	assert(!fx.actions.declare_attack("hero", "hero", 0, base));
	assert(!fx.actions.last_rejection_reason().empty());
	std::cout << "  [OK] test_declare_attack_rejected_not_enemy\n";
}

static void test_declare_attack_rejected_out_of_reach()
{
	Fixture fx;
	fx.roster.move_to("m1", "room_far");
	int base = 0;
	assert(!fx.actions.declare_attack("hero", "m1", 0, base));
	std::cout << "  [OK] test_declare_attack_rejected_out_of_reach\n";
}

// ── resolve_attack outcomes ───────────────────────────────────────────────────

static void test_defense_reduces_damage()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base)); // base = 4
	DefenseChoice choice;
	choice.block = 2;
	int hp_after = 0;
	const int dealt = fx.actions.resolve_attack("m1", base, choice, hp_after);
	assert(dealt == 2);      // 4 - 2.
	assert(hp_after == 8);   // 10 - 2.
	std::cout << "  [OK] test_defense_reduces_damage\n";
}

static void test_defense_cancels_attack()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base));
	DefenseChoice choice;
	choice.cancel = true;
	int hp_after = 0;
	const int dealt = fx.actions.resolve_attack("m1", base, choice, hp_after);
	assert(dealt == 0);
	assert(hp_after == 10);  // unchanged.
	std::cout << "  [OK] test_defense_cancels_attack\n";
}

static void test_defense_pass_takes_full_damage()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base)); // base = 4
	DefenseChoice choice;
	choice.pass = true;
	int hp_after = 0;
	const int dealt = fx.actions.resolve_attack("m1", base, choice, hp_after);
	assert(dealt == 4);
	assert(hp_after == 6);
	std::cout << "  [OK] test_defense_pass_takes_full_damage\n";
}

static void test_defense_never_below_zero()
{
	Fixture fx;
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base)); // base = 4
	DefenseChoice choice;
	choice.block = 10; // over-block.
	int hp_after = 0;
	const int dealt = fx.actions.resolve_attack("m1", base, choice, hp_after);
	assert(dealt == 0);
	assert(hp_after == 10);
	std::cout << "  [OK] test_defense_never_below_zero\n";
}

static void test_difeso_status_reduces_and_is_consumed()
{
	Fixture fx;
	fx.roster.add_status("m1", "difeso");
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base)); // base = 4
	DefenseChoice choice;
	int hp_after = 0;
	const int dealt = fx.actions.resolve_attack("m1", base, choice, hp_after);
	assert(dealt == 3);  // 4 - 1 (difeso).
	assert(!fx.roster.has_status("m1", "difeso")); // consumed.
	std::cout << "  [OK] test_difeso_status_reduces_and_is_consumed\n";
}

static void test_shield_charges_decrement_then_deplete()
{
	Fixture fx;
	fx.roster.add_tag("m1", "scudo_equipaggiato");

	// First block: -1 from shield, one charge spent (2 -> 1).
	int base = 0;
	assert(fx.actions.declare_attack("hero", "m1", 0, base));
	DefenseChoice c1;
	int hp1 = 0;
	assert(fx.actions.resolve_attack("m1", base, c1, hp1) == 3);
	assert(fx.roster.has_tag("m1", "scudo_equipaggiato")); // still equipped.

	// Second block: -1 from shield, last charge spent (1 -> 0), tag removed.
	assert(fx.actions.declare_attack("hero", "m1", 0, base));
	DefenseChoice c2;
	int hp2 = 0;
	assert(fx.actions.resolve_attack("m1", base, c2, hp2) == 3);
	assert(!fx.roster.has_tag("m1", "scudo_equipaggiato")); // depleted.

	// Third block: no shield left, full damage minus nothing.
	assert(fx.actions.declare_attack("hero", "m1", 0, base));
	DefenseChoice c3;
	int hp3 = 0;
	assert(fx.actions.resolve_attack("m1", base, c3, hp3) == 4);
	std::cout << "  [OK] test_shield_charges_decrement_then_deplete\n";
}

int main()
{
	std::cout << "=== DungeonCombat (attack + reactive defense) unit tests ===\n";
	test_declare_attack_valid();
	test_declare_attack_with_card_modifier();
	test_declare_attack_rejected_not_enemy();
	test_declare_attack_rejected_out_of_reach();
	test_defense_reduces_damage();
	test_defense_cancels_attack();
	test_defense_pass_takes_full_damage();
	test_defense_never_below_zero();
	test_difeso_status_reduces_and_is_consumed();
	test_shield_charges_decrement_then_deplete();
	std::cout << "All DungeonCombat tests PASSED.\n";
	return 0;
}
