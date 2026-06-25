/**
 * @file tests/test_actor_roster.cpp
 * @brief Unit tests for ActorRoster.
 */

#include "actors/ActorRoster.hpp"
#include "engine/DungeonTypes.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using gmDungeonBasic::ActorInfo;
using gmDungeonBasic::ActorRoster;
using gmDungeonBasic::DungeonActorKind;

static ActorInfo make_hero(const std::string& id = "hero",
                           const std::string& room = "room_1",
                           int hp = 10, int max_hp = 10)
{
	ActorInfo info;
	info.id = id;
	info.kind = DungeonActorKind::HERO;
	info.hp = hp;
	info.max_hp = max_hp;
	info.location = room;
	return info;
}

static ActorInfo make_monster(const std::string& id = "m1",
                              const std::string& room = "room_2",
                              int hp = 5, int max_hp = 5)
{
	ActorInfo info;
	info.id = id;
	info.kind = DungeonActorKind::MONSTER;
	info.hp = hp;
	info.max_hp = max_hp;
	info.location = room;
	return info;
}

static void test_add_and_has()
{
	ActorRoster roster;
	assert(!roster.has_actor("hero"));
	roster.add_actor(make_hero());
	assert(roster.has_actor("hero"));
	std::cout << "  [OK] test_add_and_has\n";
}

static void test_duplicate_throws()
{
	ActorRoster roster;
	roster.add_actor(make_hero());
	bool threw = false;
	try { roster.add_actor(make_hero()); } catch (const std::invalid_argument&) { threw = true; }
	assert(threw);
	std::cout << "  [OK] test_duplicate_throws\n";
}

static void test_get_actor_snapshot()
{
	ActorRoster roster;
	roster.add_actor(make_hero("h", "room_A", 7, 10));
	const ActorInfo info = roster.get_actor("h");
	assert(info.id == "h");
	assert(info.hp == 7);
	assert(info.max_hp == 10);
	assert(info.location == "room_A");
	assert(info.kind == DungeonActorKind::HERO);
	std::cout << "  [OK] test_get_actor_snapshot\n";
}

static void test_heroes_and_enemies()
{
	ActorRoster roster;
	roster.add_actor(make_hero("hero"));
	roster.add_actor(make_monster("m1"));
	ActorInfo boss_info;
	boss_info.id = "boss";
	boss_info.kind = DungeonActorKind::BOSS_MONSTER;
	boss_info.hp = boss_info.max_hp = 20;
	boss_info.location = "room_3";
	roster.add_actor(boss_info);
	assert(roster.heroes().size() == 1);
	assert(roster.enemies().size() == 2);
	std::cout << "  [OK] test_heroes_and_enemies\n";
}

static void test_set_hp_clamped()
{
	ActorRoster roster;
	roster.add_actor(make_hero("h", "r", 10, 10));
	roster.set_hp("h", 15);        // above max → clamp to 10
	assert(roster.get_actor("h").hp == 10);
	roster.set_hp("h", -3);        // below 0 → clamp to 0
	assert(roster.get_actor("h").hp == 0);
	roster.set_hp("h", 5);
	assert(roster.get_actor("h").hp == 5);
	std::cout << "  [OK] test_set_hp_clamped\n";
}

static void test_tags()
{
	ActorRoster roster;
	roster.add_actor(make_hero());
	assert(!roster.has_tag("hero", "has_potion"));
	roster.add_tag("hero", "has_potion");
	assert(roster.has_tag("hero", "has_potion"));
	roster.remove_tag("hero", "has_potion");
	assert(!roster.has_tag("hero", "has_potion"));
	std::cout << "  [OK] test_tags\n";
}

static void test_statuses()
{
	ActorRoster roster;
	roster.add_actor(make_hero());
	assert(!roster.has_status("hero", "stunned"));
	roster.add_status("hero", "stunned");
	assert(roster.has_status("hero", "stunned"));
	roster.remove_status("hero", "stunned");
	assert(!roster.has_status("hero", "stunned"));
	std::cout << "  [OK] test_statuses\n";
}

static void test_move_to_and_actors_in_location()
{
	ActorRoster roster;
	roster.add_actor(make_hero("hero", "room_1"));
	roster.add_actor(make_monster("m1", "room_2"));
	assert(roster.actors_in_location("room_1").size() == 1);
	roster.move_to("hero", "room_2");
	assert(roster.get_actor("hero").location == "room_2");
	assert(roster.actors_in_location("room_2").size() == 2);
	std::cout << "  [OK] test_move_to_and_actors_in_location\n";
}

static void test_remove_actor()
{
	ActorRoster roster;
	roster.add_actor(make_hero());
	roster.add_actor(make_monster());
	roster.remove_actor("hero");
	assert(!roster.has_actor("hero"));
	assert(roster.has_actor("m1"));
	std::cout << "  [OK] test_remove_actor\n";
}

static void test_reset()
{
	ActorRoster roster;
	roster.add_actor(make_hero());
	roster.reset();
	assert(roster.all_actor_ids().empty());
	std::cout << "  [OK] test_reset\n";
}

int main()
{
	std::cout << "=== ActorRoster unit tests ===\n";
	test_add_and_has();
	test_duplicate_throws();
	test_get_actor_snapshot();
	test_heroes_and_enemies();
	test_set_hp_clamped();
	test_tags();
	test_statuses();
	test_move_to_and_actors_in_location();
	test_remove_actor();
	test_reset();
	std::cout << "All ActorRoster tests PASSED.\n";
	return 0;
}
