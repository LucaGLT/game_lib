# gmMap – Generic Tabletop Game Map Library

**Version:** 1.0 (stub)
**Status:** In Development
**Language:** C++17 Standard
**Namespace:** `GameMap`
**Header:** `gmMap.hpp`

---

## Table of Contents

- [Overview](#overview)
- [Design Philosophy](#design-philosophy)
- [Requirements](#requirements)
- [API Reference](#api-reference)
  - [Type Aliases](#type-aliases)
  - [Exception Hierarchy](#exception-hierarchy)
  - [class gmMap\<ItemT\>](#class-gmmapitemt)
    - [Construction / Reset](#construction--reset)
    - [Location Management](#location-management)
    - [Tile Management](#tile-management)
    - [Location ↔ Tile Assignment](#location--tile-assignment)
    - [Adjacency](#adjacency)
    - [Items](#items)
    - [Location Metadata](#location-metadata)
    - [Tile Metadata](#tile-metadata)
  - [Private Internals](#private-internals)
- [Class Invariants](#class-invariants)
- [Usage Examples](#usage-examples)
- [Error Handling](#error-handling)
- [Design Notes](#design-notes)

---

## Overview

**gmMap** is a generic, topology-agnostic game map library for C++17 tabletop
game applications.  It provides a single class template, `gmMap<ItemT>`, that
manages the complete state of a 2-D (or abstract) game board without enforcing
any coordinate system or grid shape.

Supported use cases include dungeon crawlers, war games, tactical turn-based
games, abstract strategy games, territory-control games, and any other domain
that needs a named graph of locations with typed items and heterogeneous
metadata.

### Key Features

| Feature | Detail |
|---|---|
| **Topology-agnostic** | No grid forced; topology expressed via adjacency edges |
| **Generic items** | `ItemT` template parameter; no constraint on the item domain |
| **Serializable metadata** | `std::unordered_map<std::string, MetadataValue>` on every node and tile |
| **Grouped locations** | Locations can be grouped into named *Tiles* (zones, floors, regions) |
| **Directed adjacency** | Edges can be unidirectional or bidirectional |
| **Safe by default** | Dedicated exception hierarchy; all invariants actively enforced |
| **Standard C++17** | Zero external dependencies |

---

## Design Philosophy

- **`gmMap` manages structure only.**  Item types, metadata schemas, and game
  rules live in the application layer, not in this library.
- **IDs are plain `uint32_t`.**  Cheap to copy, O(1) to compare, cache-friendly
  in hash maps.
- **Metadata is persistence-safe.**  Values are schema-constrained via
  `MetadataValue` (serializable scalar types + UID references).
- **Adjacency is explicit.**  There is no implicit neighbor calculation from
  coordinates; all edges are set by the caller.  This supports irregular maps,
  portal connections, one-way passages, etc.

---

## Requirements

- C++17-compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard library headers: `<cstdint>`, `<optional>`,
  `<stdexcept>`, `<string>`, `<unordered_map>`, `<unordered_set>`, `<vector>`
  and `<variant>`

---

## API Reference

### Type Aliases

Defined in namespace `GameMap`.

#### `LocationId`

```cpp
using LocationId = uint32_t;
```

Unique numeric identifier for a Location node in the map graph.

---

#### `TileId`

```cpp
using TileId = uint32_t;
```

Unique numeric identifier for a Tile (named group of locations).

---

#### `Metadata`

```cpp
using EntityUid = uint64_t;

struct UidRef {
  EntityUid value;
};

using UidList = std::vector<UidRef>;
using MetadataValue = std::variant<std::nullptr_t, bool, int64_t, double, std::string, UidRef, UidList>;
using Metadata = std::unordered_map<std::string, MetadataValue>;
```

Serializable key-value store attached to both locations and tiles. Keys are
`std::string`; values are `MetadataValue` and can represent scalar values,
single UID references (`UidRef`) or UID lists (`UidList`).

---

### Exception Hierarchy

All exceptions derive from `std::runtime_error`.

```
std::runtime_error
└-- MapError                    Base class for all gmMap errors
    ├-- DuplicateLocationError  create_location() called with existing ID
    ├-- UnknownLocationError    LocationId not found in the map
    ├-- DuplicateTileError      create_tile() called with existing ID
    ├-- UnknownTileError        TileId not found in the map
    ├-- InvalidAdjacencyError   Self-loop or adjacency invariant violation
    ├-- UnknownMetaKeyError     Metadata key not present
    └-- InvalidItemIndexError   Item index out of range
```

#### `MapError`

```cpp
class MapError : public std::runtime_error;
explicit MapError(const std::string& message);
```

Base class for all gmMap errors.  The string prefix `"MapError: "` is
prepended to every message.

---

#### `DuplicateLocationError`

```cpp
class DuplicateLocationError : public MapError;
explicit DuplicateLocationError(const std::string& message);
```

Thrown by `create_location()` when the given `LocationId` already exists.

---

#### `UnknownLocationError`

```cpp
class UnknownLocationError : public MapError;
explicit UnknownLocationError(const std::string& message);
```

Thrown whenever a `LocationId` is referenced that is not present in the map.

---

#### `DuplicateTileError`

```cpp
class DuplicateTileError : public MapError;
explicit DuplicateTileError(const std::string& message);
```

Thrown by `create_tile()` when the given `TileId` already exists.

---

#### `UnknownTileError`

```cpp
class UnknownTileError : public MapError;
explicit UnknownTileError(const std::string& message);
```

Thrown whenever a `TileId` is referenced that is not present in the map.

---

#### `InvalidAdjacencyError`

```cpp
class InvalidAdjacencyError : public MapError;
explicit InvalidAdjacencyError(const std::string& message);
```

Thrown when an adjacency operation violates map invariants, such as creating a
self-loop (`a == b`).

---

#### `UnknownMetaKeyError`

```cpp
class UnknownMetaKeyError : public MapError;
explicit UnknownMetaKeyError(const std::string& message);
```

Thrown by `get_location_meta()` and `get_tile_meta()` when the requested key
is not present.

---

#### `InvalidItemIndexError`

```cpp
class InvalidItemIndexError : public MapError;
explicit InvalidItemIndexError(const std::string& message);
```

Thrown by `remove_item()` when `index` is out of range.

---

### class gmMap\<ItemT\>

```cpp
template <typename ItemT>
class gmMap;
```

**Template parameter**

| Parameter | Description |
|---|---|
| `ItemT` | Type of items stored at each location. Must be copyable. |

Generic topology-agnostic game map.  All data is keyed by `LocationId` or
`TileId` (`uint32_t`), stored in hash maps for O(1) average-case access.

---

#### Construction / Reset

##### `gmMap()`

```cpp
gmMap() = default;
```

Default constructor. Creates an empty map with no locations, tiles, adjacency
edges, items, or metadata.

---

##### `clear()`

```cpp
void clear();
```

Removes all locations, tiles, adjacency edges, items, and metadata.  After
this call the object is in the same state as a default-constructed instance.

---

#### Location Management

##### `create_location()`

```cpp
void create_location(LocationId id);
```

Creates a new empty location.  The new location has no tile assignment, no
items, no metadata, and no neighbors.

| Parameter | Description |
|---|---|
| `id` | Unique identifier for the new location. |

**Throws:** `DuplicateLocationError` if `id` already exists.

---

##### `remove_location()`

```cpp
void remove_location(LocationId id);
```

Removes a location and cleans up all related state.  The location is
unassigned from its tile (if any) and removed from the neighbor lists of every
adjacent location before being deleted.

| Parameter | Description |
|---|---|
| `id` | The location to remove. |

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `has_location()`

```cpp
bool has_location(LocationId id) const;
```

| Parameter | Description |
|---|---|
| `id` | Location identifier to query. |

**Returns:** `true` if the location exists, `false` otherwise.

---

##### `all_locations()`

```cpp
std::vector<LocationId> all_locations() const;
```

**Returns:** Vector of every `LocationId` currently in the map.  Order is
unspecified.

---

##### `location_count()`

```cpp
std::size_t location_count() const;
```

**Returns:** Total number of locations in the map.

---

#### Tile Management

##### `create_tile()`

```cpp
void create_tile(TileId id);
```

Creates a new empty tile (zero locations assigned).

| Parameter | Description |
|---|---|
| `id` | Unique identifier for the new tile. |

**Throws:** `DuplicateTileError` if `id` already exists.

---

##### `remove_tile()`

```cpp
void remove_tile(TileId id);
```

Removes a tile.  All locations that belonged to this tile are left intact but
are unassigned (their tile membership is cleared).

| Parameter | Description |
|---|---|
| `id` | The tile to remove. |

**Throws:** `UnknownTileError` if `id` does not exist.

---

##### `has_tile()`

```cpp
bool has_tile(TileId id) const;
```

| Parameter | Description |
|---|---|
| `id` | Tile identifier to query. |

**Returns:** `true` if the tile exists, `false` otherwise.

---

##### `all_tiles()`

```cpp
std::vector<TileId> all_tiles() const;
```

**Returns:** Vector of every `TileId` currently in the map.  Order is
unspecified.

---

##### `tile_count()`

```cpp
std::size_t tile_count() const;
```

**Returns:** Total number of tiles in the map.

---

#### Location ↔ Tile Assignment

##### `assign_to_tile()`

```cpp
void assign_to_tile(LocationId loc, TileId tile);
```

Assigns a location to a tile.  If the location is already assigned to a
different tile it is first unassigned from that tile before being added to
`tile`.

| Parameter | Description |
|---|---|
| `loc` | Location to assign. |
| `tile` | Target tile. |

**Throws:**
- `UnknownLocationError` if `loc` does not exist.
- `UnknownTileError` if `tile` does not exist.

---

##### `unassign_from_tile()`

```cpp
void unassign_from_tile(LocationId loc);
```

Removes a location from its current tile.  Has no effect if the location is
not currently assigned to any tile.

| Parameter | Description |
|---|---|
| `loc` | Location to unassign. |

**Throws:** `UnknownLocationError` if `loc` does not exist.

---

##### `tile_of()`

```cpp
std::optional<TileId> tile_of(LocationId loc) const;
```

| Parameter | Description |
|---|---|
| `loc` | Location to query. |

**Returns:** The `TileId` if the location is assigned to a tile, or
`std::nullopt` if not assigned.

**Throws:** `UnknownLocationError` if `loc` does not exist.

---

##### `locations_in_tile()`

```cpp
std::vector<LocationId> locations_in_tile(TileId tile) const;
```

| Parameter | Description |
|---|---|
| `tile` | Tile to query. |

**Returns:** Vector of `LocationId` values belonging to `tile`.  Order is
unspecified.

**Throws:** `UnknownTileError` if `tile` does not exist.

---

#### Adjacency

##### `set_adjacent()`

```cpp
void set_adjacent(LocationId a, LocationId b, bool bidirectional = true);
```

Creates a directed or bidirectional edge between two locations.

| Parameter | Default | Description |
|---|---|---|
| `a` | — | Source location. |
| `b` | — | Target location. |
| `bidirectional` | `true` | If `true`, creates both a→b and b→a.  If `false`, creates only a→b. |

**Throws:**
- `UnknownLocationError` if either `a` or `b` does not exist.
- `InvalidAdjacencyError` if `a == b` (self-loops are not allowed).

---

##### `remove_adjacent()`

```cpp
void remove_adjacent(LocationId a, LocationId b, bool bidirectional = true);
```

Removes the adjacency edge between two locations.  No-op if the edge does not
exist.

| Parameter | Default | Description |
|---|---|---|
| `a` | — | First location. |
| `b` | — | Second location. |
| `bidirectional` | `true` | If `true`, removes both a→b and b→a.  If `false`, removes only a→b. |

**Throws:** `UnknownLocationError` if either `a` or `b` does not exist.

---

##### `are_adjacent()`

```cpp
bool are_adjacent(LocationId a, LocationId b) const;
```

Checks for a directed edge from `a` to `b`.

| Parameter | Description |
|---|---|
| `a` | Source location. |
| `b` | Target location. |

**Returns:** `true` if there is a directed edge from `a` to `b`.

**Throws:** `UnknownLocationError` if either `a` or `b` does not exist.

---

##### `adjacent_to()`

```cpp
std::vector<LocationId> adjacent_to(LocationId id) const;
```

Returns all locations directly reachable from `id` (outgoing edges).

| Parameter | Description |
|---|---|
| `id` | Source location. |

**Returns:** Vector of neighbor `LocationId` values.  Order is unspecified.

**Throws:** `UnknownLocationError` if `id` does not exist.

---

#### Items

##### `add_item()`

```cpp
void add_item(LocationId id, const ItemT& item);
```

Appends an item to the item list of a location (copied).

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `item` | Item to add. |

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `remove_item()`

```cpp
void remove_item(LocationId id, std::size_t index);
```

Removes the item at zero-based position `index`.  Items after `index` are
shifted left by one position.

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `index` | Zero-based index of the item to remove. |

**Throws:**
- `UnknownLocationError` if `id` does not exist.
- `InvalidItemIndexError` if `index` is out of range.

---

##### `items_at()`

```cpp
const std::vector<ItemT>& items_at(LocationId id) const;
```

| Parameter | Description |
|---|---|
| `id` | Target location. |

**Returns:** Const reference to the internal item list.  The reference is
invalidated by any call that modifies the map.

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `clear_items()`

```cpp
void clear_items(LocationId id);
```

Removes all items from a location.

| Parameter | Description |
|---|---|
| `id` | Target location. |

**Throws:** `UnknownLocationError` if `id` does not exist.

---

#### Location Metadata

##### `set_location_meta()`

```cpp
void set_location_meta(LocationId id, const std::string& key, const MetadataValue& value);
```

Sets (or overwrites) a metadata key on a location.

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `key` | Metadata key string. |
| `value` | Serializable metadata value (`MetadataValue`). |

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `get_location_meta()`

```cpp
const MetadataValue& get_location_meta(LocationId id, const std::string& key) const;
```

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `key` | Metadata key string. |

**Returns:** Const reference to the stored `MetadataValue`.

**Throws:**
- `UnknownLocationError` if `id` does not exist.
- `UnknownMetaKeyError` if `key` is not present.

---

##### `has_location_meta()`

```cpp
bool has_location_meta(LocationId id, const std::string& key) const;
```

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `key` | Metadata key string. |

**Returns:** `true` if the key exists, `false` otherwise.

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `remove_location_meta()`

```cpp
void remove_location_meta(LocationId id, const std::string& key);
```

Removes a metadata key from a location.  No-op if the key does not exist.

| Parameter | Description |
|---|---|
| `id` | Target location. |
| `key` | Metadata key string to remove. |

**Throws:** `UnknownLocationError` if `id` does not exist.

---

##### `location_metadata()`

```cpp
const Metadata& location_metadata(LocationId id) const;
```

| Parameter | Description |
|---|---|
| `id` | Target location. |

**Returns:** Const reference to the full `Metadata` map of the location.

**Throws:** `UnknownLocationError` if `id` does not exist.

---

#### Tile Metadata

##### `set_tile_meta()`

```cpp
void set_tile_meta(TileId id, const std::string& key, const MetadataValue& value);
```

Sets (or overwrites) a metadata key on a tile.

| Parameter | Description |
|---|---|
| `id` | Target tile. |
| `key` | Metadata key string. |
| `value` | Serializable metadata value (`MetadataValue`). |

**Throws:** `UnknownTileError` if `id` does not exist.

---

##### `get_tile_meta()`

```cpp
const MetadataValue& get_tile_meta(TileId id, const std::string& key) const;
```

| Parameter | Description |
|---|---|
| `id` | Target tile. |
| `key` | Metadata key string. |

**Returns:** Const reference to the stored `MetadataValue`.

**Throws:**
- `UnknownTileError` if `id` does not exist.
- `UnknownMetaKeyError` if `key` is not present.

---

##### `has_tile_meta()`

```cpp
bool has_tile_meta(TileId id, const std::string& key) const;
```

| Parameter | Description |
|---|---|
| `id` | Target tile. |
| `key` | Metadata key string. |

**Returns:** `true` if the key exists, `false` otherwise.

**Throws:** `UnknownTileError` if `id` does not exist.

---

##### `remove_tile_meta()`

```cpp
void remove_tile_meta(TileId id, const std::string& key);
```

Removes a metadata key from a tile.  No-op if the key does not exist.

| Parameter | Description |
|---|---|
| `id` | Target tile. |
| `key` | Metadata key string to remove. |

**Throws:** `UnknownTileError` if `id` does not exist.

---

##### `tile_metadata()`

```cpp
const Metadata& tile_metadata(TileId id) const;
```

| Parameter | Description |
|---|---|
| `id` | Target tile. |

**Returns:** Const reference to the full `Metadata` map of the tile.

**Throws:** `UnknownTileError` if `id` does not exist.

---

### Private Internals

> These are implementation details not part of the public API.  Documented here
> for contributors.

#### `LocationRecord`

```cpp
struct LocationRecord {
    std::optional<TileId>           tile_id;   // owning tile, if assigned
    std::vector<ItemT>              items;     // items at this location
    Metadata                        meta;      // heterogeneous metadata
    std::unordered_set<LocationId>  neighbors; // outgoing adjacency edges
};
```

Internal storage record for a single location node.

#### `TileRecord`

```cpp
struct TileRecord {
    std::unordered_set<LocationId>  locations; // locations assigned to this tile
    Metadata                        meta;      // heterogeneous metadata
};
```

Internal storage record for a tile (named group of locations).

#### `_require_location(LocationId)`

```cpp
void _require_location(LocationId id) const;
```

Asserts that a location ID exists; throws `UnknownLocationError` otherwise.
Called at the top of every public method that takes a `LocationId`.

#### `_require_tile(TileId)`

```cpp
void _require_tile(TileId id) const;
```

Asserts that a tile ID exists; throws `UnknownTileError` otherwise.
Called at the top of every public method that takes a `TileId`.

---

## Class Invariants

The following invariants are actively enforced at all times:

1. **Unique IDs** – No two locations share the same `LocationId`; no two tiles
   share the same `TileId`.
2. **Single tile membership** – A location belongs to at most one tile.
3. **Valid neighbors** – Every neighbor in an adjacency list is a valid
   `LocationId` present in `_locations`.
4. **No self-loops** – A location cannot be adjacent to itself.
5. **Symmetric bidirectional edges** – When `bidirectional = true`, both
   directions are created and removed together.
6. **Cascade on remove_location** –
   - Location is removed from its owning tile's `locations` set.
   - Location is removed from the `neighbors` set of every adjacent location.
7. **Cascade on remove_tile** – All member locations have their `tile_id`
   reset to `std::nullopt`; the locations themselves are not deleted.

---

## Usage Examples

### Basic dungeon map

```cpp
#include "gmMap.hpp"
#include <string>

struct GameItem { std::string name; int value; };

// Instantiate with GameItem as the item type
GameMap::gmMap<GameItem> dungeon;

// Create tiles (floors/zones)
dungeon.create_tile(1);   // floor 1
dungeon.create_tile(2);   // floor 2

// Create locations (rooms)
for (GameMap::LocationId id = 101; id <= 105; ++id)
    dungeon.create_location(id);

// Group rooms into tiles
dungeon.assign_to_tile(101, 1);
dungeon.assign_to_tile(102, 1);
dungeon.assign_to_tile(103, 2);
dungeon.assign_to_tile(104, 2);
dungeon.assign_to_tile(105, 2);

// Connect rooms (bidirectional corridors)
dungeon.set_adjacent(101, 102);
dungeon.set_adjacent(102, 103);
dungeon.set_adjacent(103, 104);
dungeon.set_adjacent(104, 105);

// One-way secret passage
dungeon.set_adjacent(105, 101, /*bidirectional=*/false);

// Place items
dungeon.add_item(102, GameItem{"Sword",  50});
dungeon.add_item(103, GameItem{"Shield", 30});

// Attach metadata
dungeon.set_location_meta(101, "name",    std::string("Entry Hall"));
dungeon.set_location_meta(101, "visited", false);
dungeon.set_tile_meta    (1,   "level",   1);
```

### Reading metadata with std::get

```cpp
const GameMap::MetadataValue& raw = dungeon.get_location_meta(101, "name");
std::string room_name = std::get<std::string>(raw);

bool visited = std::get<bool>(dungeon.get_location_meta(101, "visited"));
```

### Querying adjacency

```cpp
std::vector<GameMap::LocationId> neighbors = dungeon.adjacent_to(103);
// neighbors == {102, 104}  (order unspecified)

bool connected = dungeon.are_adjacent(105, 101);   // true (one-way)
bool reverse   = dungeon.are_adjacent(101, 105);   // false
```

### Iterating items at a location

```cpp
for (const GameItem& item : dungeon.items_at(102)) {
    // item.name, item.value
}
```

### Exception handling

```cpp
try {
    dungeon.create_location(101);  // already exists
} catch (const GameMap::DuplicateLocationError& e) {
    // e.what() -> "MapError: ..."
}

try {
  const GameMap::MetadataValue& v = dungeon.get_location_meta(101, "missing_key");
  (void)v;
} catch (const GameMap::UnknownMetaKeyError& e) {
    // handle missing key
}
```

### Integrazione con gmLog (tracing operazioni mappa)

```cpp
#include "gmMap.hpp"
#include "gmLog/LoggerFactory.hpp"
#include "gmLog/macros/LogMacros.hpp"

GameMap::gmMap<std::string> world;
GmLog::Logger log = GmLog::LoggerFactory::createFileLogger(
  "gmMapFlow", "gmMap_flow.log", GmLog::LogLevel::Info, true);

world.create_tile(1);
LOG_INFO(log, "Creato tile 1");

world.create_location(1001);
world.assign_to_tile(1001, 1);
LOG_INFO(log, "Assegnata location 1001 a tile 1");

world.set_location_meta(1001, "name", std::string("Bridge"));
LOG_INFO(log, "Metadata aggiornati per location 1001");
```

### Integrazione con gmSave (snapshot JSON dello stato)

`gmMap` usa metadata serializzabili (`MetadataValue`) con supporto a UID.
Per snapshot completi e versionabili e' comunque consigliato usare un DTO
esplicito, separando la persistenza dalla cache runtime UID -> puntatore.

```cpp
#include "gmMap.hpp"
#include "gmSave/gmSave.hpp"
#include <string>
#include <vector>

struct MapSnapshot {
  std::vector<GameMap::LocationId> locations;
  std::vector<GameMap::TileId> tiles;
};

inline void to_json(nlohmann::json& j, const MapSnapshot& s) {
  j = nlohmann::json{{"locations", s.locations}, {"tiles", s.tiles}};
}

inline void from_json(const nlohmann::json& j, MapSnapshot& s) {
  j.at("locations").get_to(s.locations);
  j.at("tiles").get_to(s.tiles);
}

GameMap::gmMap<std::string> world;
world.create_location(1);
world.create_location(2);
world.create_tile(10);

MapSnapshot out;
out.locations = world.all_locations();
out.tiles = world.all_tiles();

GmSave::save_versioned("gmMap_snapshot.json", out, 1);

MapSnapshot loaded = GmSave::load_versioned<MapSnapshot>("gmMap_snapshot.json", 1);
```

Note:
- Per persistere anche items e adjacency in modo completo, estendi il DTO con:
  liste di edge, item per location e metadati `MetadataValue`/UID.
- `gmSave::peek_version()` e' utile per migrazioni di formato degli snapshot.

---

## Error Handling

| Situation | Exception thrown |
|---|---|
| `create_location` with existing ID | `DuplicateLocationError` |
| Any operation on non-existent `LocationId` | `UnknownLocationError` |
| `create_tile` with existing ID | `DuplicateTileError` |
| Any operation on non-existent `TileId` | `UnknownTileError` |
| `set_adjacent(a, a, ...)` self-loop | `InvalidAdjacencyError` |
| `get_*_meta` with missing key | `UnknownMetaKeyError` |
| `remove_item` with out-of-range index | `InvalidItemIndexError` |

---

## Design Notes

### Why no coordinates?

Coordinates impose a specific topology (grid, hex, isometric).  gmMap stores
adjacency edges explicitly to support any topology: irregular dungeon graphs,
point-to-point war-game maps, node networks, etc.  If coordinates are needed
for rendering they can be stored as metadata (e.g.
`set_location_meta(id, "x", 3); set_location_meta(id, "y", 7);`).

### Why MetadataValue + UID for metadata?

`MetadataValue` keeps metadata serializable by design and supports stable
cross-object references via UID (`UidRef` / `UidList`).
Pointers remain runtime-only concerns and should stay in a transient cache,
not in persisted map state.

### Template header-only implementation

Because `gmMap` is a class template, all method bodies must be compiled in
every translation unit that uses the class.  They are therefore defined as
inline functions at the bottom of `gmMap.hpp`.  The accompanying `gmMap.cpp`
is intentionally minimal and serves only as a documentation anchor and a place
for future explicit template instantiations.
