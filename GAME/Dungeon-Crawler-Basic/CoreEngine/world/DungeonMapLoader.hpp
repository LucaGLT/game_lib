#ifndef GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP
#define GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP

/**
 * @file world/DungeonMapLoader.hpp
 * @brief JSON-based loader for dungeon maps and initial actor placements.
 *
 * DungeonMapLoader reads a dungeon definition file formatted as JSON and
 * populates a @ref DungeonMap and an @ref ActorRoster. It uses
 * @c gmSave/json.hpp (nlohmann::json) for parsing so that the same JSON
 * infrastructure used elsewhere in the project is reused here.
 *
 * @par Expected JSON schema (info/wire-contract-v1.md for full spec):
 * @code{.json}
 * {
 *   "map_id": "dungeon_01",
 *   "rooms": [
 *     { "id": "room_1", "tags": ["start"], "adjacent": ["room_2"] }
 *   ],
 *   "actors": [
 *     { "id": "hero", "kind": "HERO", "hp": 10, "max_hp": 10,
 *       "room": "room_1", "tags": [] }
 *   ]
 * }
 * @endcode
 */

#include "actors/ActorRoster.hpp"
#include "world/DungeonMap.hpp"

#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Parses a dungeon JSON file and populates the map and actor roster.
 *
 * Each call to @ref load_from_file replaces all previous content in the
 * supplied @p map and @p actors objects. On failure the objects are left in
 * an unspecified but valid state; call @c reset() on them to recover.
 *
 * @note Not thread-safe.
 */
class DungeonMapLoader
{
public:
	/// @brief Constructs a loader with no file loaded.
	DungeonMapLoader();

	/**
	 * @brief Loads a dungeon definition from a JSON file.
	 *
	 * @param file_path  Absolute or relative path to the JSON file.
	 * @param map        Target DungeonMap to populate (existing content cleared).
	 * @param actors     Target ActorRoster to populate (existing content cleared).
	 * @return           @c true on success, @c false on any I/O or parse error.
	 *
	 * @note On failure call @ref last_error() for a human-readable description.
	 */
	bool load_from_file(const std::string& file_path,
	                    DungeonMap&         map,
	                    ActorRoster&        actors);

	/**
	 * @brief Returns a human-readable description of the last load failure.
	 *
	 * Empty string if the last @ref load_from_file call succeeded.
	 *
	 * @return  Error description string.
	 */
	std::string last_error() const;

private:
	std::string _last_error;  ///< Set by load_from_file on failure.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONMAPLOADER_HPP
