#ifndef GMMAP_GMMAP_HPP
#define GMMAP_GMMAP_HPP

/**
 * @file gmMap.hpp
 * @brief Generic topology-agnostic game map for tabletop applications.
 *
 * @note Because gmMap is a class template, all method implementations must be
 *       visible at each instantiation site.  Stub bodies are therefore defined
 *       as inline template methods at the bottom of this header, below the
 *       class declaration.  They will be completed in subsequent development
 *       phases (see PLAN.md).
 */

#include "gmSave/gmSave.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace gmMap
{

// --- Type aliases -------------------------------------------------------------

/// @brief Unique identifier for a Location node in the map graph.
using LocationId = uint32_t;

/// @brief Unique identifier for a Tile (named group of locations).
using TileId = uint32_t;

/// @brief Stable identifier used to reference entities stored externally.
using EntityUid = uint64_t;

/// @brief Explicit UID reference for serializable metadata fields.
struct UidRef
{
	EntityUid value;
};

/// @brief List of UID references (e.g. links to multiple external entities).
using UidList = std::vector<UidRef>;

/// @brief Serializable metadata value used by gmMap for persistence-safe fields.
using MetadataValue =
    std::variant<std::nullptr_t, bool, int64_t, double, std::string, UidRef, UidList>;

/// @brief Key-value store used for metadata on locations and tiles.
using Metadata = std::unordered_map<std::string, MetadataValue>;

// --- Snapshot DTO for JSON persistence ----------------------------------------

/**
 * @brief Serializable snapshot of a gmMap state (includes locations, tiles,
 *        assignment, adjacency, items, and metadata).
 *
 * @tparam ItemT The item type stored at locations.
 */
template <typename ItemT> struct MapSnapshot
{
	/// All location IDs currently in the map.
	std::vector<LocationId> location_ids;

	/// All tile IDs currently in the map.
	std::vector<TileId> tile_ids;

	/// Location-to-tile assignments (location -> tile).
	std::vector<std::pair<LocationId, TileId>> assignments;

	/// Adjacency edges stored as pairs of location IDs.
	std::vector<std::pair<LocationId, LocationId>> adjacency_edges;

	/// Items stored at each location.
	std::unordered_map<LocationId, std::vector<ItemT>> items_by_location;

	/// Metadata for each location.
	std::unordered_map<LocationId, Metadata> location_metadata_map;

	/// Metadata for each tile.
	std::unordered_map<TileId, Metadata> tile_metadata_map;
};

// --- Exceptions ---------------------------------------------------------------

/**
 * @brief Base exception class for all gmMap errors.
 */
class EMapError : public std::runtime_error
{
  public:
	explicit EMapError(const std::string& message) : std::runtime_error("EMapError: " + message)
	{
	}
};

/**
 * @brief Thrown when trying to create a Location with an ID that already exists.
 */
class EDuplicateLocationError : public EMapError
{
  public:
	explicit EDuplicateLocationError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when referencing a LocationId that does not exist in the map.
 */
class EUnknownLocationError : public EMapError
{
  public:
	explicit EUnknownLocationError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when trying to create a Tile with an ID that already exists.
 */
class EDuplicateTileError : public EMapError
{
  public:
	explicit EDuplicateTileError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when referencing a TileId that does not exist in the map.
 */
class EUnknownTileError : public EMapError
{
  public:
	explicit EUnknownTileError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when an adjacency operation violates map invariants
 *        (e.g. self-loops, referencing a non-existent location).
 */
class EInvalidAdjacencyError : public EMapError
{
  public:
	explicit EInvalidAdjacencyError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when accessing a metadata key that does not exist.
 */
class EUnknownMetaKeyError : public EMapError
{
  public:
	explicit EUnknownMetaKeyError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when accessing an item at an out-of-range index.
 */
class EInvalidItemIndexError : public EMapError
{
  public:
	explicit EInvalidItemIndexError(const std::string& message) : EMapError(message)
	{
	}
};

// --- gmMap --------------------------------------------------------------------

/**
 * @class gmMap
 * @brief Generic topology-agnostic game map for tabletop game applications.
 *
 * @tparam ItemT  Type of items stored at each location.
 *
 * This class manages the complete state of a tabletop game map without
 * enforcing any grid or coordinate system.  Topology is expressed purely
 * through adjacency relationships, making the class suitable for dungeon
 * crawlers, war games, tactical maps, abstract strategy games, and similar
 * applications.
 *
 * ### Core concepts
 * - **Location** – An addressable node in the map graph (room, hex, territory…).
 * - **Tile**     – A named group of locations (zone, floor, region…).
 * - **Adjacency**– A directed or bidirectional edge between two locations.
 * - **Item**     – A typed game object placed at a location.
 * - **Metadata** – A serializable `string → MetadataValue` key-value store
 *                  attached to either a location or a tile.
 *
 * ### Invariants
 * - A location belongs to at most one tile at a time.
 * - Every neighbor referenced in an adjacency edge must be a valid location.
 * - When @p bidirectional is `true`, adjacency is kept symmetric.
 * - Removing a location also removes it from its tile and from all neighbor lists.
 * - Removing a tile does not remove its locations; it only ungroups them.
 *
 * @note All method bodies are defined as inline template functions below this
 *       class declaration (C++ template requirement).
 */
template <typename ItemT> class gmMap
{
  public:
	// -- Construction / reset --------------------------------------------------

	/**
     * @brief Default constructor. Creates an empty map.
     */
	gmMap() = default;

	/**
     * @brief Removes all locations, tiles, adjacencies, items, and metadata.
     *
     * After this call the map is in the same state as a default-constructed
     * instance.
     */
	void clear();

	// -- Location management ---------------------------------------------------

	/**
     * @brief Creates a new empty location with the given ID.
     *
     * The new location has no tile assignment, no items, no metadata, and no
     * neighbors.
     * @param id Unique identifier for the new location.
     * @throws EDuplicateLocationError if @p id already exists.
     */
	void create_location(LocationId id);

	/**
     * @brief Removes a location and cleans up all related state.
     *
     * The location is unassigned from its tile (if any) and removed from the
     * neighbor lists of all adjacent locations before being deleted.
     * @param id The location to remove.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	void remove_location(LocationId id);

	/**
     * @brief Checks whether a location with the given ID exists.
     * @param id Location identifier to query.
     * @return `true` if the location exists, `false` otherwise.
     */
	bool has_location(LocationId id) const;

	/**
     * @brief Returns all location IDs currently in the map.
     * @return Vector of every LocationId (order unspecified).
     */
	std::vector<LocationId> all_locations() const;

	/**
     * @brief Returns the total number of locations in the map.
     * @return Number of locations.
     */
	std::size_t location_count() const;

	// -- Tile management -------------------------------------------------------

	/**
     * @brief Creates a new empty tile with the given ID.
     * @param id Unique identifier for the new tile.
     * @throws EDuplicateTileError if @p id already exists.
     */
	void create_tile(TileId id);

	/**
     * @brief Removes a tile.
     *
     * All locations that belonged to this tile are left intact but are
     * unassigned (their tile membership is cleared).
     * @param id The tile to remove.
     * @throws EUnknownTileError if @p id does not exist.
     */
	void remove_tile(TileId id);

	/**
     * @brief Checks whether a tile with the given ID exists.
     * @param id Tile identifier to query.
     * @return `true` if the tile exists, `false` otherwise.
     */
	bool has_tile(TileId id) const;

	/**
     * @brief Returns all tile IDs currently in the map.
     * @return Vector of every TileId (order unspecified).
     */
	std::vector<TileId> all_tiles() const;

	/**
     * @brief Returns the total number of tiles in the map.
     * @return Number of tiles.
     */
	std::size_t tile_count() const;

	// -- Location ↔ Tile assignment --------------------------------------------

	/**
     * @brief Assigns a location to a tile.
     *
     * If the location is already assigned to a different tile it is first
     * unassigned from that tile before being added to @p tile.
     * @param loc  Location to assign.
     * @param tile Target tile.
     * @throws EUnknownLocationError if @p loc does not exist.
     * @throws EUnknownTileError     if @p tile does not exist.
     */
	void assign_to_tile(LocationId loc, TileId tile);

	/**
     * @brief Removes a location from its current tile (if any).
     *
     * Has no effect if the location is not currently assigned to any tile.
     * @param loc Location to unassign.
     * @throws EUnknownLocationError if @p loc does not exist.
     */
	void unassign_from_tile(LocationId loc);

	/**
     * @brief Returns the tile a location currently belongs to.
     * @param loc Location to query.
     * @return The TileId if the location is assigned, or `std::nullopt` if not.
     * @throws EUnknownLocationError if @p loc does not exist.
     */
	std::optional<TileId> tile_of(LocationId loc) const;

	/**
     * @brief Returns all locations that belong to a tile.
     * @param tile Tile to query.
     * @return Vector of LocationId values (order unspecified).
     * @throws EUnknownTileError if @p tile does not exist.
     */
	std::vector<LocationId> locations_in_tile(TileId tile) const;

	// -- Adjacency -------------------------------------------------------------

	/**
     * @brief Marks two locations as adjacent (creates a directed or
     *        bidirectional edge).
     *
     * @param a             First location (source of the edge).
     * @param b             Second location (target of the edge).
     * @param bidirectional If `true` (default), both @p a→@p b and @p b→@p a
     *                      edges are created.  If `false`, only @p a→@p b.
     * @throws EUnknownLocationError  if either @p a or @p b does not exist.
     * @throws EInvalidAdjacencyError if @p a == @p b (self-loops not allowed).
     */
	void set_adjacent(LocationId a, LocationId b, bool bidirectional = true);

	/**
     * @brief Removes the adjacency edge between two locations.
     *
     * @param a             First location.
     * @param b             Second location.
     * @param bidirectional If `true` (default), removes both @p a→@p b and
     *                      @p b→@p a.  If `false`, removes only @p a→@p b.
     * @throws EUnknownLocationError if either @p a or @p b does not exist.
     */
	void remove_adjacent(LocationId a, LocationId b, bool bidirectional = true);

	/**
     * @brief Checks whether location @p a has a directed edge to location @p b.
     * @param a Source location.
     * @param b Target location.
     * @return `true` if there is an edge from @p a to @p b.
     * @throws EUnknownLocationError if either @p a or @p b does not exist.
     */
	bool are_adjacent(LocationId a, LocationId b) const;

	/**
     * @brief Returns all locations directly reachable from a given location.
     * @param id Source location.
     * @return Vector of neighbor LocationId values (order unspecified).
     * @throws EUnknownLocationError if @p id does not exist.
     */
	std::vector<LocationId> adjacent_to(LocationId id) const;

	// -- Items -----------------------------------------------------------------

	/**
     * @brief Adds an item to the item list of a location (appended at the end).
     * @param id   Target location.
     * @param item Item to add (copied into the location's internal list).
     * @throws EUnknownLocationError if @p id does not exist.
     */
	void add_item(LocationId id, const ItemT& item);

	/**
     * @brief Removes the item at position @p index from a location's item list.
     *
     * Items after @p index are shifted left by one position.
     * @param id    Target location.
     * @param index Zero-based index of the item to remove.
     * @throws EUnknownLocationError  if @p id does not exist.
     * @throws EInvalidItemIndexError if @p index is out of range.
     */
	void remove_item(LocationId id, std::size_t index);

	/**
     * @brief Returns a const reference to the item list of a location.
     * @param id Target location.
     * @return Const reference to the internal `std::vector<ItemT>`.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	const std::vector<ItemT>& items_at(LocationId id) const;

	/**
     * @brief Removes all items from a location.
     * @param id Target location.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	void clear_items(LocationId id);

	// -- Location metadata -----------------------------------------------------

	/**
     * @brief Sets (or overwrites) a metadata key on a location.
     * @param id    Target location.
     * @param key   Metadata key string.
    * @param value Serializable metadata value.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	void set_location_meta(LocationId id, const std::string& key, const MetadataValue& value);

	/**
     * @brief Retrieves a metadata value from a location.
     * @param id  Target location.
     * @param key Metadata key string.
    * @return Const reference to the stored metadata value.
     * @throws EUnknownLocationError if @p id does not exist.
     * @throws EUnknownMetaKeyError  if @p key is not present.
     */
	const MetadataValue& get_location_meta(LocationId id, const std::string& key) const;

	/**
     * @brief Checks whether a metadata key is present on a location.
     * @param id  Target location.
     * @param key Metadata key string.
     * @return `true` if the key exists, `false` otherwise.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	bool has_location_meta(LocationId id, const std::string& key) const;

	/**
     * @brief Removes a metadata key from a location.
     *
     * Has no effect if the key does not exist.
     * @param id  Target location.
     * @param key Metadata key string to remove.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	void remove_location_meta(LocationId id, const std::string& key);

	/**
     * @brief Returns the full metadata map for a location.
     * @param id Target location.
     * @return Const reference to the location's Metadata map.
     * @throws EUnknownLocationError if @p id does not exist.
     */
	const Metadata& location_metadata(LocationId id) const;

	// -- Tile metadata ---------------------------------------------------------

	/**
     * @brief Sets (or overwrites) a metadata key on a tile.
     * @param id    Target tile.
     * @param key   Metadata key string.
    * @param value Serializable metadata value.
     * @throws EUnknownTileError if @p id does not exist.
     */
	void set_tile_meta(TileId id, const std::string& key, const MetadataValue& value);

	/**
     * @brief Retrieves a metadata value from a tile.
     * @param id  Target tile.
     * @param key Metadata key string.
    * @return Const reference to the stored metadata value.
     * @throws EUnknownTileError    if @p id does not exist.
     * @throws EUnknownMetaKeyError if @p key is not present.
     */
	const MetadataValue& get_tile_meta(TileId id, const std::string& key) const;

	// -- Runtime entity cache (transient, non-persistent) ---------------------

	/**
    * @brief Registers or updates a transient runtime pointer for an external UID.
    *
    * This cache is not part of gmMap persistent state and is intentionally
    * excluded from save/load flows.
    */
	void register_runtime_entity(EntityUid uid, const void* ptr);

	/**
    * @brief Removes a runtime pointer mapping for an external UID.
    */
	void unregister_runtime_entity(EntityUid uid);

	/**
    * @brief Returns the transient runtime pointer for an external UID, if present.
    */
	const void* runtime_entity(EntityUid uid) const;

	/**
    * @brief Clears all transient UID -> pointer runtime mappings.
    */
	void clear_runtime_entity_cache();

	// -- JSON persistence (versioned via gmSave) -------------------------------

	/**
     * @brief Exports the complete map state to a versioned JSON file.
     *
     * @param filepath Path to write the snapshot to.
        * @throws gmSave::EFileWriteError if the file cannot be written.
     */
	void export_snapshot_json(const std::string& filepath) const;

	/**
     * @brief Imports a complete map state from a versioned JSON file.
     *
     * Clears the current map and replaces it with the loaded state.
     *
     * @param filepath Path to read the snapshot from.
        * @throws gmSave::EFileReadError if the file cannot be read.
        * @throws gmSave::EJsonParseError if the JSON is malformed.
        * @throws gmSave::EVersionMismatchError if the version does not match expected (currently v1).
     */
	void import_snapshot_json(const std::string& filepath);

	/**
     * @brief Checks if a tile has a specific metadata key.
     * @param id  Target tile.
     * @param key Metadata key string.
     * @return `true` if the key exists, `false` otherwise.
     * @throws EUnknownTileError if @p id does not exist.
     */
	bool has_tile_meta(TileId id, const std::string& key) const;

	/**
     * @brief Removes a metadata key from a tile.
     *
     * Has no effect if the key does not exist.
     * @param id  Target tile.
     * @param key Metadata key string to remove.
     * @throws EUnknownTileError if @p id does not exist.
     */
	void remove_tile_meta(TileId id, const std::string& key);

	/**
     * @brief Returns the full metadata map for a tile.
     * @param id Target tile.
     * @return Const reference to the tile's Metadata map.
     * @throws EUnknownTileError if @p id does not exist.
     */
	const Metadata& tile_metadata(TileId id) const;

  private:
	// --- Internal record types ------------------------------------------------

	/**
     * @brief Internal storage record for a single location node.
     */
	struct LocationRecord
	{
		std::optional<TileId> tile_id;            ///< Owning tile, if assigned.
		std::vector<ItemT> items;                 ///< Items placed at this location.
		Metadata meta;                            ///< Heterogeneous metadata.
		std::unordered_set<LocationId> neighbors; ///< Outgoing adjacency edges.
	};

	/**
     * @brief Internal storage record for a tile (group of locations).
     */
	struct TileRecord
	{
		std::unordered_set<LocationId> locations; ///< Locations assigned to this tile.
		Metadata meta;                            ///< Heterogeneous metadata.
	};

	// --- Private helpers ------------------------------------------------------

	/**
     * @brief Asserts that a location ID exists; throws EUnknownLocationError otherwise.
     * @param id Location ID to validate.
     */
	void _require_location(LocationId id) const;

	/**
     * @brief Asserts that a tile ID exists; throws EUnknownTileError otherwise.
     * @param id Tile ID to validate.
     */
	void _require_tile(TileId id) const;

	// --- Data members ---------------------------------------------------------

	std::unordered_map<LocationId, LocationRecord> _locations;    ///< All location records.
	std::unordered_map<TileId, TileRecord> _tiles;                ///< All tile records.
	std::unordered_map<EntityUid, const void*> _runtime_entities; ///< Transient UID->pointer cache.
};

// =============================================================================
// Inline template stub implementations
// (Bodies to be filled in subsequent development phases — see PLAN.md)
// =============================================================================

// -- Private helpers -----------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::_require_location(LocationId id) const
{
	if (_locations.find(id) == _locations.end())
	{
		throw EUnknownLocationError("Location " + std::to_string(id) + " does not exist");
	}
}

template <typename ItemT> void gmMap<ItemT>::_require_tile(TileId id) const
{
	if (_tiles.find(id) == _tiles.end())
	{
		throw EUnknownTileError("Tile " + std::to_string(id) + " does not exist");
	}
}

// -- Construction / reset ------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::clear()
{
	_locations.clear();
	_tiles.clear();
	_runtime_entities.clear();
}

// -- Location management -------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::create_location(LocationId id)
{
	if (_locations.find(id) != _locations.end())
	{
		throw EDuplicateLocationError("Location " + std::to_string(id) + " already exists");
	}
	_locations[id] = LocationRecord{};
}

template <typename ItemT> void gmMap<ItemT>::remove_location(LocationId id)
{
	_require_location(id);

	LocationRecord& rec = _locations[id];

	// Unassign from tile if assigned
	if (rec.tile_id.has_value())
	{
		TileId tile_id = rec.tile_id.value();
		_tiles[tile_id].locations.erase(id);
	}

	// Remove this location from all neighbor lists of adjacent locations
	for (LocationId neighbor : rec.neighbors)
	{
		if (_locations.find(neighbor) != _locations.end())
		{
			_locations[neighbor].neighbors.erase(id);
		}
	}

	// Also remove reverse edges: iterate all locations and remove id from their neighbor lists
	for (auto& [other_id, other_rec] : _locations)
	{
		if (other_id != id)
		{
			other_rec.neighbors.erase(id);
		}
	}

	_locations.erase(id);
}

template <typename ItemT> bool gmMap<ItemT>::has_location(LocationId id) const
{
	return _locations.find(id) != _locations.end();
}

template <typename ItemT> std::vector<LocationId> gmMap<ItemT>::all_locations() const
{
	std::vector<LocationId> result;
	result.reserve(_locations.size());
	for (const auto& [id, _] : _locations)
	{
		result.push_back(id);
	}
	return result;
}

template <typename ItemT> std::size_t gmMap<ItemT>::location_count() const
{
	return _locations.size();
}

// -- Tile management -----------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::create_tile(TileId id)
{
	if (_tiles.find(id) != _tiles.end())
	{
		throw EDuplicateTileError("Tile " + std::to_string(id) + " already exists");
	}
	_tiles[id] = TileRecord{};
}

template <typename ItemT> void gmMap<ItemT>::remove_tile(TileId id)
{
	_require_tile(id);

	TileRecord& tile = _tiles[id];
	for (LocationId loc : tile.locations)
	{
		auto loc_it = _locations.find(loc);
		if (loc_it != _locations.end())
		{
			loc_it->second.tile_id.reset();
		}
	}

	_tiles.erase(id);
}

template <typename ItemT> bool gmMap<ItemT>::has_tile(TileId id) const
{
	return _tiles.find(id) != _tiles.end();
}

template <typename ItemT> std::vector<TileId> gmMap<ItemT>::all_tiles() const
{
	std::vector<TileId> result;
	result.reserve(_tiles.size());
	for (const auto& [id, tile] : _tiles)
	{
		(void)tile;
		result.push_back(id);
	}
	return result;
}

template <typename ItemT> std::size_t gmMap<ItemT>::tile_count() const
{
	return _tiles.size();
}

// -- Location ↔ Tile assignment ------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::assign_to_tile(LocationId loc, TileId tile)
{
	_require_location(loc);
	_require_tile(tile);

	LocationRecord& loc_rec = _locations[loc];
	if (loc_rec.tile_id.has_value())
	{
		TileId prev_tile = loc_rec.tile_id.value();
		if (prev_tile == tile)
		{
			return;
		}
		_tiles[prev_tile].locations.erase(loc);
	}

	_tiles[tile].locations.insert(loc);
	loc_rec.tile_id = tile;
}

template <typename ItemT> void gmMap<ItemT>::unassign_from_tile(LocationId loc)
{
	_require_location(loc);

	LocationRecord& loc_rec = _locations[loc];
	if (!loc_rec.tile_id.has_value())
	{
		return;
	}

	TileId tile = loc_rec.tile_id.value();
	auto tile_it = _tiles.find(tile);
	if (tile_it != _tiles.end())
	{
		tile_it->second.locations.erase(loc);
	}
	loc_rec.tile_id.reset();
}

template <typename ItemT> std::optional<TileId> gmMap<ItemT>::tile_of(LocationId loc) const
{
	_require_location(loc);
	return _locations.at(loc).tile_id;
}

template <typename ItemT> std::vector<LocationId> gmMap<ItemT>::locations_in_tile(TileId tile) const
{
	_require_tile(tile);

	const TileRecord& tile_rec = _tiles.at(tile);
	std::vector<LocationId> result;
	result.reserve(tile_rec.locations.size());
	for (LocationId loc : tile_rec.locations)
	{
		result.push_back(loc);
	}
	return result;
}

// -- Adjacency -----------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_adjacent(LocationId a, LocationId b, bool bidirectional)
{
	_require_location(a);
	_require_location(b);

	if (a == b)
	{
		throw EInvalidAdjacencyError("Cannot create self-loop for location " + std::to_string(a));
	}

	_locations[a].neighbors.insert(b);
	if (bidirectional)
	{
		_locations[b].neighbors.insert(a);
	}
}

template <typename ItemT>
void gmMap<ItemT>::remove_adjacent(LocationId a, LocationId b, bool bidirectional)
{
	_require_location(a);
	_require_location(b);

	_locations[a].neighbors.erase(b);
	if (bidirectional)
	{
		_locations[b].neighbors.erase(a);
	}
}

template <typename ItemT> bool gmMap<ItemT>::are_adjacent(LocationId a, LocationId b) const
{
	_require_location(a);
	_require_location(b);

	const auto& neighbors = _locations.at(a).neighbors;
	return neighbors.find(b) != neighbors.end();
}

template <typename ItemT> std::vector<LocationId> gmMap<ItemT>::adjacent_to(LocationId id) const
{
	_require_location(id);

	const auto& neighbors = _locations.at(id).neighbors;
	std::vector<LocationId> result;
	result.reserve(neighbors.size());
	for (LocationId neighbor : neighbors)
	{
		result.push_back(neighbor);
	}
	return result;
}

// -- Items ---------------------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::add_item(LocationId id, const ItemT& item)
{
	_require_location(id);
	_locations[id].items.push_back(item);
}

template <typename ItemT> void gmMap<ItemT>::remove_item(LocationId id, std::size_t index)
{
	_require_location(id);

	std::vector<ItemT>& items = _locations[id].items;
	if (index >= items.size())
	{
		throw EInvalidItemIndexError("Item index " + std::to_string(index) +
		                             " out of range for location " + std::to_string(id));
	}

	items.erase(items.begin() + static_cast<std::ptrdiff_t>(index));
}

template <typename ItemT> const std::vector<ItemT>& gmMap<ItemT>::items_at(LocationId id) const
{
	_require_location(id);
	return _locations.at(id).items;
}

template <typename ItemT> void gmMap<ItemT>::clear_items(LocationId id)
{
	_require_location(id);
	_locations[id].items.clear();
}

// -- Location metadata ---------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_location_meta(LocationId id,
                                     const std::string& key,
                                     const MetadataValue& value)
{
	_require_location(id);
	_locations[id].meta[key] = value;
}

template <typename ItemT>
const MetadataValue& gmMap<ItemT>::get_location_meta(LocationId id, const std::string& key) const
{
	_require_location(id);

	const Metadata& meta = _locations.at(id).meta;
	auto it = meta.find(key);
	if (it == meta.end())
	{
		throw EUnknownMetaKeyError("Location metadata key '" + key + "' not found for location " +
		                           std::to_string(id));
	}
	return it->second;
}

template <typename ItemT>
bool gmMap<ItemT>::has_location_meta(LocationId id, const std::string& key) const
{
	_require_location(id);
	const Metadata& meta = _locations.at(id).meta;
	return meta.find(key) != meta.end();
}

template <typename ItemT>
void gmMap<ItemT>::remove_location_meta(LocationId id, const std::string& key)
{
	_require_location(id);
	_locations[id].meta.erase(key);
}

template <typename ItemT> const Metadata& gmMap<ItemT>::location_metadata(LocationId id) const
{
	_require_location(id);
	return _locations.at(id).meta;
}

// -- Tile metadata -------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_tile_meta(TileId id, const std::string& key, const MetadataValue& value)
{
	_require_tile(id);
	_tiles[id].meta[key] = value;
}

template <typename ItemT>
const MetadataValue& gmMap<ItemT>::get_tile_meta(TileId id, const std::string& key) const
{
	_require_tile(id);

	const Metadata& meta = _tiles.at(id).meta;
	auto it = meta.find(key);
	if (it == meta.end())
	{
		throw EUnknownMetaKeyError("Tile metadata key '" + key + "' not found for tile " +
		                           std::to_string(id));
	}
	return it->second;
}

template <typename ItemT> bool gmMap<ItemT>::has_tile_meta(TileId id, const std::string& key) const
{
	_require_tile(id);
	const Metadata& meta = _tiles.at(id).meta;
	return meta.find(key) != meta.end();
}

template <typename ItemT> void gmMap<ItemT>::remove_tile_meta(TileId id, const std::string& key)
{
	_require_tile(id);
	_tiles[id].meta.erase(key);
}

template <typename ItemT> const Metadata& gmMap<ItemT>::tile_metadata(TileId id) const
{
	_require_tile(id);
	return _tiles.at(id).meta;
}

// -- Runtime entity cache (transient, non-persistent) -------------------------

template <typename ItemT> void gmMap<ItemT>::register_runtime_entity(EntityUid uid, const void* ptr)
{
	_runtime_entities[uid] = ptr;
}

template <typename ItemT> void gmMap<ItemT>::unregister_runtime_entity(EntityUid uid)
{
	_runtime_entities.erase(uid);
}

template <typename ItemT> const void* gmMap<ItemT>::runtime_entity(EntityUid uid) const
{
	auto it = _runtime_entities.find(uid);
	if (it == _runtime_entities.end())
	{
		return nullptr;
	}
	return it->second;
}

template <typename ItemT> void gmMap<ItemT>::clear_runtime_entity_cache()
{
	_runtime_entities.clear();
}

// Helper to serialize a single MetadataValue to JSON
inline nlohmann::json serialize_metadata_value(const MetadataValue& val)
{
	nlohmann::json j = nlohmann::json::object();

	if (std::holds_alternative<std::nullptr_t>(val))
	{
		j["_type"] = "null";
		j["_value"] = nullptr;
	}
	else if (std::holds_alternative<bool>(val))
	{
		j["_type"] = "bool";
		j["_value"] = std::get<bool>(val);
	}
	else if (std::holds_alternative<int64_t>(val))
	{
		j["_type"] = "int64";
		j["_value"] = std::get<int64_t>(val);
	}
	else if (std::holds_alternative<double>(val))
	{
		j["_type"] = "double";
		j["_value"] = std::get<double>(val);
	}
	else if (std::holds_alternative<std::string>(val))
	{
		j["_type"] = "string";
		j["_value"] = std::get<std::string>(val);
	}
	else if (std::holds_alternative<UidRef>(val))
	{
		j["_type"] = "uid_ref";
		j["_value"] = std::get<UidRef>(val).value;
	}
	else if (std::holds_alternative<UidList>(val))
	{
		j["_type"] = "uid_list";
		const auto& list = std::get<UidList>(val);
		j["_value"] = nlohmann::json::array();
		for (const auto& ref : list)
		{
			j["_value"].push_back(ref.value);
		}
	}
	return j;
}

// Helper to deserialize a MetadataValue from JSON
inline MetadataValue deserialize_metadata_value(const nlohmann::json& j)
{
	std::string type = j.at("_type").get<std::string>();

	if (type == "null")
	{
		return nullptr;
	}
	else if (type == "bool")
	{
		return j.at("_value").get<bool>();
	}
	else if (type == "int64")
	{
		return j.at("_value").get<int64_t>();
	}
	else if (type == "double")
	{
		return j.at("_value").get<double>();
	}
	else if (type == "string")
	{
		return j.at("_value").get<std::string>();
	}
	else if (type == "uid_ref")
	{
		return UidRef{j.at("_value").get<EntityUid>()};
	}
	else if (type == "uid_list")
	{
		UidList list;
		for (const auto& item : j.at("_value"))
		{
			list.push_back(UidRef{item.get<EntityUid>()});
		}
		return list;
	}
	return nullptr;
}

template <typename ItemT> void to_json(nlohmann::json& j, const MapSnapshot<ItemT>& snap)
{
	j["location_ids"] = snap.location_ids;
	j["tile_ids"] = snap.tile_ids;
	j["assignments"] = snap.assignments;
	j["adjacency_edges"] = snap.adjacency_edges;
	j["items_by_location"] = snap.items_by_location;

	// Serialize location metadata with explicit MetadataValue conversion
	nlohmann::json loc_meta_json = nlohmann::json::object();
	for (const auto& [loc_id, meta_map] : snap.location_metadata_map)
	{
		nlohmann::json loc_meta_obj = nlohmann::json::object();
		for (const auto& [key, value] : meta_map)
		{
			loc_meta_obj[key] = serialize_metadata_value(value);
		}
		loc_meta_json[std::to_string(loc_id)] = loc_meta_obj;
	}
	j["location_metadata_map"] = loc_meta_json;

	// Serialize tile metadata with explicit MetadataValue conversion
	nlohmann::json tile_meta_json = nlohmann::json::object();
	for (const auto& [tile_id, meta_map] : snap.tile_metadata_map)
	{
		nlohmann::json tile_meta_obj = nlohmann::json::object();
		for (const auto& [key, value] : meta_map)
		{
			tile_meta_obj[key] = serialize_metadata_value(value);
		}
		tile_meta_json[std::to_string(tile_id)] = tile_meta_obj;
	}
	j["tile_metadata_map"] = tile_meta_json;
}

template <typename ItemT> void from_json(const nlohmann::json& j, MapSnapshot<ItemT>& snap)
{
	j.at("location_ids").get_to(snap.location_ids);
	j.at("tile_ids").get_to(snap.tile_ids);
	j.at("assignments").get_to(snap.assignments);
	j.at("adjacency_edges").get_to(snap.adjacency_edges);
	j.at("items_by_location").get_to(snap.items_by_location);

	// Deserialize location metadata
	snap.location_metadata_map.clear();
	const auto& loc_meta_j = j.at("location_metadata_map");
	for (const auto& [loc_id_str, meta_obj] : loc_meta_j.items())
	{
		LocationId loc_id = std::stoul(loc_id_str);
		Metadata meta;
		for (const auto& [key, value_j] : meta_obj.items())
		{
			meta[key] = deserialize_metadata_value(value_j);
		}
		snap.location_metadata_map[loc_id] = meta;
	}

	// Deserialize tile metadata
	snap.tile_metadata_map.clear();
	const auto& tile_meta_j = j.at("tile_metadata_map");
	for (const auto& [tile_id_str, meta_obj] : tile_meta_j.items())
	{
		TileId tile_id = std::stoul(tile_id_str);
		Metadata meta;
		for (const auto& [key, value_j] : meta_obj.items())
		{
			meta[key] = deserialize_metadata_value(value_j);
		}
		snap.tile_metadata_map[tile_id] = meta;
	}
}

// -- JSON persistence implementation -------------------------------------------

template <typename ItemT> void gmMap<ItemT>::export_snapshot_json(const std::string& filepath) const
{
	// Build snapshot from current state
	MapSnapshot<ItemT> snap;

	// Collect location IDs
	snap.location_ids = all_locations();

	// Collect tile IDs
	snap.tile_ids = all_tiles();

	// Collect location-to-tile assignments
	for (LocationId loc : snap.location_ids)
	{
		std::optional<TileId> tile = tile_of(loc);
		if (tile.has_value())
		{
			snap.assignments.push_back({loc, tile.value()});
		}
	}

	// Collect adjacency edges (store all directed edges)
	for (LocationId from : snap.location_ids)
	{
		for (LocationId to : adjacent_to(from))
		{
			snap.adjacency_edges.push_back({from, to});
		}
	}

	// Collect items at each location
	for (LocationId loc : snap.location_ids)
	{
		const std::vector<ItemT>& items = items_at(loc);
		if (!items.empty())
		{
			snap.items_by_location[loc] = items;
		}
	}

	// Collect location metadata
	for (LocationId loc : snap.location_ids)
	{
		const Metadata& meta = location_metadata(loc);
		if (!meta.empty())
		{
			snap.location_metadata_map[loc] = meta;
		}
	}

	// Collect tile metadata
	for (TileId tile : snap.tile_ids)
	{
		const Metadata& meta = tile_metadata(tile);
		if (!meta.empty())
		{
			snap.tile_metadata_map[tile] = meta;
		}
	}

	// Write versioned snapshot to file
	gmSave::save_versioned(filepath, snap, 1U, 2);
}

template <typename ItemT> void gmMap<ItemT>::import_snapshot_json(const std::string& filepath)
{
	// Load versioned snapshot from file
	MapSnapshot<ItemT> snap = gmSave::load_versioned<MapSnapshot<ItemT>>(filepath, 1U);

	// Clear current state
	clear();

	// Recreate locations
	for (LocationId loc : snap.location_ids)
	{
		create_location(loc);
	}

	// Recreate tiles
	for (TileId tile : snap.tile_ids)
	{
		create_tile(tile);
	}

	// Restore location-to-tile assignments
	for (const auto& [loc, tile] : snap.assignments)
	{
		assign_to_tile(loc, tile);
	}

	// Restore adjacency edges
	for (const auto& [from, to] : snap.adjacency_edges)
	{
		// Only create if not already bidirectional
		if (!are_adjacent(from, to))
		{
			set_adjacent(from, to, /*bidirectional=*/false);
		}
	}

	// Restore items
	for (const auto& [loc, items_list] : snap.items_by_location)
	{
		for (const ItemT& item : items_list)
		{
			add_item(loc, item);
		}
	}

	// Restore location metadata
	for (const auto& [loc, meta] : snap.location_metadata_map)
	{
		for (const auto& [key, value] : meta)
		{
			set_location_meta(loc, key, value);
		}
	}

	// Restore tile metadata
	for (const auto& [tile, meta] : snap.tile_metadata_map)
	{
		for (const auto& [key, value] : meta)
		{
			set_tile_meta(tile, key, value);
		}
	}
}

} // namespace gmMap

#endif // GMMAP_GMMAP_HPP
