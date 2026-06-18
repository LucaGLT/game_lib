/**
 * @file world/DungeonMap.cpp
 * @brief DungeonMap implementation backed by gmMap.
 */

#include "world/DungeonMap.hpp"

#include <algorithm>
#include <stdexcept>

namespace gmDungeonBasic
{

DungeonMap::DungeonMap()
{
	// ToBeImplemented //
}

void DungeonMap::create_room(const std::string& room_id)
{
	if (_room_to_location.find(room_id) != _room_to_location.end())
	{
		throw std::invalid_argument("Room already exists: " + room_id);
	}

	const gmMap::LocationId id = _next_location_id++;
	_map.create_location(id);
	_room_to_location[room_id] = id;
	_location_to_room[id]      = room_id;
}

void DungeonMap::add_connection(const std::string& from_id,
                                const std::string& to_id,
                                bool               bidirectional)
{
	const gmMap::LocationId from_loc = location_of(from_id);
	const gmMap::LocationId to_loc   = location_of(to_id);
	_map.set_adjacent(from_loc, to_loc, bidirectional);
}

void DungeonMap::set_room_tag(const std::string& room_id, const std::string& tag)
{
	_map.set_location_meta(location_of(room_id), tag, true);
}

void DungeonMap::remove_room_tag(const std::string& room_id, const std::string& tag)
{
	const gmMap::LocationId id = location_of(room_id);
	if (_map.has_location_meta(id, tag))
	{
		_map.remove_location_meta(id, tag);
	}
}

bool DungeonMap::has_room(const std::string& room_id) const
{
	return _room_to_location.find(room_id) != _room_to_location.end();
}

bool DungeonMap::is_adjacent(const std::string& from_id, const std::string& to_id) const
{
	if (!has_room(from_id) || !has_room(to_id))
	{
		return false;
	}
	return _map.are_adjacent(location_of(from_id), location_of(to_id));
}

bool DungeonMap::room_has_tag(const std::string& room_id, const std::string& tag) const
{
	if (!has_room(room_id))
	{
		return false;
	}
	return _map.has_location_meta(location_of(room_id), tag);
}

std::vector<std::string> DungeonMap::all_rooms() const
{
	std::vector<std::string> out;
	out.reserve(_room_to_location.size());
	for (const auto& kv : _room_to_location)
	{
		out.push_back(kv.first);
	}
	std::sort(out.begin(), out.end());
	return out;
}

std::vector<std::string> DungeonMap::rooms_adjacent_to(const std::string& room_id) const
{
	const gmMap::LocationId id = location_of(room_id);
	const std::vector<gmMap::LocationId> neighbors = _map.adjacent_to(id);
	std::vector<std::string> out;
	out.reserve(neighbors.size());
	for (gmMap::LocationId neighbor_id : neighbors)
	{
		out.push_back(_location_to_room.at(neighbor_id));
	}
	std::sort(out.begin(), out.end());
	return out;
}

std::vector<std::string> DungeonMap::tags_of_room(const std::string& room_id) const
{
	const gmMap::Metadata& metadata = _map.location_metadata(location_of(room_id));
	std::vector<std::string> out;
	out.reserve(metadata.size());
	for (const auto& kv : metadata)
	{
		out.push_back(kv.first);
	}
	std::sort(out.begin(), out.end());
	return out;
}

void DungeonMap::reset()
{
	_map.clear();
	_room_to_location.clear();
	_location_to_room.clear();
	_next_location_id = 1;
}

gmMap::LocationId DungeonMap::location_of(const std::string& room_id) const
{
	const auto it = _room_to_location.find(room_id);
	if (it == _room_to_location.end())
	{
		throw std::invalid_argument("Unknown room id: " + room_id);
	}
	return it->second;
}

} // namespace gmDungeonBasic
