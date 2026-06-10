#ifndef GMMAP_HPP
#define GMMAP_HPP

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

#include <any>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GameMap {

// --- Type aliases -------------------------------------------------------------

/// @brief Unique identifier for a Location node in the map graph.
using LocationId = uint32_t;

/// @brief Unique identifier for a Tile (named group of locations).
using TileId = uint32_t;

/// @brief Heterogeneous key-value store used for metadata on locations and tiles.
using Metadata = std::unordered_map<std::string, std::any>;

// --- Exceptions ---------------------------------------------------------------

/**
 * @brief Base exception class for all gmMap errors.
 */
class MapError : public std::runtime_error {
public:
    explicit MapError(const std::string& message)
        : std::runtime_error("MapError: " + message) {}
};

/**
 * @brief Thrown when trying to create a Location with an ID that already exists.
 */
class DuplicateLocationError : public MapError {
public:
    explicit DuplicateLocationError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when referencing a LocationId that does not exist in the map.
 */
class UnknownLocationError : public MapError {
public:
    explicit UnknownLocationError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when trying to create a Tile with an ID that already exists.
 */
class DuplicateTileError : public MapError {
public:
    explicit DuplicateTileError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when referencing a TileId that does not exist in the map.
 */
class UnknownTileError : public MapError {
public:
    explicit UnknownTileError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when an adjacency operation violates map invariants
 *        (e.g. self-loops, referencing a non-existent location).
 */
class InvalidAdjacencyError : public MapError {
public:
    explicit InvalidAdjacencyError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when accessing a metadata key that does not exist.
 */
class UnknownMetaKeyError : public MapError {
public:
    explicit UnknownMetaKeyError(const std::string& message)
        : MapError(message) {}
};

/**
 * @brief Thrown when accessing an item at an out-of-range index.
 */
class InvalidItemIndexError : public MapError {
public:
    explicit InvalidItemIndexError(const std::string& message)
        : MapError(message) {}
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
 * - **Metadata** – A heterogeneous `string → any` key-value store attached
 *                  to either a location or a tile.
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
template <typename ItemT>
class gmMap {
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
     * @throws DuplicateLocationError if @p id already exists.
     */
    void create_location(LocationId id);

    /**
     * @brief Removes a location and cleans up all related state.
     *
     * The location is unassigned from its tile (if any) and removed from the
     * neighbor lists of all adjacent locations before being deleted.
     * @param id The location to remove.
     * @throws UnknownLocationError if @p id does not exist.
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
     * @throws DuplicateTileError if @p id already exists.
     */
    void create_tile(TileId id);

    /**
     * @brief Removes a tile.
     *
     * All locations that belonged to this tile are left intact but are
     * unassigned (their tile membership is cleared).
     * @param id The tile to remove.
     * @throws UnknownTileError if @p id does not exist.
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
     * @throws UnknownLocationError if @p loc does not exist.
     * @throws UnknownTileError     if @p tile does not exist.
     */
    void assign_to_tile(LocationId loc, TileId tile);

    /**
     * @brief Removes a location from its current tile (if any).
     *
     * Has no effect if the location is not currently assigned to any tile.
     * @param loc Location to unassign.
     * @throws UnknownLocationError if @p loc does not exist.
     */
    void unassign_from_tile(LocationId loc);

    /**
     * @brief Returns the tile a location currently belongs to.
     * @param loc Location to query.
     * @return The TileId if the location is assigned, or `std::nullopt` if not.
     * @throws UnknownLocationError if @p loc does not exist.
     */
    std::optional<TileId> tile_of(LocationId loc) const;

    /**
     * @brief Returns all locations that belong to a tile.
     * @param tile Tile to query.
     * @return Vector of LocationId values (order unspecified).
     * @throws UnknownTileError if @p tile does not exist.
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
     * @throws UnknownLocationError  if either @p a or @p b does not exist.
     * @throws InvalidAdjacencyError if @p a == @p b (self-loops not allowed).
     */
    void set_adjacent(LocationId a, LocationId b, bool bidirectional = true);

    /**
     * @brief Removes the adjacency edge between two locations.
     *
     * @param a             First location.
     * @param b             Second location.
     * @param bidirectional If `true` (default), removes both @p a→@p b and
     *                      @p b→@p a.  If `false`, removes only @p a→@p b.
     * @throws UnknownLocationError if either @p a or @p b does not exist.
     */
    void remove_adjacent(LocationId a, LocationId b, bool bidirectional = true);

    /**
     * @brief Checks whether location @p a has a directed edge to location @p b.
     * @param a Source location.
     * @param b Target location.
     * @return `true` if there is an edge from @p a to @p b.
     * @throws UnknownLocationError if either @p a or @p b does not exist.
     */
    bool are_adjacent(LocationId a, LocationId b) const;

    /**
     * @brief Returns all locations directly reachable from a given location.
     * @param id Source location.
     * @return Vector of neighbor LocationId values (order unspecified).
     * @throws UnknownLocationError if @p id does not exist.
     */
    std::vector<LocationId> adjacent_to(LocationId id) const;

    // -- Items -----------------------------------------------------------------

    /**
     * @brief Adds an item to the item list of a location (appended at the end).
     * @param id   Target location.
     * @param item Item to add (copied into the location's internal list).
     * @throws UnknownLocationError if @p id does not exist.
     */
    void add_item(LocationId id, const ItemT& item);

    /**
     * @brief Removes the item at position @p index from a location's item list.
     *
     * Items after @p index are shifted left by one position.
     * @param id    Target location.
     * @param index Zero-based index of the item to remove.
     * @throws UnknownLocationError  if @p id does not exist.
     * @throws InvalidItemIndexError if @p index is out of range.
     */
    void remove_item(LocationId id, std::size_t index);

    /**
     * @brief Returns a const reference to the item list of a location.
     * @param id Target location.
     * @return Const reference to the internal `std::vector<ItemT>`.
     * @throws UnknownLocationError if @p id does not exist.
     */
    const std::vector<ItemT>& items_at(LocationId id) const;

    /**
     * @brief Removes all items from a location.
     * @param id Target location.
     * @throws UnknownLocationError if @p id does not exist.
     */
    void clear_items(LocationId id);

    // -- Location metadata -----------------------------------------------------

    /**
     * @brief Sets (or overwrites) a metadata key on a location.
     * @param id    Target location.
     * @param key   Metadata key string.
     * @param value Value to store; any type is accepted via `std::any`.
     * @throws UnknownLocationError if @p id does not exist.
     */
    void set_location_meta(LocationId id,
                           const std::string& key,
                           const std::any& value);

    /**
     * @brief Retrieves a metadata value from a location.
     * @param id  Target location.
     * @param key Metadata key string.
     * @return Const reference to the stored `std::any` value.
     * @throws UnknownLocationError if @p id does not exist.
     * @throws UnknownMetaKeyError  if @p key is not present.
     */
    const std::any& get_location_meta(LocationId id,
                                      const std::string& key) const;

    /**
     * @brief Checks whether a metadata key is present on a location.
     * @param id  Target location.
     * @param key Metadata key string.
     * @return `true` if the key exists, `false` otherwise.
     * @throws UnknownLocationError if @p id does not exist.
     */
    bool has_location_meta(LocationId id, const std::string& key) const;

    /**
     * @brief Removes a metadata key from a location.
     *
     * Has no effect if the key does not exist.
     * @param id  Target location.
     * @param key Metadata key string to remove.
     * @throws UnknownLocationError if @p id does not exist.
     */
    void remove_location_meta(LocationId id, const std::string& key);

    /**
     * @brief Returns the full metadata map for a location.
     * @param id Target location.
     * @return Const reference to the location's Metadata map.
     * @throws UnknownLocationError if @p id does not exist.
     */
    const Metadata& location_metadata(LocationId id) const;

    // -- Tile metadata ---------------------------------------------------------

    /**
     * @brief Sets (or overwrites) a metadata key on a tile.
     * @param id    Target tile.
     * @param key   Metadata key string.
     * @param value Value to store; any type is accepted via `std::any`.
     * @throws UnknownTileError if @p id does not exist.
     */
    void set_tile_meta(TileId id,
                       const std::string& key,
                       const std::any& value);

    /**
     * @brief Retrieves a metadata value from a tile.
     * @param id  Target tile.
     * @param key Metadata key string.
     * @return Const reference to the stored `std::any` value.
     * @throws UnknownTileError    if @p id does not exist.
     * @throws UnknownMetaKeyError if @p key is not present.
     */
    const std::any& get_tile_meta(TileId id, const std::string& key) const;

