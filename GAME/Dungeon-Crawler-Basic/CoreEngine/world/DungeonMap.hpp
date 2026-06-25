#ifndef GMDUNGEONBASIC_DUNGEONMAP_HPP
#define GMDUNGEONBASIC_DUNGEONMAP_HPP

/**
 * @file world/DungeonMap.hpp
 * @brief Dungeon spatial model: rooms, connections, tags and actor positions.
 *
 * Wraps a @c gmMap::gmMap<std::string> instance to represent the dungeon as a
 * graph of named rooms. Each room maps to a @c gmMap::LocationId internally;
 * the public API exposes only string-based room identifiers so that the rest of
 * the engine is decoupled from gmMap's uint32_t type system.
 *
 * @note Not thread-safe.
 */

#include "gmMap/gmMap.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gmDungeonBasic
{

/**
 * @brief Graph-based model of a dungeon: rooms connected by passages.
 *
 * Each room has a unique string identifier, an optional set of tags (e.g.
 * "start", "has_potion", "has_item"), and a set of directional connections to
 * adjacent rooms. The underlying storage is delegated to @c gmMap::gmMap.
 *
 * @note Connections are bidirectional by default but can be one-way.
 */
class DungeonMap
{
public:
	/// @brief Constructs an empty dungeon map.
	DungeonMap();

	/**
	 * @brief Creates a new room in the dungeon.
	 *
	 * @param room_id  Unique string identifier for the room.
	 * @throws std::invalid_argument  If a room with the same id already exists.
	 */
	void create_room(const std::string& room_id);

	/**
	 * @brief Adds a connection between two rooms.
	 *
	 * @param from_id       Source room identifier.
	 * @param to_id         Destination room identifier.
	 * @param bidirectional If true, the reverse connection is also added.
	 * @throws std::invalid_argument  If either room does not exist.
	 */
	void add_connection(const std::string& from_id,
	                    const std::string& to_id,
	                    bool               bidirectional = true);

	/**
	 * @brief Attaches a tag to a room.
	 *
	 * @param room_id  Room to tag.
	 * @param tag      Tag string (e.g. "start", "has_potion").
	 * @throws std::invalid_argument  If the room does not exist.
	 */
	void set_room_tag(const std::string& room_id, const std::string& tag);

	/**
	 * @brief Removes a tag from a room.
	 *
	 * @param room_id  Room whose tag should be removed.
	 * @param tag      Tag string to remove.
	 */
	void remove_room_tag(const std::string& room_id, const std::string& tag);

	/**
	 * @brief Checks whether a room with the given id exists.
	 *
	 * @param room_id  Room identifier to query.
	 * @return         @c true if the room exists, @c false otherwise.
	 */
	bool has_room(const std::string& room_id) const;

	/**
	 * @brief Checks whether two rooms are directly connected.
	 *
	 * @param from_id  Source room identifier.
	 * @param to_id    Destination room identifier.
	 * @return         @c true if a direct connection exists from @p from_id
	 *                 to @p to_id.
	 */
	bool is_adjacent(const std::string& from_id, const std::string& to_id) const;

	/**
	 * @brief Checks whether a room has a specific tag.
	 *
	 * @param room_id  Room to query.
	 * @param tag      Tag string to look for.
	 * @return         @c true if the tag is present.
	 */
	bool room_has_tag(const std::string& room_id, const std::string& tag) const;

	/**
	 * @brief Returns all room identifiers in the dungeon.
	 *
	 * @return  Vector of room id strings in unspecified order.
	 */
	std::vector<std::string> all_rooms() const;

	/**
	 * @brief Returns the rooms directly connected from a given room.
	 *
	 * @param room_id  Source room identifier.
	 * @return         Vector of adjacent room id strings.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	std::vector<std::string> rooms_adjacent_to(const std::string& room_id) const;

	/**
	 * @brief Returns all tags attached to a room.
	 *
	 * @param room_id  Room to query.
	 * @return         Vector of tag strings.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	std::vector<std::string> tags_of_room(const std::string& room_id) const;

	// ── Actor presence (delegates to gmMap native actor sets) ─────────────────

	/**
	 * @brief Records an actor's presence in a room.
	 *
	 * If the actor is already placed elsewhere it is moved to @p room_id. A
	 * stable opaque @c gmMap::ActorId is assigned to each string actor id on
	 * first placement.
	 *
	 * @param room_id   Destination room identifier.
	 * @param actor_id  Actor string identifier.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	void place_actor(const std::string& room_id, const std::string& actor_id);

	/**
	 * @brief Moves an actor to a different room.
	 *
	 * Equivalent to @ref place_actor for an already-known actor; places the
	 * actor if it was not yet present.
	 *
	 * @param actor_id     Actor to move.
	 * @param to_room_id   Destination room identifier.
	 * @throws std::invalid_argument  If @p to_room_id does not exist.
	 */
	void move_actor(const std::string& actor_id, const std::string& to_room_id);

	/**
	 * @brief Removes an actor's presence from the map (no-op if unknown).
	 *
	 * @param actor_id  Actor string identifier.
	 */
	void remove_actor(const std::string& actor_id);

	/**
	 * @brief Returns the ids of all actors currently in a room.
	 *
	 * @param room_id  Room identifier to query.
	 * @return         Sorted vector of actor id strings.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	std::vector<std::string> actors_in_room(const std::string& room_id) const;

	// ── Interactable placement (opaque ids owned by gmInteraction) ─────────────

	/**
	 * @brief Places an interactable object id in a room.
	 *
	 * @param room_id  Room identifier.
	 * @param obj_id   Opaque interactable object id (owned by gmInteraction).
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	void place_interactable(const std::string& room_id, gmMap::InteractableObjectId obj_id);

	/**
	 * @brief Removes an interactable object id from a room (no-op if absent).
	 *
	 * @param room_id  Room identifier.
	 * @param obj_id   Opaque interactable object id.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	void remove_interactable(const std::string& room_id, gmMap::InteractableObjectId obj_id);

	/**
	 * @brief Returns the interactable object ids placed in a room.
	 *
	 * @param room_id  Room identifier to query.
	 * @return         Vector of opaque interactable object ids.
	 * @throws std::invalid_argument  If @p room_id does not exist.
	 */
	std::vector<gmMap::InteractableObjectId>
	interactables_in_room(const std::string& room_id) const;

	/// @brief Removes all rooms, connections and tags. Resets to empty state.
	void reset();

private:
	gmMap::LocationId location_of(const std::string& room_id) const;

	gmMap::gmMap<std::string> _map;
	std::unordered_map<std::string, gmMap::LocationId> _room_to_location;
	std::unordered_map<gmMap::LocationId, std::string> _location_to_room;
	gmMap::LocationId _next_location_id = 1;

	std::unordered_map<std::string, gmMap::ActorId> _actor_to_uid;
	std::unordered_map<gmMap::ActorId, std::string> _uid_to_actor;
	std::unordered_map<std::string, std::string>    _actor_room;
	gmMap::ActorId _next_actor_uid = 1;
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONMAP_HPP
