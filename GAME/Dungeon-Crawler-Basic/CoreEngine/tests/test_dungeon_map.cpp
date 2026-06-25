/**
 * @file tests/test_dungeon_map.cpp
 * @brief Unit tests for DungeonMap.
 */

#include "world/DungeonMap.hpp"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void test_create_and_has_room()
{
	gmDungeonBasic::DungeonMap map;
	assert(!map.has_room("room_1"));
	map.create_room("room_1");
	assert(map.has_room("room_1"));
	std::cout << "  [OK] test_create_and_has_room\n";
}

static void test_duplicate_room_throws()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("room_1");
	bool threw = false;
	try { map.create_room("room_1"); } catch (const std::invalid_argument&) { threw = true; }
	assert(threw);
	std::cout << "  [OK] test_duplicate_room_throws\n";
}

static void test_add_connection_and_adjacency()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("A");
	map.create_room("B");
	map.create_room("C");
	map.add_connection("A", "B");
	assert(map.is_adjacent("A", "B"));
	assert(map.is_adjacent("B", "A"));   // bidirectional by default
	assert(!map.is_adjacent("A", "C"));
	std::cout << "  [OK] test_add_connection_and_adjacency\n";
}

static void test_one_way_connection()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("A");
	map.create_room("B");
	map.add_connection("A", "B", false);
	assert(map.is_adjacent("A", "B"));
	assert(!map.is_adjacent("B", "A"));
	std::cout << "  [OK] test_one_way_connection\n";
}

static void test_room_tags()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("room_1");
	assert(!map.room_has_tag("room_1", "start"));
	map.set_room_tag("room_1", "start");
	assert(map.room_has_tag("room_1", "start"));
	map.remove_room_tag("room_1", "start");
	assert(!map.room_has_tag("room_1", "start"));
	std::cout << "  [OK] test_room_tags\n";
}

static void test_all_rooms()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("B");
	map.create_room("A");
	map.create_room("C");
	const std::vector<std::string> rooms = map.all_rooms();
	assert(rooms.size() == 3);
	assert(std::find(rooms.begin(), rooms.end(), "A") != rooms.end());
	std::cout << "  [OK] test_all_rooms\n";
}

static void test_rooms_adjacent_to()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("X");
	map.create_room("Y");
	map.create_room("Z");
	map.add_connection("X", "Y");
	map.add_connection("X", "Z");
	const std::vector<std::string> adj = map.rooms_adjacent_to("X");
	assert(adj.size() == 2);
	std::cout << "  [OK] test_rooms_adjacent_to\n";
}

static void test_reset()
{
	gmDungeonBasic::DungeonMap map;
	map.create_room("room_1");
	map.reset();
	assert(!map.has_room("room_1"));
	assert(map.all_rooms().empty());
	std::cout << "  [OK] test_reset\n";
}

int main()
{
	std::cout << "=== DungeonMap unit tests ===\n";
	test_create_and_has_room();
	test_duplicate_room_throws();
	test_add_connection_and_adjacency();
	test_one_way_connection();
	test_room_tags();
	test_all_rooms();
	test_rooms_adjacent_to();
	test_reset();
	std::cout << "All DungeonMap tests PASSED.\n";
	return 0;
}
