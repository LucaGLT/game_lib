#ifndef GMMAP_GMMAP_HPP
#define GMMAP_GMMAP_HPP

/**
 * @file gmMap.hpp
 * @brief Generic topology-agnostic game map for tabletop applications.
 *
 * @note Because gmMap is a class template, all method implementations must be
 *       visible at each instantiation site.  They are therefore defined as
 *       inline template methods at the bottom of this header, below the class
 *       declaration.
 *
 * ### Hierarchy (since snapshot v2)
 * @verbatim
 *   Region  --(zones_in_region)-->  Zone  --(locations_in_zone)-->  Location
 *   Location --(neighbors)--> Location   (adjacency graph)
 *   Location --> Items (ItemT)
 *   Location --> ActorsContained (ActorId)
 *   Location --> InteractableObjectsContained (InteractableObjectId)
 * @endverbatim
 *
 * Region and Zone membership are both optional (`std::optional`); a Location may
 * exist without a Zone and a Zone without a Region.
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

/// @brief Unique identifier for a Zone (named group of locations, ex Tile).
using ZoneId = uint32_t;

/// @brief Unique identifier for a Region (named group of zones).
using RegionId = uint32_t;

/// @brief Opaque identifier for an actor contained at a location.
using ActorId = uint64_t;

/// @brief Opaque identifier for an interactable object contained at a location.
using InteractableObjectId = uint64_t;

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

/// @brief Key-value store used for metadata on locations, zones and regions.
using Metadata = std::unordered_map<std::string, MetadataValue>;

// --- Snapshot DTO for JSON persistence ----------------------------------------

/**
 * @brief Serializable snapshot of a gmMap state (schema v2).
 *
 * Includes the full Region/Zone/Location hierarchy, adjacency, items,
 * contained actors and interactables, and per-level metadata.
 *
 * @tparam ItemT The item type stored at locations.
 */
