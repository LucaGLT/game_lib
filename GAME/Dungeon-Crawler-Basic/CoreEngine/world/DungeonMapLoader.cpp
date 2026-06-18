/**
 * @file world/DungeonMapLoader.cpp
 * @brief JSON loader implementation for dungeon map and actors.
 */

#include "world/DungeonMapLoader.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "gmSave/json.hpp"

namespace {

namespace fs = std::filesystem;

std::string json_get_string(const nlohmann::json& j, const char* key)
{
	if (!j.contains(key) || !j.at(key).is_string())
	{
		throw std::invalid_argument(std::string("Missing or invalid string field: ") + key);
	}
	return j.at(key).get<std::string>();
}

gmDungeonBasic::DungeonActorKind parse_kind(const std::string& kind)
{
	if (kind == "HERO")
	{
		return gmDungeonBasic::DungeonActorKind::HERO;
	}
	if (kind == "MONSTER")
	{
		return gmDungeonBasic::DungeonActorKind::MONSTER;
	}
	if (kind == "MONSTER_ELITE")
	{
		return gmDungeonBasic::DungeonActorKind::MONSTER_ELITE;
	}
	if (kind == "BOSS_MONSTER")
	{
		return gmDungeonBasic::DungeonActorKind::BOSS_MONSTER;
	}

	throw std::invalid_argument("Unknown actor kind: " + kind);
}

fs::path resolve_path(const std::string& file_path)
{
	const fs::path original(file_path);
	if (fs::exists(original))
	{
		return original;
	}

	const fs::path cache_candidate = fs::path(".cache") / original;
	if (fs::exists(cache_candidate))
	{
		return cache_candidate;
	}

	fs::path current = fs::current_path();
	while (!current.empty())
	{
		const fs::path upward_original = current / original;
		if (fs::exists(upward_original))
		{
			return upward_original;
		}

		const fs::path upward_cache = current / ".cache" / original;
		if (fs::exists(upward_cache))
		{
			return upward_cache;
		}

		if (current == current.root_path())
		{
			break;
		}

		current = current.parent_path();
	}

	return original;
}

} // namespace

namespace gmDungeonBasic
{

DungeonMapLoader::DungeonMapLoader()
	: _last_error("")
{
	// ToBeImplemented //
}

bool DungeonMapLoader::load_from_file(const std::string& file_path,
                                      DungeonMap&         map,
                                      ActorRoster&        actors)
{
	_last_error.clear();

	try
	{
		const std::filesystem::path input = resolve_path(file_path);
		std::ifstream ifs(input);
		if (!ifs)
		{
			_last_error = "Cannot open map file: " + input.string();
			return false;
		}

		nlohmann::json root;
		ifs >> root;

		if (!root.contains("rooms") || !root.at("rooms").is_array())
		{
			_last_error = "Invalid JSON: missing 'rooms' array";
			return false;
		}

		map.reset();
		actors.reset();

		for (const nlohmann::json& room : root.at("rooms"))
		{
			const std::string room_id = json_get_string(room, "id");
			map.create_room(room_id);
		}

		for (const nlohmann::json& room : root.at("rooms"))
		{
			const std::string room_id = json_get_string(room, "id");

			if (room.contains("tags") && room.at("tags").is_array())
			{
				for (const nlohmann::json& tag : room.at("tags"))
				{
					if (tag.is_string())
					{
						map.set_room_tag(room_id, tag.get<std::string>());
					}
				}
			}

			if (room.contains("adjacent") && room.at("adjacent").is_array())
			{
				for (const nlohmann::json& adjacent : room.at("adjacent"))
				{
					if (adjacent.is_string())
					{
						map.add_connection(room_id, adjacent.get<std::string>(), false);
					}
				}
			}
		}

		if (root.contains("actors") && root.at("actors").is_array())
		{
			for (const nlohmann::json& actor : root.at("actors"))
			{
				ActorInfo info;
				info.id = json_get_string(actor, "id");
				info.kind = parse_kind(json_get_string(actor, "kind"));
				info.hp = actor.value("hp", 1);
				info.max_hp = actor.value("max_hp", std::max(1, info.hp));
				info.location = json_get_string(actor, "room");

				if (actor.contains("tags") && actor.at("tags").is_array())
				{
					for (const nlohmann::json& tag : actor.at("tags"))
					{
						if (tag.is_string())
						{
							info.tags.push_back(tag.get<std::string>());
						}
					}
				}

				if (actor.contains("statuses") && actor.at("statuses").is_array())
				{
					for (const nlohmann::json& status : actor.at("statuses"))
					{
						if (status.is_string())
						{
							info.statuses.push_back(status.get<std::string>());
						}
					}
				}

				actors.add_actor(info);
			}
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
