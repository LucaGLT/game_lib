/**
 * @file tests/test_map_loader.cpp
 * @brief Unit test for DungeonMapLoader with the .cache test map.
 */

#include "actors/ActorRoster.hpp"
#include "world/DungeonMap.hpp"
#include "world/DungeonMapLoader.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
	std::cout << "=== DungeonMapLoader unit tests ===\n";

	gmDungeonBasic::DungeonMap       map;
	gmDungeonBasic::ActorRoster      roster;
	gmDungeonBasic::DungeonMapLoader loader;

	// The test map lives in .cache/maps/dungeon_01.json.
	// When running from build output dir the path is relative to the exe.
	const std::string map_path = ".cache/maps/dungeon_01.json";

	const bool ok = loader.load_from_file(map_path, map, roster);
	if (!ok)
	{
		std::cerr << "[SKIP] Cannot open " << map_path
		          << ": " << loader.last_error() << "\n"
		          << "  Run tests from the workspace root.\n";
		return 0;  // Not a hard failure — file may be absent in CI
	}

	// Rooms
	const auto rooms = map.all_rooms();
	assert(rooms.size() == 3);
	assert(map.has_room("room_start"));
	assert(map.has_room("room_corridor"));
	assert(map.has_room("room_boss"));

	// Tags
	assert(map.room_has_tag("room_start", "start"));
	assert(map.room_has_tag("room_boss", "boss_room"));

	// Adjacency
	// JSON adjacency is one-way per entry; room_start→room_corridor
	assert(map.is_adjacent("room_start", "room_corridor"));

	// Actors
	assert(roster.has_actor("hero"));
	assert(roster.has_actor("monster_1"));
	assert(roster.has_actor("boss"));
	assert(roster.heroes().size() == 1);
	assert(roster.enemies().size() == 2);

	const auto hero_info = roster.get_actor("hero");
	assert(hero_info.hp == 10);
	assert(hero_info.max_hp == 10);
	assert(hero_info.location == "room_start");
	assert(roster.has_tag("hero", "has_potion"));
	assert(roster.has_tag("hero", "bigword_available"));

	std::cout << "  [OK] load_from_file with .cache map\n";
	std::cout << "All MapLoader tests PASSED.\n";
	return 0;
}
