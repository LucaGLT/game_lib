#ifndef GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP
#define GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP

/**
 * @file world/DungeonMapLoader.hpp
 * @brief JSON-based loader for dungeon maps and initial actor placements.
 *
 * DungeonMapLoader reads a dungeon definition file via **gmSave** and
 * populates a @ref DungeonMap, an @ref ActorRoster, and a per-room metadata
 * map (@ref DungeonRoomMeta).
 *
 * @par Expected JSON schema (extended v2 — see info/wire-contract-v1.md):
 * @code{.json}
 * {
 *   "map_id": "dungeon_02",
 *   "rooms": [
 *     {
 *       "id": "start",
 *       "tags": ["start", "terrain:grass"],
 *       "adjacent": ["corridor_1"],
 *       "zone_id": "External",
 *       "region_id": "Extern",
 *       "zone_color_token": "map_zone_dark_green",
 *       "region_color_token": "map_region_green_ext",
 *       "items": []
 *     }
 *   ],
 *   "actors": [
 *     { "id": "hero_1", "kind": "HERO", "hp": 10, "max_hp": 10,
 *       "room": "start", "tags": [], "statuses": [] }
 *   ]
 * }
 * @endcode
 */

#include "actors/ActorRoster.hpp"
#include "world/DungeonMap.hpp"
#include "gmSave/json.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace gmDungeonBasic
{

// ── Data structs used for gmSave deserialization ──────────────────────────────

/**
 * @brief Visual and regional metadata for a single room.
 *
 * These fields are GUI-facing: they drive zone/region colouring in the map
 * view and define which items exist in the room.
 */
struct DungeonRoomMeta
{
	std::string zone_id;              ///< Zone identifier (e.g. "External", "Corridor").
	std::string region_id;            ///< Region identifier (e.g. "Extern", "Dunguen").
	std::string zone_color_token;     ///< Semantic colour token for zone fill.
	std::string region_color_token;   ///< Semantic colour token for region border.
	std::vector<std::string> items;   ///< Interactable item names present in this room.
};

/// @brief Raw definition of a single actor as loaded from JSON.
struct DungeonActorDef
{
	std::string              id;
	std::string              kind;
	int                      hp      = 1;
	int                      max_hp  = 1;
	int                      attack  = 0;
	int                      defense = 0;
	std::string              room;
	std::vector<std::string> tags;
	std::vector<std::string> statuses;
};

/// @brief Raw definition of a single room as loaded from JSON.
struct DungeonRoomDef
{
	std::string              id;
	std::vector<std::string> tags;
	std::vector<std::string> adjacent;
	DungeonRoomMeta          meta;   ///< Zone / region / colour / items.
};

/// @brief Top-level dungeon map definition as loaded from JSON.
struct DungeonMapDef
{
	std::string                  map_id;
	std::vector<DungeonRoomDef>  rooms;
	std::vector<DungeonActorDef> actors;
};

// ── ADL to_json / from_json (required by gmSave::load<T>) ─────────────────────

void to_json  (nlohmann::json& j, const DungeonRoomMeta&  m);
void from_json(const nlohmann::json& j,  DungeonRoomMeta&  m);

void to_json  (nlohmann::json& j, const DungeonActorDef&  a);
void from_json(const nlohmann::json& j,  DungeonActorDef&  a);

void to_json  (nlohmann::json& j, const DungeonRoomDef&   r);
void from_json(const nlohmann::json& j,  DungeonRoomDef&   r);

void to_json  (nlohmann::json& j, const DungeonMapDef&    d);
void from_json(const nlohmann::json& j,  DungeonMapDef&    d);

// ── Loader class ──────────────────────────────────────────────────────────────

/**
 * @brief Parses a dungeon JSON file and populates the map, actor roster and
 *        per-room metadata.
 *
 * Uses @c gmSave::load<DungeonMapDef>() for JSON deserialization so the file
 * format is governed by the @ref DungeonMapDef / @ref DungeonRoomDef /
 * @ref DungeonActorDef structs and their @c from_json counterparts.
 *
 * @note Not thread-safe.
 */
class DungeonMapLoader
{
public:
	/// @brief Constructs a loader with no file loaded.
	DungeonMapLoader();

	/**
	 * @brief Loads a dungeon definition from a JSON file via gmSave.
	 *
	 * @param file_path  Absolute or relative path to the JSON file.
	 * @param map        Target DungeonMap (cleared before populating).
	 * @param actors     Target ActorRoster (cleared before populating).
	 * @param room_meta  Output map of room_id → @ref DungeonRoomMeta (cleared
	 *                   before populating).
	 * @return  @c true on success, @c false on any I/O or parse error.
	 */
	bool load_from_file(
		const std::string& file_path,
		DungeonMap&         map,
		ActorRoster&        actors,
		std::unordered_map<std::string, DungeonRoomMeta>& room_meta);

	/**
	 * @brief Returns a human-readable description of the last load failure.
	 *
	 * Empty string if the last @ref load_from_file call succeeded.
	 */
	std::string last_error() const;

private:
	std::string _last_error;
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP
