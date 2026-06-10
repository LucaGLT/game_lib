# gmMap – Development Plan

## Phase 1 – Structure & API Design
- [x] Define type aliases (`LocationId`, `TileId`, `Metadata`)
- [x] Define exception hierarchy (`MapError` and subclasses)
- [x] Design class template `gmMap<ItemT>` with full API
- [x] Write Doxygen comments on all declarations
- [x] Create `gmMap.hpp` (declarations + inline stub bodies)
- [x] Create `gmMap.cpp` (minimal template note)
- [x] Create `PLAN.md`

---

## Phase 2 – Private Helpers

- [x] Implement `_require_location(LocationId)` — throws `UnknownLocationError`
- [x] Implement `_require_tile(TileId)` — throws `UnknownTileError`
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
- [ ] Implement `create_tile(id)` — inserts into `_tiles`, throws on duplicate
- [ ] Implement `remove_tile(id)` — unassigns all member locations before deletion
- [ ] Implement `has_tile(id)`
- [ ] Implement `all_tiles()`
- [ ] Implement `tile_count()`

---

## Phase 6 – Location ↔ Tile Assignment
- [ ] Implement `assign_to_tile(loc, tile)` — handles re-assignment from another tile
- [ ] Implement `unassign_from_tile(loc)` — no-op if not assigned
- [ ] Implement `tile_of(loc)` — returns `std::optional<TileId>`
- [ ] Implement `locations_in_tile(tile)`

---

## Phase 7 – Adjacency
- [ ] Implement `set_adjacent(a, b, bidirectional)` — rejects self-loops
- [ ] Implement `remove_adjacent(a, b, bidirectional)` — no-op if edge absent
- [ ] Implement `are_adjacent(a, b)`
- [ ] Implement `adjacent_to(id)`

---

## Phase 8 – Items
- [ ] Implement `add_item(id, item)`
- [ ] Implement `remove_item(id, index)` — throws on out-of-range
- [ ] Implement `items_at(id)` — remove temporary static stub
- [ ] Implement `clear_items(id)`

---

## Phase 9 – Location Metadata
- [ ] Implement `set_location_meta(id, key, value)`
- [ ] Implement `get_location_meta(id, key)` — throws `UnknownMetaKeyError`
- [ ] Implement `has_location_meta(id, key)`
- [ ] Implement `remove_location_meta(id, key)` — no-op if absent
- [ ] Implement `location_metadata(id)` — remove temporary static stub

---

## Phase 10 – Tile Metadata
- [ ] Implement `set_tile_meta(id, key, value)`
- [ ] Implement `get_tile_meta(id, key)` — throws `UnknownMetaKeyError`
- [ ] Implement `has_tile_meta(id, key)`
- [ ] Implement `remove_tile_meta(id, key)` — no-op if absent
- [ ] Implement `tile_metadata(id)` — remove temporary static stub

---

## Phase 11 – Unit Testing
- [ ] Tests for location CRUD + invariants
- [ ] Tests for tile CRUD + invariants
- [ ] Tests for location ↔ tile assignment (including re-assignment)
- [ ] Tests for adjacency (directed / bidirectional / self-loop guard)
- [ ] Tests for items (add / remove / out-of-range)
- [ ] Tests for location metadata
- [ ] Tests for tile metadata
- [ ] Tests for `clear()` (verifies full reset)
- [ ] Tests for `remove_location` cascade (tile + neighbors cleanup)
- [ ] Tests for `remove_tile` cascade (location unassignment)

---

## Phase 12 – Documentation
- [ ] Configure `Doxyfile` for `gmMap`
- [ ] Generate Doxygen HTML docs
- [ ] Write `gmMap_API.md` (usage examples)
- [ ] Update root `README.md`
