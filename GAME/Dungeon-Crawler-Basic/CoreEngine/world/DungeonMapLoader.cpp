/**
 * @file world/DungeonMapLoader.cpp
 * @brief JSON loader implementation for dungeon map and actors via gmSave.
 */

#include "world/DungeonMapLoader.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "gmSave/gmSave.hpp"

namespace {

namespace fs = std::filesystem;

gmDungeonBasic::DungeonActorKind parse_kind(const std::string& kind)
{
	if (kind == "HERO")          return gmDungeonBasic::DungeonActorKind::HERO;
	if (kind == "MONSTER")       return gmDungeonBasic::DungeonActorKind::MONSTER;
	if (kind == "MONSTER_ELITE") return gmDungeonBasic::DungeonActorKind::MONSTER_ELITE;
	if (kind == "BOSS_MONSTER")  return gmDungeonBasic::DungeonActorKind::BOSS_MONSTER;
	throw std::invalid_argument("Unknown actor kind: " + kind);
}

fs::path resolve_path(const std::string& file_path)
{
	const fs::path original(file_path);
	if (fs::exists(original))
		return original;

	const fs::path cache_candidate = fs::path(".cache") / original;
	if (fs::exists(cache_candidate))
		return cache_candidate;

	fs::path current = fs::current_path();
	while (!current.empty())
	{
		if (fs::exists(current / original))      return current / original;
		if (fs::exists(current / cache_candidate)) return current / cache_candidate;
		if (current == current.root_path())        break;
		current = current.parent_path();
	}
	return original;
}

} // namespace

namespace gmDungeonBasic
{

// ── ADL from_json / to_json ───────────────────────────────────────────────────

void from_json(const nlohmann::json& j, DungeonRoomMeta& m)
{
	m.zone_id            = j.value("zone_id",            "");
	m.region_id          = j.value("region_id",          "");
	m.zone_color_token   = j.value("zone_color_token",   "");
	m.region_color_token = j.value("region_color_token", "");
	if (j.contains("items") && j.at("items").is_array())
		m.items = j.at("items").get<std::vector<std::string>>();
}

void to_json(nlohmann::json& j, const DungeonRoomMeta& m)
{
	j["zone_id"]            = m.zone_id;
	j["region_id"]          = m.region_id;
	j["zone_color_token"]   = m.zone_color_token;
	j["region_color_token"] = m.region_color_token;
	j["items"]              = m.items;
}

void from_json(const nlohmann::json& j, DungeonActorDef& a)
{
	a.id     = j.at("id").get<std::string>();
	a.kind   = j.at("kind").get<std::string>();
	a.hp     = j.value("hp",     1);
	a.max_hp = j.value("max_hp", std::max(1, a.hp));
	a.room   = j.at("room").get<std::string>();
	if (j.contains("tags")     && j.at("tags").is_array())
		a.tags     = j.at("tags").get<std::vector<std::string>>();
	if (j.contains("statuses") && j.at("statuses").is_array())
		a.statuses = j.at("statuses").get<std::vector<std::string>>();
}

void to_json(nlohmann::json& j, const DungeonActorDef& a)
{
	j["id"]       = a.id;
	j["kind"]     = a.kind;
	j["hp"]       = a.hp;
	j["max_hp"]   = a.max_hp;
	j["room"]     = a.room;
	j["tags"]     = a.tags;
	j["statuses"] = a.statuses;
}

void from_json(const nlohmann::json& j, DungeonRoomDef& r)
{
	r.id = j.at("id").get<std::string>();
	if (j.contains("tags")     && j.at("tags").is_array())
		r.tags     = j.at("tags").get<std::vector<std::string>>();
	if (j.contains("adjacent") && j.at("adjacent").is_array())
		r.adjacent = j.at("adjacent").get<std::vector<std::string>>();
	// Meta fields live inline in the room object (not nested).
	from_json(j, r.meta);
}

void to_json(nlohmann::json& j, const DungeonRoomDef& r)
{
	j["id"]       = r.id;
	j["tags"]     = r.tags;
	j["adjacent"] = r.adjacent;
	// Merge meta fields inline.
	nlohmann::json meta_j;
	to_json(meta_j, r.meta);
	j.merge_patch(meta_j);
}

void from_json(const nlohmann::json& j, DungeonMapDef& d)
{
	d.map_id = j.value("map_id", "");
	if (j.contains("rooms") && j.at("rooms").is_array())
	{
		for (const auto& rj : j.at("rooms"))
		{
			DungeonRoomDef r;
			from_json(rj, r);
			d.rooms.push_back(std::move(r));
		}
	}
	if (j.contains("actors") && j.at("actors").is_array())
	{
		for (const auto& aj : j.at("actors"))
		{
			DungeonActorDef a;
			from_json(aj, a);
			d.actors.push_back(std::move(a));
		}
	}
}

void to_json(nlohmann::json& j, const DungeonMapDef& d)
{
	j["map_id"] = d.map_id;
	j["rooms"]  = nlohmann::json::array();
	for (const auto& r : d.rooms)
	{
		nlohmann::json rj;
		to_json(rj, r);
		j["rooms"].push_back(rj);
	}
	j["actors"] = nlohmann::json::array();
	for (const auto& a : d.actors)
	{
		nlohmann::json aj;
		to_json(aj, a);
		j["actors"].push_back(aj);
	}
}

// ── DungeonMapLoader ──────────────────────────────────────────────────────────

DungeonMapLoader::DungeonMapLoader()
	: _last_error("")
{}

bool DungeonMapLoader::load_from_file(
	const std::string& file_path,
	DungeonMap&         map,
	ActorRoster&        actors,
	std::unordered_map<std::string, DungeonRoomMeta>& room_meta)
{
	_last_error.clear();
	try
	{
		const std::string resolved = resolve_path(file_path).string();
		const DungeonMapDef def    = gmSave::load<DungeonMapDef>(resolved);

		map.reset();
		actors.reset();
		room_meta.clear();

		// Pass 1: create rooms.
		for (const DungeonRoomDef& r : def.rooms)
			map.create_room(r.id);

		// Pass 2: tags, connections, metadata.
		for (const DungeonRoomDef& r : def.rooms)
		{
			for (const std::string& tag : r.tags)
				map.set_room_tag(r.id, tag);
			for (const std::string& adj : r.adjacent)
				map.add_connection(r.id, adj, false);
			room_meta[r.id] = r.meta;
		}

		// Pass 3: actors.
		for (const DungeonActorDef& a : def.actors)
		{
			ActorInfo info;
			info.id       = a.id;
			info.kind     = parse_kind(a.kind);
			info.hp       = a.hp;
			info.max_hp   = a.max_hp;
			info.location = a.room;
			info.tags     = a.tags;
			info.statuses = a.statuses;
			actors.add_actor(info);
			if (map.has_room(info.location))
				map.place_actor(info.location, info.id);
		}

		return true;
	}
	catch (const std::exception& ex)
	{
		_last_error = ex.what();
		return false;
	}
}

std::string DungeonMapLoader::last_error() const
{
	return _last_error;
}

} // namespace gmDungeonBasic

