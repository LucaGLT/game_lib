/**
 * @file world/DungeonMap.cpp
 * @brief Stub implementation of DungeonMap.
 *
 * All methods are placeholder stubs. Real integration with gmMap will be
 * introduced in FASE B.
 */

#include "world/DungeonMap.hpp"

namespace gmDungeonBasic
{

DungeonMap::DungeonMap()
{
	// ToBeImplemented //
}

void DungeonMap::create_room(const std::string& room_id)
{
	(void)room_id;
	// ToBeImplemented //
}

void DungeonMap::add_connection(const std::string& from_id,
                                const std::string& to_id,
                                bool               bidirectional)
{
	(void)from_id;
	(void)to_id;
	(void)bidirectional;
	// ToBeImplemented //
}

void DungeonMap::set_room_tag(const std::string& room_id, const std::string& tag)
{
	(void)room_id;
	(void)tag;
	// ToBeImplemented //
}

void DungeonMap::remove_room_tag(const std::string& room_id, const std::string& tag)
{
	(void)room_id;
	(void)tag;
	// ToBeImplemented //
}

bool DungeonMap::has_room(const std::string& room_id) const
{
	(void)room_id;
	// ToBeImplemented //
	return false;
}

bool DungeonMap::is_adjacent(const std::string& from_id, const std::string& to_id) const
{
	(void)from_id;
	(void)to_id;
	// ToBeImplemented //
	return false;
}

bool DungeonMap::room_has_tag(const std::string& room_id, const std::string& tag) const
{
	(void)room_id;
	(void)tag;
	// ToBeImplemented //
	return false;
}

std::vector<std::string> DungeonMap::all_rooms() const
{
	// ToBeImplemented //
	return {};
}

std::vector<std::string> DungeonMap::rooms_adjacent_to(const std::string& room_id) const
{
	(void)room_id;
	// ToBeImplemented //
	return {};
}

std::vector<std::string> DungeonMap::tags_of_room(const std::string& room_id) const
{
	(void)room_id;
	// ToBeImplemented //
	return {};
}

void DungeonMap::reset()
{
	// ToBeImplemented //
}

} // namespace gmDungeonBasic
