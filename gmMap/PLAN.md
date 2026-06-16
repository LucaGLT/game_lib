# gmMap – Development Plan

## Phase 1 – Structure & API Design
- [x] Define type aliases (`LocationId`, `TileId`, `Metadata`)
- [x] Define exception hierarchy (`EMapError` and subclasses)
- [x] Design class template `gmMap<ItemT>` with full API
- [x] Write Doxygen comments on all declarations
- [x] Create `gmMap.hpp` (declarations + inline stub bodies)
- [x] Create `gmMap.cpp` (minimal template note)
- [x] Create `PLAN.md`

---

## Phase 2 – Private Helpers

- [x] Implement `_require_location(LocationId)` — throws `EUnknownLocationError`
- [x] Implement `_require_tile(TileId)` — throws `EUnknownTileError`
---

## Phase 3 – Construction / Reset

- [x] Implement `clear()` — empties `_locations` and `_tiles`
---

## Phase 4 – Location Management

- [x] Implement `create_location(id)` — inserts into `_locations`, throws on duplicate
- [x] Implement `remove_location(id)` — cleans tile membership + neighbor lists
- [x] Implement `has_location(id)`
- [x] Implement `all_locations()`
- [x] Implement `location_count()`
---

## Phase 5 – Tile Management
- [x] Implement `create_tile(id)` — inserts into `_tiles`, throws on duplicate
- [x] Implement `remove_tile(id)` — unassigns all member locations before deletion
- [x] Implement `has_tile(id)`
- [x] Implement `all_tiles()`
- [x] Implement `tile_count()`

---

## Phase 6 – Location ↔ Tile Assignment
- [x] Implement `assign_to_tile(loc, tile)` — handles re-assignment from another tile
- [x] Implement `unassign_from_tile(loc)` — no-op if not assigned
- [x] Implement `tile_of(loc)` — returns `std::optional<TileId>`
- [x] Implement `locations_in_tile(tile)`

---

## Phase 7 – Adjacency
- [x] Implement `set_adjacent(a, b, bidirectional)` — rejects self-loops
- [x] Implement `remove_adjacent(a, b, bidirectional)` — no-op if edge absent
- [x] Implement `are_adjacent(a, b)`
- [x] Implement `adjacent_to(id)`

---

## Phase 8 – Items
- [x] Implement `add_item(id, item)`
- [x] Implement `remove_item(id, index)` — throws on out-of-range
- [x] Implement `items_at(id)` — remove temporary static stub
- [x] Implement `clear_items(id)`

---

## Phase 9 – Location Metadata
- [x] Implement `set_location_meta(id, key, value)`
- [x] Implement `get_location_meta(id, key)` — throws `EUnknownMetaKeyError`
- [x] Implement `has_location_meta(id, key)`
- [x] Implement `remove_location_meta(id, key)` — no-op if absent
- [x] Implement `location_metadata(id)` — remove temporary static stub

---

## Phase 10 – Tile Metadata
- [x] Implement `set_tile_meta(id, key, value)`
- [x] Implement `get_tile_meta(id, key)` — throws `EUnknownMetaKeyError`
- [x] Implement `has_tile_meta(id, key)`
- [x] Implement `remove_tile_meta(id, key)` — no-op if absent
- [x] Implement `tile_metadata(id)` — remove temporary static stub

---

## Phase 11 – Unit Testing
- [x] Tests for location CRUD + invariants
- [x] Tests for tile CRUD + invariants
- [x] Tests for location ↔ tile assignment (including re-assignment)
- [x] Tests for adjacency (directed / bidirectional / self-loop guard)
- [x] Tests for items (add / remove / out-of-range)
- [x] Tests for location metadata
- [x] Tests for tile metadata
- [x] Tests for `clear()` (verifies full reset)
- [x] Tests for `remove_location` cascade (tile + neighbors cleanup)
- [x] Tests for `remove_tile` cascade (location unassignment)

---

## Phase 12 – Documentation
- [x] Write `gmMap_API.md` (usage examples)
- [x] Update root `README.md`