    /**
     * @brief Checks whether a metadata key is present on a tile.
     * @param id  Target tile.
     * @param key Metadata key string.
     * @return `true` if the key exists, `false` otherwise.
     * @throws UnknownTileError if @p id does not exist.
     */
    bool has_tile_meta(TileId id, const std::string& key) const;

    /**
     * @brief Removes a metadata key from a tile.
     *
     * Has no effect if the key does not exist.
     * @param id  Target tile.
     * @param key Metadata key string to remove.
     * @throws UnknownTileError if @p id does not exist.
     */
    void remove_tile_meta(TileId id, const std::string& key);

    /**
     * @brief Returns the full metadata map for a tile.
     * @param id Target tile.
     * @return Const reference to the tile's Metadata map.
     * @throws UnknownTileError if @p id does not exist.
     */
    const Metadata& tile_metadata(TileId id) const;

private:

    // --- Internal record types ------------------------------------------------

    /**
     * @brief Internal storage record for a single location node.
     */
    struct LocationRecord {
        std::optional<TileId>           tile_id;   ///< Owning tile, if assigned.
        std::vector<ItemT>              items;     ///< Items placed at this location.
        Metadata                        meta;      ///< Heterogeneous metadata.
        std::unordered_set<LocationId>  neighbors; ///< Outgoing adjacency edges.
    };

    /**
     * @brief Internal storage record for a tile (group of locations).
     */
    struct TileRecord {
        std::unordered_set<LocationId>  locations; ///< Locations assigned to this tile.
        Metadata                        meta;      ///< Heterogeneous metadata.
    };

    // --- Private helpers ------------------------------------------------------

    /**
     * @brief Asserts that a location ID exists; throws UnknownLocationError otherwise.
     * @param id Location ID to validate.
     */
    void _require_location(LocationId id) const;

    /**
     * @brief Asserts that a tile ID exists; throws UnknownTileError otherwise.
     * @param id Tile ID to validate.
     */
    void _require_tile(TileId id) const;

    // --- Data members ---------------------------------------------------------