template <typename ItemT> struct MapSnapshot
{
	/// All location IDs currently in the map.
	std::vector<LocationId> location_ids;

	/// All zone IDs currently in the map.
	std::vector<ZoneId> zone_ids;

	/// All region IDs currently in the map.
	std::vector<RegionId> region_ids;

	/// Location-to-zone memberships (location -> zone).
	std::vector<std::pair<LocationId, ZoneId>> location_to_zone;

	/// Zone-to-region memberships (zone -> region).
	std::vector<std::pair<ZoneId, RegionId>> zone_to_region;

	/// Adjacency edges stored as pairs of location IDs.
	std::vector<std::pair<LocationId, LocationId>> adjacency_edges;

	/// Items stored at each location.
	std::unordered_map<LocationId, std::vector<ItemT>> items_by_location;

	/// Actors contained at each location.
	std::unordered_map<LocationId, std::vector<ActorId>> actors_by_location;

	/// Interactable objects contained at each location.
	std::unordered_map<LocationId, std::vector<InteractableObjectId>> interactables_by_location;

	/// Metadata for each location.
	std::unordered_map<LocationId, Metadata> location_metadata_map;

	/// Metadata for each zone.
	std::unordered_map<ZoneId, Metadata> zone_metadata_map;

	/// Metadata for each region.
	std::unordered_map<RegionId, Metadata> region_metadata_map;
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
 * @brief Thrown when trying to create a Zone with an ID that already exists.
 */
class EDuplicateZoneError : public EMapError
{
  public:
	explicit EDuplicateZoneError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when referencing a ZoneId that does not exist in the map.
 */
class EUnknownZoneError : public EMapError
{
  public:
	explicit EUnknownZoneError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when trying to create a Region with an ID that already exists.
 */
class EDuplicateRegionError : public EMapError
{
  public:
	explicit EDuplicateRegionError(const std::string& message) : EMapError(message)
	{
	}
};

/**
 * @brief Thrown when referencing a RegionId that does not exist in the map.
 */
class EUnknownRegionError : public EMapError
{
  public:
	explicit EUnknownRegionError(const std::string& message) : EMapError(message)
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
 * - **Zone**     – A named group of locations (floor, sector…).  Optional.
 * - **Region**   – A named group of zones (continent, district…).  Optional.
 * - **Adjacency**– A directed or bidirectional edge between two locations.
 * - **Item**     – A typed game object placed at a location.
 * - **Actor**        – An opaque external actor contained at a location.
 * - **Interactable** – An opaque external interactable object at a location.
 * - **Metadata** – A serializable `string → MetadataValue` key-value store
 *                  attached to a location, a zone or a region.
 *
 * ### Invariants
 * - A location belongs to at most one zone at a time.
 * - A zone belongs to at most one region at a time.
 * - Every neighbor referenced in an adjacency edge must be a valid location.
 * - When @p bidirectional is `true`, adjacency is kept symmetric.
 * - Removing a location also removes it from its zone and from all neighbor lists.
 * - Removing a zone does not remove its locations; it only ungroups them, and
 *   detaches the zone from its region.
 * - Removing a region does not remove its zones; it only ungroups them.
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
	 * @brief Removes all locations, zones, regions, adjacencies, items,
	 *        actors, interactables and metadata.
	 *
	 * After this call the map is in the same state as a default-constructed
	 * instance.
	 */
	void clear();

	// -- Location management ---------------------------------------------------

	/**
	 * @brief Creates a new empty location with the given ID.
	 * @param id Unique identifier for the new location.
	 * @throws EDuplicateLocationError if @p id already exists.
	 */
	void create_location(LocationId id);

	/**
	 * @brief Removes a location and cleans up all related state.
	 *
	 * The location is unassigned from its zone (if any) and removed from the
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

	// -- Zone management -------------------------------------------------------

	/**
	 * @brief Creates a new empty zone with the given ID.
	 * @param id Unique identifier for the new zone.
	 * @throws EDuplicateZoneError if @p id already exists.
	 */
	void create_zone(ZoneId id);

	/**
	 * @brief Removes a zone.
	 *
	 * All locations that belonged to this zone are left intact but unassigned.
	 * The zone is also detached from its region (if any).
	 * @param id The zone to remove.
	 * @throws EUnknownZoneError if @p id does not exist.
	 */
	void remove_zone(ZoneId id);

	/**
	 * @brief Checks whether a zone with the given ID exists.
	 * @param id Zone identifier to query.
	 * @return `true` if the zone exists, `false` otherwise.
	 */
	bool has_zone(ZoneId id) const;

	/**
	 * @brief Returns all zone IDs currently in the map.
	 * @return Vector of every ZoneId (order unspecified).
	 */
	std::vector<ZoneId> all_zones() const;

	/**
	 * @brief Returns the total number of zones in the map.
	 * @return Number of zones.
	 */
	std::size_t zone_count() const;

	// -- Region management -----------------------------------------------------

	/**
	 * @brief Creates a new empty region with the given ID.
	 * @param id Unique identifier for the new region.
	 * @throws EDuplicateRegionError if @p id already exists.
	 */
	void create_region(RegionId id);

	/**
	 * @brief Removes a region.
	 *
	 * All zones that belonged to this region are left intact but unassigned.
	 * @param id The region to remove.
	 * @throws EUnknownRegionError if @p id does not exist.
	 */
	void remove_region(RegionId id);

	/**
	 * @brief Checks whether a region with the given ID exists.
	 * @param id Region identifier to query.
	 * @return `true` if the region exists, `false` otherwise.
	 */
	bool has_region(RegionId id) const;

	/**
	 * @brief Returns all region IDs currently in the map.
	 * @return Vector of every RegionId (order unspecified).
	 */
	std::vector<RegionId> all_regions() const;

	/**
	 * @brief Returns the total number of regions in the map.
	 * @return Number of regions.
	 */
	std::size_t region_count() const;

	// -- Location ↔ Zone assignment --------------------------------------------

	/**
	 * @brief Assigns a location to a zone.
	 *
	 * If the location is already assigned to a different zone it is first
	 * unassigned from that zone before being added to @p zone.
	 * @param loc  Location to assign.
	 * @param zone Target zone.
	 * @throws EUnknownLocationError if @p loc does not exist.
	 * @throws EUnknownZoneError     if @p zone does not exist.
	 */
	void assign_to_zone(LocationId loc, ZoneId zone);

	/**
	 * @brief Removes a location from its current zone (if any).
	 *
	 * Has no effect if the location is not currently assigned to any zone.
	 * @param loc Location to unassign.
	 * @throws EUnknownLocationError if @p loc does not exist.
	 */
	void unassign_from_zone(LocationId loc);

	/**
	 * @brief Returns the zone a location currently belongs to.
	 * @param loc Location to query.
	 * @return The ZoneId if the location is assigned, or `std::nullopt` if not.
	 * @throws EUnknownLocationError if @p loc does not exist.
	 */
	std::optional<ZoneId> zone_of(LocationId loc) const;

	/**
	 * @brief Returns all locations that belong to a zone.
	 * @param zone Zone to query.
	 * @return Vector of LocationId values (order unspecified).
	 * @throws EUnknownZoneError if @p zone does not exist.
	 */
	std::vector<LocationId> locations_in_zone(ZoneId zone) const;

	// -- Zone ↔ Region assignment ----------------------------------------------

	/**
	 * @brief Assigns a zone to a region.
	 *
	 * If the zone is already assigned to a different region it is first
	 * unassigned from that region before being added to @p region.
	 * @param zone   Zone to assign.
	 * @param region Target region.
	 * @throws EUnknownZoneError   if @p zone does not exist.
	 * @throws EUnknownRegionError if @p region does not exist.
	 */
	void assign_zone_to_region(ZoneId zone, RegionId region);

	/**
	 * @brief Removes a zone from its current region (if any).
	 *
	 * Has no effect if the zone is not currently assigned to any region.
	 * @param zone Zone to unassign.
	 * @throws EUnknownZoneError if @p zone does not exist.
	 */
	void unassign_zone_from_region(ZoneId zone);

	/**
	 * @brief Returns the region a zone currently belongs to.
	 * @param zone Zone to query.
	 * @return The RegionId if the zone is assigned, or `std::nullopt` if not.
	 * @throws EUnknownZoneError if @p zone does not exist.
	 */
	std::optional<RegionId> region_of(ZoneId zone) const;

	/**
	 * @brief Returns all zones that belong to a region.
	 * @param region Region to query.
	 * @return Vector of ZoneId values (order unspecified).
	 * @throws EUnknownRegionError if @p region does not exist.
	 */
	std::vector<ZoneId> zones_in_region(RegionId region) const;

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

	// -- Contained actors ------------------------------------------------------

	/**
	 * @brief Places an actor at a location.
	 *
	 * Idempotent: placing an actor that is already present has no effect.
	 * @param id    Target location.
	 * @param actor Opaque actor identifier.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void place_actor(LocationId id, ActorId actor);

	/**
	 * @brief Removes an actor from a location.
	 *
	 * Has no effect if the actor is not present at the location.
	 * @param id    Target location.
	 * @param actor Opaque actor identifier.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void remove_actor(LocationId id, ActorId actor);

	/**
	 * @brief Checks whether an actor is present at a location.
	 * @param id    Target location.
	 * @param actor Opaque actor identifier.
	 * @return `true` if the actor is present, `false` otherwise.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	bool has_actor(LocationId id, ActorId actor) const;

	/**
	 * @brief Returns all actors contained at a location.
	 * @param id Target location.
	 * @return Vector of ActorId values (order unspecified).
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	std::vector<ActorId> actors_at(LocationId id) const;

	/**
	 * @brief Removes all actors from a location.
	 * @param id Target location.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void clear_actors(LocationId id);

	// -- Contained interactable objects ----------------------------------------

	/**
	 * @brief Places an interactable object at a location.
	 *
	 * Idempotent: placing an object that is already present has no effect.
	 * @param id     Target location.
	 * @param object Opaque interactable object identifier.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void place_interactable(LocationId id, InteractableObjectId object);

	/**
	 * @brief Removes an interactable object from a location.
	 *
	 * Has no effect if the object is not present at the location.
	 * @param id     Target location.
	 * @param object Opaque interactable object identifier.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void remove_interactable(LocationId id, InteractableObjectId object);

	/**
	 * @brief Checks whether an interactable object is present at a location.
	 * @param id     Target location.
	 * @param object Opaque interactable object identifier.
	 * @return `true` if the object is present, `false` otherwise.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	bool has_interactable(LocationId id, InteractableObjectId object) const;

	/**
	 * @brief Returns all interactable objects contained at a location.
	 * @param id Target location.
	 * @return Vector of InteractableObjectId values (order unspecified).
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	std::vector<InteractableObjectId> interactables_at(LocationId id) const;

	/**
	 * @brief Removes all interactable objects from a location.
	 * @param id Target location.
	 * @throws EUnknownLocationError if @p id does not exist.
	 */
	void clear_interactables(LocationId id);

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

	// -- Zone metadata ---------------------------------------------------------

	/**
	 * @brief Sets (or overwrites) a metadata key on a zone.
	 * @param id    Target zone.
	 * @param key   Metadata key string.
	 * @param value Serializable metadata value.
	 * @throws EUnknownZoneError if @p id does not exist.
	 */
	void set_zone_meta(ZoneId id, const std::string& key, const MetadataValue& value);

	/**
	 * @brief Retrieves a metadata value from a zone.
	 * @param id  Target zone.
	 * @param key Metadata key string.
	 * @return Const reference to the stored metadata value.
	 * @throws EUnknownZoneError    if @p id does not exist.
	 * @throws EUnknownMetaKeyError if @p key is not present.
	 */
	const MetadataValue& get_zone_meta(ZoneId id, const std::string& key) const;

	/**
	 * @brief Checks if a zone has a specific metadata key.
	 * @param id  Target zone.
	 * @param key Metadata key string.
	 * @return `true` if the key exists, `false` otherwise.
	 * @throws EUnknownZoneError if @p id does not exist.
	 */
	bool has_zone_meta(ZoneId id, const std::string& key) const;

	/**
	 * @brief Removes a metadata key from a zone.
	 *
	 * Has no effect if the key does not exist.
	 * @param id  Target zone.
	 * @param key Metadata key string to remove.
	 * @throws EUnknownZoneError if @p id does not exist.
	 */
	void remove_zone_meta(ZoneId id, const std::string& key);

	/**
	 * @brief Returns the full metadata map for a zone.
	 * @param id Target zone.
	 * @return Const reference to the zone's Metadata map.
	 * @throws EUnknownZoneError if @p id does not exist.
	 */
	const Metadata& zone_metadata(ZoneId id) const;

	// -- Region metadata -------------------------------------------------------

	/**
	 * @brief Sets (or overwrites) a metadata key on a region.
	 * @param id    Target region.
	 * @param key   Metadata key string.
	 * @param value Serializable metadata value.
	 * @throws EUnknownRegionError if @p id does not exist.
	 */
	void set_region_meta(RegionId id, const std::string& key, const MetadataValue& value);

	/**
	 * @brief Retrieves a metadata value from a region.
	 * @param id  Target region.
	 * @param key Metadata key string.
	 * @return Const reference to the stored metadata value.
	 * @throws EUnknownRegionError  if @p id does not exist.
	 * @throws EUnknownMetaKeyError if @p key is not present.
	 */
	const MetadataValue& get_region_meta(RegionId id, const std::string& key) const;

	/**
	 * @brief Checks if a region has a specific metadata key.
	 * @param id  Target region.
	 * @param key Metadata key string.
	 * @return `true` if the key exists, `false` otherwise.
	 * @throws EUnknownRegionError if @p id does not exist.
	 */
	bool has_region_meta(RegionId id, const std::string& key) const;

	/**
	 * @brief Removes a metadata key from a region.
	 *
	 * Has no effect if the key does not exist.
	 * @param id  Target region.
	 * @param key Metadata key string to remove.
	 * @throws EUnknownRegionError if @p id does not exist.
	 */
	void remove_region_meta(RegionId id, const std::string& key);

	/**
	 * @brief Returns the full metadata map for a region.
	 * @param id Target region.
	 * @return Const reference to the region's Metadata map.
	 * @throws EUnknownRegionError if @p id does not exist.
	 */
	const Metadata& region_metadata(RegionId id) const;

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
	 * @brief Exports the complete map state to a versioned JSON file (schema v2).
	 *
	 * @param filepath Path to write the snapshot to.
	 * @throws gmSave::EFileWriteError if the file cannot be written.
	 */
	void export_snapshot_json(const std::string& filepath) const;

	/**
	 * @brief Imports a complete map state from a versioned JSON file.
	 *
	 * Clears the current map and replaces it with the loaded state.  Both
	 * schema v1 (legacy Tile model) and v2 (Region/Zone/Actors/Interactables)
	 * files are accepted; v1 files are migrated transparently (Tile → Zone,
	 * no regions, empty actor/interactable containers).
	 *
	 * @param filepath Path to read the snapshot from.
	 * @throws gmSave::EFileReadError  if the file cannot be read.
	 * @throws gmSave::EJsonParseError if the JSON is malformed.
	 */
	void import_snapshot_json(const std::string& filepath);

  private:
	// --- Internal record types ------------------------------------------------

	/**
	 * @brief Internal storage record for a single location node.
	 */
	struct LocationRecord
	{
		std::optional<ZoneId> zone_id; ///< Owning zone, if assigned.
		std::vector<ItemT> items;      ///< Items placed at this location.
		std::unordered_set<ActorId> actors;                  ///< Contained actors.
		std::unordered_set<InteractableObjectId> interactables; ///< Contained interactables.
		Metadata meta;                            ///< Heterogeneous metadata.
		std::unordered_set<LocationId> neighbors; ///< Outgoing adjacency edges.
	};

	/**
	 * @brief Internal storage record for a zone (group of locations).
	 */
	struct ZoneRecord
	{
		std::optional<RegionId> region_id;        ///< Owning region, if assigned.
		std::unordered_set<LocationId> locations; ///< Locations assigned to this zone.
		Metadata meta;                            ///< Heterogeneous metadata.
	};

	/**
	 * @brief Internal storage record for a region (group of zones).
	 */
	struct RegionRecord
	{
		std::unordered_set<ZoneId> zones; ///< Zones assigned to this region.
		Metadata meta;                    ///< Heterogeneous metadata.
	};

	// --- Private helpers ------------------------------------------------------

	/**
	 * @brief Asserts that a location ID exists; throws EUnknownLocationError otherwise.
	 * @param id Location ID to validate.
	 */
	void _require_location(LocationId id) const;

	/**
	 * @brief Asserts that a zone ID exists; throws EUnknownZoneError otherwise.
	 * @param id Zone ID to validate.
	 */
	void _require_zone(ZoneId id) const;

	/**
	 * @brief Asserts that a region ID exists; throws EUnknownRegionError otherwise.
	 * @param id Region ID to validate.
	 */
	void _require_region(RegionId id) const;

	// --- Data members ---------------------------------------------------------

	std::unordered_map<LocationId, LocationRecord> _locations;    ///< All location records.
	std::unordered_map<ZoneId, ZoneRecord> _zones;                ///< All zone records.
	std::unordered_map<RegionId, RegionRecord> _regions;          ///< All region records.
	std::unordered_map<EntityUid, const void*> _runtime_entities; ///< Transient UID->pointer cache.
};

// =============================================================================
// Inline template implementations
// =============================================================================

// -- Private helpers -----------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::_require_location(LocationId id) const
{
	if (_locations.find(id) == _locations.end())
	{
		throw EUnknownLocationError("Location " + std::to_string(id) + " does not exist");
	}
}

template <typename ItemT> void gmMap<ItemT>::_require_zone(ZoneId id) const
{
	if (_zones.find(id) == _zones.end())
	{
		throw EUnknownZoneError("Zone " + std::to_string(id) + " does not exist");
	}
}

template <typename ItemT> void gmMap<ItemT>::_require_region(RegionId id) const
{
	if (_regions.find(id) == _regions.end())
	{
		throw EUnknownRegionError("Region " + std::to_string(id) + " does not exist");
	}
}

// -- Construction / reset ------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::clear()
{
	_locations.clear();
	_zones.clear();
	_regions.clear();
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

	// Unassign from zone if assigned
	if (rec.zone_id.has_value())
	{
		ZoneId zone_id = rec.zone_id.value();
		_zones[zone_id].locations.erase(id);
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

// -- Zone management -----------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::create_zone(ZoneId id)
{
	if (_zones.find(id) != _zones.end())
	{
		throw EDuplicateZoneError("Zone " + std::to_string(id) + " already exists");
	}
	_zones[id] = ZoneRecord{};
}

template <typename ItemT> void gmMap<ItemT>::remove_zone(ZoneId id)
{
	_require_zone(id);

	ZoneRecord& zone = _zones[id];

	// Ungroup all member locations
	for (LocationId loc : zone.locations)
	{
		auto loc_it = _locations.find(loc);
		if (loc_it != _locations.end())
		{
			loc_it->second.zone_id.reset();
		}
	}

	// Detach from owning region (if any)
	if (zone.region_id.has_value())
	{
		auto region_it = _regions.find(zone.region_id.value());
		if (region_it != _regions.end())
		{
			region_it->second.zones.erase(id);
		}
	}

	_zones.erase(id);
}

template <typename ItemT> bool gmMap<ItemT>::has_zone(ZoneId id) const
{
	return _zones.find(id) != _zones.end();
}

template <typename ItemT> std::vector<ZoneId> gmMap<ItemT>::all_zones() const
{
	std::vector<ZoneId> result;
	result.reserve(_zones.size());
	for (const auto& [id, zone] : _zones)
	{
		(void)zone;
		result.push_back(id);
	}
	return result;
}

template <typename ItemT> std::size_t gmMap<ItemT>::zone_count() const
{
	return _zones.size();
}

// -- Region management ---------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::create_region(RegionId id)
{
	if (_regions.find(id) != _regions.end())
	{
		throw EDuplicateRegionError("Region " + std::to_string(id) + " already exists");
	}
	_regions[id] = RegionRecord{};
}

template <typename ItemT> void gmMap<ItemT>::remove_region(RegionId id)
{
	_require_region(id);

	RegionRecord& region = _regions[id];
	for (ZoneId zone : region.zones)
	{
		auto zone_it = _zones.find(zone);
		if (zone_it != _zones.end())
		{
			zone_it->second.region_id.reset();
		}
	}

	_regions.erase(id);
}

template <typename ItemT> bool gmMap<ItemT>::has_region(RegionId id) const
{
	return _regions.find(id) != _regions.end();
}

template <typename ItemT> std::vector<RegionId> gmMap<ItemT>::all_regions() const
{
	std::vector<RegionId> result;
	result.reserve(_regions.size());
	for (const auto& [id, region] : _regions)
	{
		(void)region;
		result.push_back(id);
	}
	return result;
}

template <typename ItemT> std::size_t gmMap<ItemT>::region_count() const
{
	return _regions.size();
}

// -- Location ↔ Zone assignment ------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::assign_to_zone(LocationId loc, ZoneId zone)
{
	_require_location(loc);
	_require_zone(zone);

	LocationRecord& loc_rec = _locations[loc];
	if (loc_rec.zone_id.has_value())
	{
		ZoneId prev_zone = loc_rec.zone_id.value();
		if (prev_zone == zone)
		{
			return;
		}
		_zones[prev_zone].locations.erase(loc);
	}

	_zones[zone].locations.insert(loc);
	loc_rec.zone_id = zone;
}

template <typename ItemT> void gmMap<ItemT>::unassign_from_zone(LocationId loc)
{
	_require_location(loc);

	LocationRecord& loc_rec = _locations[loc];
	if (!loc_rec.zone_id.has_value())
	{
		return;
	}

	ZoneId zone = loc_rec.zone_id.value();
	auto zone_it = _zones.find(zone);
	if (zone_it != _zones.end())
	{
		zone_it->second.locations.erase(loc);
	}
	loc_rec.zone_id.reset();
}

template <typename ItemT> std::optional<ZoneId> gmMap<ItemT>::zone_of(LocationId loc) const
{
	_require_location(loc);
	return _locations.at(loc).zone_id;
}

template <typename ItemT> std::vector<LocationId> gmMap<ItemT>::locations_in_zone(ZoneId zone) const
{
	_require_zone(zone);

	const ZoneRecord& zone_rec = _zones.at(zone);
	std::vector<LocationId> result;
	result.reserve(zone_rec.locations.size());
	for (LocationId loc : zone_rec.locations)
	{
		result.push_back(loc);
	}
	return result;
}

// -- Zone ↔ Region assignment --------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::assign_zone_to_region(ZoneId zone, RegionId region)
{
	_require_zone(zone);
	_require_region(region);

	ZoneRecord& zone_rec = _zones[zone];
	if (zone_rec.region_id.has_value())
	{
		RegionId prev_region = zone_rec.region_id.value();
		if (prev_region == region)
		{
			return;
		}
		_regions[prev_region].zones.erase(zone);
	}

	_regions[region].zones.insert(zone);
	zone_rec.region_id = region;
}

template <typename ItemT> void gmMap<ItemT>::unassign_zone_from_region(ZoneId zone)
{
	_require_zone(zone);

	ZoneRecord& zone_rec = _zones[zone];
	if (!zone_rec.region_id.has_value())
	{
		return;
	}

	RegionId region = zone_rec.region_id.value();
	auto region_it = _regions.find(region);
	if (region_it != _regions.end())
	{
		region_it->second.zones.erase(zone);
	}
	zone_rec.region_id.reset();
}

template <typename ItemT> std::optional<RegionId> gmMap<ItemT>::region_of(ZoneId zone) const
{
	_require_zone(zone);
	return _zones.at(zone).region_id;
}

template <typename ItemT> std::vector<ZoneId> gmMap<ItemT>::zones_in_region(RegionId region) const
{
	_require_region(region);

	const RegionRecord& region_rec = _regions.at(region);
	std::vector<ZoneId> result;
	result.reserve(region_rec.zones.size());
	for (ZoneId zone : region_rec.zones)
	{
		result.push_back(zone);
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

// -- Contained actors ----------------------------------------------------------

template <typename ItemT> void gmMap<ItemT>::place_actor(LocationId id, ActorId actor)
{
	_require_location(id);
	_locations[id].actors.insert(actor);
}

template <typename ItemT> void gmMap<ItemT>::remove_actor(LocationId id, ActorId actor)
{
	_require_location(id);
	_locations[id].actors.erase(actor);
}

template <typename ItemT> bool gmMap<ItemT>::has_actor(LocationId id, ActorId actor) const
{
	_require_location(id);
	const auto& actors = _locations.at(id).actors;
	return actors.find(actor) != actors.end();
}

template <typename ItemT> std::vector<ActorId> gmMap<ItemT>::actors_at(LocationId id) const
{
	_require_location(id);

	const auto& actors = _locations.at(id).actors;
	std::vector<ActorId> result;
	result.reserve(actors.size());
	for (ActorId actor : actors)
	{
		result.push_back(actor);
	}
	return result;
}

template <typename ItemT> void gmMap<ItemT>::clear_actors(LocationId id)
{
	_require_location(id);
	_locations[id].actors.clear();
}

// -- Contained interactable objects --------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::place_interactable(LocationId id, InteractableObjectId object)
{
	_require_location(id);
	_locations[id].interactables.insert(object);
}

template <typename ItemT>
void gmMap<ItemT>::remove_interactable(LocationId id, InteractableObjectId object)
{
	_require_location(id);
	_locations[id].interactables.erase(object);
}

template <typename ItemT>
bool gmMap<ItemT>::has_interactable(LocationId id, InteractableObjectId object) const
{
	_require_location(id);
	const auto& interactables = _locations.at(id).interactables;
	return interactables.find(object) != interactables.end();
}

template <typename ItemT>
std::vector<InteractableObjectId> gmMap<ItemT>::interactables_at(LocationId id) const
{
	_require_location(id);

	const auto& interactables = _locations.at(id).interactables;
	std::vector<InteractableObjectId> result;
	result.reserve(interactables.size());
	for (InteractableObjectId object : interactables)
	{
		result.push_back(object);
	}
	return result;
}

template <typename ItemT> void gmMap<ItemT>::clear_interactables(LocationId id)
{
	_require_location(id);
	_locations[id].interactables.clear();
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

// -- Zone metadata -------------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_zone_meta(ZoneId id, const std::string& key, const MetadataValue& value)
{
	_require_zone(id);
	_zones[id].meta[key] = value;
}

template <typename ItemT>
const MetadataValue& gmMap<ItemT>::get_zone_meta(ZoneId id, const std::string& key) const
{
	_require_zone(id);

	const Metadata& meta = _zones.at(id).meta;
	auto it = meta.find(key);
	if (it == meta.end())
	{
		throw EUnknownMetaKeyError("Zone metadata key '" + key + "' not found for zone " +
		                           std::to_string(id));
	}
	return it->second;
}

template <typename ItemT> bool gmMap<ItemT>::has_zone_meta(ZoneId id, const std::string& key) const
{
	_require_zone(id);
	const Metadata& meta = _zones.at(id).meta;
	return meta.find(key) != meta.end();
}

template <typename ItemT> void gmMap<ItemT>::remove_zone_meta(ZoneId id, const std::string& key)
{
	_require_zone(id);
	_zones[id].meta.erase(key);
}

template <typename ItemT> const Metadata& gmMap<ItemT>::zone_metadata(ZoneId id) const
{
	_require_zone(id);
	return _zones.at(id).meta;
}

// -- Region metadata -----------------------------------------------------------

template <typename ItemT>
void gmMap<ItemT>::set_region_meta(RegionId id, const std::string& key, const MetadataValue& value)
{
	_require_region(id);
	_regions[id].meta[key] = value;
}

template <typename ItemT>
const MetadataValue& gmMap<ItemT>::get_region_meta(RegionId id, const std::string& key) const
{
	_require_region(id);

	const Metadata& meta = _regions.at(id).meta;
	auto it = meta.find(key);
	if (it == meta.end())
	{
		throw EUnknownMetaKeyError("Region metadata key '" + key + "' not found for region " +
		                           std::to_string(id));
	}
	return it->second;
}

template <typename ItemT>
bool gmMap<ItemT>::has_region_meta(RegionId id, const std::string& key) const
{
	_require_region(id);
	const Metadata& meta = _regions.at(id).meta;
	return meta.find(key) != meta.end();
}

template <typename ItemT> void gmMap<ItemT>::remove_region_meta(RegionId id, const std::string& key)
{
	_require_region(id);
	_regions[id].meta.erase(key);
}

template <typename ItemT> const Metadata& gmMap<ItemT>::region_metadata(RegionId id) const
{
	_require_region(id);
	return _regions.at(id).meta;
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

// -- Metadata (de)serialization helpers ----------------------------------------

/// @brief Serializes a single MetadataValue to a typed JSON object.
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

/// @brief Deserializes a MetadataValue from a typed JSON object.
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

/// @brief Serializes a metadata map keyed by numeric ID into a JSON object.
template <typename KeyT>
inline nlohmann::json serialize_metadata_map(const std::unordered_map<KeyT, Metadata>& meta_map)
{
	nlohmann::json out = nlohmann::json::object();
	for (const auto& [id, meta] : meta_map)
	{
		nlohmann::json meta_obj = nlohmann::json::object();
		for (const auto& [key, value] : meta)
		{
			meta_obj[key] = serialize_metadata_value(value);
		}
		out[std::to_string(id)] = meta_obj;
	}
	return out;
}

/// @brief Deserializes a metadata map keyed by numeric ID from a JSON object.
template <typename KeyT>
inline void deserialize_metadata_map(const nlohmann::json& j,
                                     std::unordered_map<KeyT, Metadata>& meta_map)
{
	meta_map.clear();
	for (const auto& [id_str, meta_obj] : j.items())
	{
		KeyT id = static_cast<KeyT>(std::stoul(id_str));
		Metadata meta;
		for (const auto& [key, value_j] : meta_obj.items())
		{
			meta[key] = deserialize_metadata_value(value_j);
		}
		meta_map[id] = meta;
	}
}

// -- Snapshot (de)serialization (schema v2) ------------------------------------

template <typename ItemT> void to_json(nlohmann::json& j, const MapSnapshot<ItemT>& snap)
{
	j["location_ids"] = snap.location_ids;
	j["zone_ids"] = snap.zone_ids;
	j["region_ids"] = snap.region_ids;
	j["location_to_zone"] = snap.location_to_zone;
	j["zone_to_region"] = snap.zone_to_region;
	j["adjacency_edges"] = snap.adjacency_edges;
	j["items_by_location"] = snap.items_by_location;
	j["actors_by_location"] = snap.actors_by_location;
	j["interactables_by_location"] = snap.interactables_by_location;
	j["location_metadata_map"] = serialize_metadata_map(snap.location_metadata_map);
	j["zone_metadata_map"] = serialize_metadata_map(snap.zone_metadata_map);
	j["region_metadata_map"] = serialize_metadata_map(snap.region_metadata_map);
}

template <typename ItemT> void from_json(const nlohmann::json& j, MapSnapshot<ItemT>& snap)
{
	// Shared between v1 and v2
	j.at("location_ids").get_to(snap.location_ids);
	j.at("adjacency_edges").get_to(snap.adjacency_edges);
	j.at("items_by_location").get_to(snap.items_by_location);

	// Zones: v2 "zone_ids" / v1 "tile_ids"
	if (j.contains("zone_ids"))
	{
		j.at("zone_ids").get_to(snap.zone_ids);
	}
	else if (j.contains("tile_ids"))
	{
		j.at("tile_ids").get_to(snap.zone_ids);
	}
	else
	{
		snap.zone_ids.clear();
	}

	// Regions: v2 only
	if (j.contains("region_ids"))
	{
		j.at("region_ids").get_to(snap.region_ids);
	}
	else
	{
		snap.region_ids.clear();
	}

	// Location -> zone: v2 "location_to_zone" / v1 "assignments"
	if (j.contains("location_to_zone"))
	{
		j.at("location_to_zone").get_to(snap.location_to_zone);
	}
	else if (j.contains("assignments"))
	{
		j.at("assignments").get_to(snap.location_to_zone);
	}
	else
	{
		snap.location_to_zone.clear();
	}

	// Zone -> region: v2 only
	if (j.contains("zone_to_region"))
	{
		j.at("zone_to_region").get_to(snap.zone_to_region);
	}
	else
	{
		snap.zone_to_region.clear();
	}

	// Contained actors / interactables: v2 only
	if (j.contains("actors_by_location"))
	{
		j.at("actors_by_location").get_to(snap.actors_by_location);
	}
	else
	{
		snap.actors_by_location.clear();
	}

	if (j.contains("interactables_by_location"))
	{
		j.at("interactables_by_location").get_to(snap.interactables_by_location);
	}
	else
	{
		snap.interactables_by_location.clear();
	}

	// Location metadata: shared key
	deserialize_metadata_map(j.at("location_metadata_map"), snap.location_metadata_map);

	// Zone metadata: v2 "zone_metadata_map" / v1 "tile_metadata_map"
	if (j.contains("zone_metadata_map"))
	{
		deserialize_metadata_map(j.at("zone_metadata_map"), snap.zone_metadata_map);
	}
	else if (j.contains("tile_metadata_map"))
	{
		deserialize_metadata_map(j.at("tile_metadata_map"), snap.zone_metadata_map);
	}
	else
	{
		snap.zone_metadata_map.clear();
	}

	// Region metadata: v2 only
	if (j.contains("region_metadata_map"))
	{
		deserialize_metadata_map(j.at("region_metadata_map"), snap.region_metadata_map);
	}
	else
	{
		snap.region_metadata_map.clear();
	}
}

// -- JSON persistence implementation -------------------------------------------

template <typename ItemT> void gmMap<ItemT>::export_snapshot_json(const std::string& filepath) const
{
	MapSnapshot<ItemT> snap;

	snap.location_ids = all_locations();
	snap.zone_ids = all_zones();
	snap.region_ids = all_regions();

	// Location-to-zone memberships
	for (LocationId loc : snap.location_ids)
	{
		std::optional<ZoneId> zone = zone_of(loc);
		if (zone.has_value())
		{
			snap.location_to_zone.push_back({loc, zone.value()});
		}
	}

	// Zone-to-region memberships
	for (ZoneId zone : snap.zone_ids)
	{
		std::optional<RegionId> region = region_of(zone);
		if (region.has_value())
		{
			snap.zone_to_region.push_back({zone, region.value()});
		}
	}

	// Adjacency edges (store all directed edges)
	for (LocationId from : snap.location_ids)
	{
		for (LocationId to : adjacent_to(from))
		{
			snap.adjacency_edges.push_back({from, to});
		}
	}

	// Items at each location
	for (LocationId loc : snap.location_ids)
	{
		const std::vector<ItemT>& items = items_at(loc);
		if (!items.empty())
		{
			snap.items_by_location[loc] = items;
		}
	}

	// Actors at each location
	for (LocationId loc : snap.location_ids)
	{
		std::vector<ActorId> actors = actors_at(loc);
		if (!actors.empty())
		{
			snap.actors_by_location[loc] = actors;
		}
	}

	// Interactables at each location
	for (LocationId loc : snap.location_ids)
	{
		std::vector<InteractableObjectId> interactables = interactables_at(loc);
		if (!interactables.empty())
		{
			snap.interactables_by_location[loc] = interactables;
		}
	}

	// Location metadata
	for (LocationId loc : snap.location_ids)
	{
		const Metadata& meta = location_metadata(loc);
		if (!meta.empty())
		{
			snap.location_metadata_map[loc] = meta;
		}
	}

	// Zone metadata
	for (ZoneId zone : snap.zone_ids)
	{
		const Metadata& meta = zone_metadata(zone);
		if (!meta.empty())
		{
			snap.zone_metadata_map[zone] = meta;
		}
	}

	// Region metadata
	for (RegionId region : snap.region_ids)
	{
		const Metadata& meta = region_metadata(region);
		if (!meta.empty())
		{
			snap.region_metadata_map[region] = meta;
		}
	}

	gmSave::save_versioned(filepath, snap, 2U, 2);
}

template <typename ItemT> void gmMap<ItemT>::import_snapshot_json(const std::string& filepath)
{
	// Detect the on-disk schema version; v1 files are migrated transparently.
	std::optional<uint32_t> version = gmSave::peek_version(filepath);
	uint32_t found_version = version.value_or(2U);

	MapSnapshot<ItemT> snap = gmSave::load_versioned<MapSnapshot<ItemT>>(filepath, found_version);

	clear();

	// Recreate regions, zones and locations
	for (RegionId region : snap.region_ids)
	{
		create_region(region);
	}
	for (ZoneId zone : snap.zone_ids)
	{
		create_zone(zone);
	}
	for (LocationId loc : snap.location_ids)
	{
		create_location(loc);
	}

	// Restore zone-to-region memberships
	for (const auto& [zone, region] : snap.zone_to_region)
	{
		assign_zone_to_region(zone, region);
	}

	// Restore location-to-zone memberships
	for (const auto& [loc, zone] : snap.location_to_zone)
	{
		assign_to_zone(loc, zone);
	}

	// Restore adjacency edges
	for (const auto& [from, to] : snap.adjacency_edges)
	{
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

	// Restore contained actors
	for (const auto& [loc, actor_list] : snap.actors_by_location)
	{
		for (ActorId actor : actor_list)
		{
			place_actor(loc, actor);
		}
	}

	// Restore contained interactables
	for (const auto& [loc, object_list] : snap.interactables_by_location)
	{
		for (InteractableObjectId object : object_list)
		{
			place_interactable(loc, object);
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

	// Restore zone metadata
	for (const auto& [zone, meta] : snap.zone_metadata_map)
	{
		for (const auto& [key, value] : meta)
		{
			set_zone_meta(zone, key, value);
		}
	}

	// Restore region metadata
	for (const auto& [region, meta] : snap.region_metadata_map)
	{
		for (const auto& [key, value] : meta)
		{
			set_region_meta(region, key, value);
		}
	}
}

} // namespace gmMap

#endif // GMMAP_GMMAP_HPP
