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

void DungeonMap::place_actor(const std::string& room_id, const std::string& actor_id)
{
	const gmMap::LocationId destination = location_of(room_id);

	const auto known = _actor_to_uid.find(actor_id);
	gmMap::ActorId uid = 0;
	if (known == _actor_to_uid.end())
	{
		uid = _next_actor_uid++;
		_actor_to_uid[actor_id] = uid;
		_uid_to_actor[uid]      = actor_id;
	}
	else
	{
		uid = known->second;
		const auto previous = _actor_room.find(actor_id);
		if (previous != _actor_room.end() && has_room(previous->second))
		{
			_map.remove_actor(location_of(previous->second), uid);
		}
	}

	_map.place_actor(destination, uid);
	_actor_room[actor_id] = room_id;
}

void DungeonMap::move_actor(const std::string& actor_id, const std::string& to_room_id)
{
	place_actor(to_room_id, actor_id);
}

void DungeonMap::remove_actor(const std::string& actor_id)
{
	const auto known = _actor_to_uid.find(actor_id);
	if (known == _actor_to_uid.end())
	{
		return;
	}

	const gmMap::ActorId uid = known->second;
	const auto previous = _actor_room.find(actor_id);
	if (previous != _actor_room.end() && has_room(previous->second))
	{
		_map.remove_actor(location_of(previous->second), uid);
	}

	_actor_to_uid.erase(actor_id);
	_uid_to_actor.erase(uid);
	_actor_room.erase(actor_id);
}

std::vector<std::string> DungeonMap::actors_in_room(const std::string& room_id) const
{
	const std::vector<gmMap::ActorId> uids = _map.actors_at(location_of(room_id));
	std::vector<std::string> out;
	out.reserve(uids.size());
	for (gmMap::ActorId uid : uids)
	{
		const auto it = _uid_to_actor.find(uid);
		if (it != _uid_to_actor.end())
		{
			out.push_back(it->second);
		}
	}
	std::sort(out.begin(), out.end());
	return out;
}

void DungeonMap::place_interactable(const std::string&          room_id,
                                    gmMap::InteractableObjectId obj_id)
{
	_map.place_interactable(location_of(room_id), obj_id);
}

void DungeonMap::remove_interactable(const std::string&          room_id,
                                     gmMap::InteractableObjectId obj_id)
{
	_map.remove_interactable(location_of(room_id), obj_id);
}

std::vector<gmMap::InteractableObjectId>
DungeonMap::interactables_in_room(const std::string& room_id) const
{
	return _map.interactables_at(location_of(room_id));
}

void DungeonMap::reset()
{
	_map.clear();
	_room_to_location.clear();
	_location_to_room.clear();
	_next_location_id = 1;
	_actor_to_uid.clear();
	_uid_to_actor.clear();
	_actor_room.clear();
	_next_actor_uid = 1;
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