    std::unordered_map<LocationId, LocationRecord>  _locations; ///< All location records.
    std::unordered_map<TileId,     TileRecord>      _tiles;     ///< All tile records.
};


// =============================================================================
// Inline template stub implementations
// (Bodies to be filled in subsequent development phases — see PLAN.md)
// =============================================================================

// -- Private helpers -----------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::_require_location(LocationId id) const
{
    if (_locations.find(id) == _locations.end()) {
        throw UnknownLocationError("Location " + std::to_string(id) + " does not exist");
    }
}

template <typename ItemT>
void gmMap<ItemT>::_require_tile(TileId id) const
{
    if (_tiles.find(id) == _tiles.end()) {
        throw UnknownTileError("Tile " + std::to_string(id) + " does not exist");
    }
}

// -- Construction / reset ------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::clear()
{
    _locations.clear();
    _tiles.clear();
}

// -- Location management -------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::create_location(LocationId id)
{
    if (_locations.find(id) != _locations.end()) {
        throw DuplicateLocationError("Location " + std::to_string(id) + " already exists");
    }
    _locations[id] = LocationRecord{};
}

template <typename ItemT>
void gmMap<ItemT>::remove_location(LocationId id)
{
    _require_location(id);

    LocationRecord& rec = _locations[id];

    // Unassign from tile if assigned
    if (rec.tile_id.has_value()) {
        TileId tile_id = rec.tile_id.value();
        _tiles[tile_id].locations.erase(id);
    }

    // Remove this location from all neighbor lists of adjacent locations
    for (LocationId neighbor : rec.neighbors) {
        if (_locations.find(neighbor) != _locations.end()) {
            _locations[neighbor].neighbors.erase(id);
        }
    }

    // Also remove reverse edges: iterate all locations and remove id from their neighbor lists
    for (auto& [other_id, other_rec] : _locations) {
        if (other_id != id) {
            other_rec.neighbors.erase(id);
        }
    }

    _locations.erase(id);
}

template <typename ItemT>
bool gmMap<ItemT>::has_location(LocationId id) const
{
    return _locations.find(id) != _locations.end();
}

template <typename ItemT>
std::vector<LocationId> gmMap<ItemT>::all_locations() const
{
    std::vector<LocationId> result;
    result.reserve(_locations.size());
    for (const auto& [id, _] : _locations) {
        result.push_back(id);
    }
    return result;
}

template <typename ItemT>
std::size_t gmMap<ItemT>::location_count() const
{
    return _locations.size();
}

// -- Tile management -----------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::create_tile(TileId id)
{
    // TODO
}

template <typename ItemT>
void gmMap<ItemT>::remove_tile(TileId id)
{
    // TODO
}

template <typename ItemT>
bool gmMap<ItemT>::has_tile(TileId id) const
{
    return false; // TODO
}

template <typename ItemT>
std::vector<TileId> gmMap<ItemT>::all_tiles() const
{
    return {}; // TODO
}

template <typename ItemT>
std::size_t gmMap<ItemT>::tile_count() const
{
    return 0; // TODO
}

// -- Location ↔ Tile assignment ------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::assign_to_tile(LocationId loc, TileId tile)
{
    // TODO
}

template <typename ItemT>
void gmMap<ItemT>::unassign_from_tile(LocationId loc)
{
    // TODO
}

template <typename ItemT>
std::optional<TileId> gmMap<ItemT>::tile_of(LocationId loc) const
{
    return std::nullopt; // TODO
}

template <typename ItemT>
std::vector<LocationId> gmMap<ItemT>::locations_in_tile(TileId tile) const
{
    return {}; // TODO
}

// -- Adjacency -----------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_adjacent(LocationId a, LocationId b, bool bidirectional)
{
    // TODO
}

template <typename ItemT>
void gmMap<ItemT>::remove_adjacent(LocationId a, LocationId b, bool bidirectional)
{
    // TODO
}

template <typename ItemT>
bool gmMap<ItemT>::are_adjacent(LocationId a, LocationId b) const
{
    return false; // TODO
}

template <typename ItemT>
std::vector<LocationId> gmMap<ItemT>::adjacent_to(LocationId id) const
{
    return {}; // TODO
}

// -- Items ---------------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::add_item(LocationId id, const ItemT& item)
{
    // TODO
}

template <typename ItemT>
void gmMap<ItemT>::remove_item(LocationId id, std::size_t index)
{
    // TODO
}

template <typename ItemT>
const std::vector<ItemT>& gmMap<ItemT>::items_at(LocationId id) const
{
    static std::vector<ItemT> empty; // TODO - temporary stub return
    return empty;
}

template <typename ItemT>
void gmMap<ItemT>::clear_items(LocationId id)
{
    // TODO
}

// -- Location metadata ---------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_location_meta(LocationId id,
                                     const std::string& key,
                                     const std::any& value)
{
    // TODO
}

template <typename ItemT>
const std::any& gmMap<ItemT>::get_location_meta(LocationId id,
                                                const std::string& key) const
{
    static std::any empty; // TODO - temporary stub return
    return empty;
}

template <typename ItemT>
bool gmMap<ItemT>::has_location_meta(LocationId id, const std::string& key) const
{
    return false; // TODO
}

template <typename ItemT>
void gmMap<ItemT>::remove_location_meta(LocationId id, const std::string& key)
{
    // TODO
}

template <typename ItemT>
const Metadata& gmMap<ItemT>::location_metadata(LocationId id) const
{
    static Metadata empty; // TODO - temporary stub return
    return empty;
}

// -- Tile metadata -------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_tile_meta(TileId id,
                                 const std::string& key,
                                 const std::any& value)
{
    // TODO
}

template <typename ItemT>
const std::any& gmMap<ItemT>::get_tile_meta(TileId id,
                                            const std::string& key) const
{
    static std::any empty; // TODO - temporary stub return
    return empty;
}

template <typename ItemT>
bool gmMap<ItemT>::has_tile_meta(TileId id, const std::string& key) const
{
    return false; // TODO
}

template <typename ItemT>
void gmMap<ItemT>::remove_tile_meta(TileId id, const std::string& key)
{
    // TODO
}

template <typename ItemT>
const Metadata& gmMap<ItemT>::tile_metadata(TileId id) const
{
    static Metadata empty; // TODO - temporary stub return
    return empty;
}

} // namespace GameMap

#endif // GMMAP_HPP
