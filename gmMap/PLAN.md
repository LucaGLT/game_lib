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

---

## Phase 13 – Hierarchy Evolution: Region/Zone + contained entities + snapshot v2

> Detailed design and decision log: `gmMap_evolution_plan.md`.
> Note: legacy *Tile* terminology renamed to *Zone* across the API (no backward
> alias kept); existing dependents (TicTacToe `Board`, gmMap tests) updated.

### Phase 13A – Design freeze (no code)
- [x] Decisions frozen: DP-1=B (optional `std::optional`), DP-2=A (`unordered_set`
      adjacency), A1 (`unordered_set` for actors/interactables), DP-3=A (distinct aliases)
- [x] Define aliases `RegionId`, `ZoneId` (ex `TileId`), `ActorId`, `InteractableObjectId`
- [x] Freeze JSON snapshot v2 schema

### Phase 13B – Tile → Zone rename + new Region level
- [x] Rename aliases, methods and exceptions (`E*Tile*` → `E*Zone*`)
- [x] Add `create/remove/has/all/count` for Region; region metadata API
- [x] `ZoneRecord.region_id` (`std::optional<RegionId>`); `_require_region`
- [x] `assign_zone_to_region`, `unassign_zone_from_region`, `region_of`, `zones_in_region`
- [x] `remove_zone` detaches from region; `remove_region` ungroups zones

### Phase 13C – Contained actors and interactables on Location
- [x] `LocationRecord` gains `unordered_set<ActorId> actors` and
      `unordered_set<InteractableObjectId> interactables`
- [x] `place_actor/remove_actor/has_actor/actors_at/clear_actors`
- [x] `place_interactable/remove_interactable/has_interactable/interactables_at/clear_interactables`

### Phase 13D – Snapshot v2 and migration
- [x] Extend `MapSnapshot` (region/zone/location ids, `location_to_zone`,
      `zone_to_region`, actors/interactables, per-level metadata maps)
- [x] Bump export version to v2
- [x] Transparent v1 → v2 migration on import (via `gmSave::peek_version`)
- [x] Snapshot round-trip test (T34)

### Phase 13 – Tests & docs
- [x] Update `test_gmMap.cpp` to Zone/Region API + new coverage (34 tests, all pass)
- [x] Update auxiliary tests (`smoke_phase5_10.cpp`, `test_snapshot_json.cpp`)
- [x] Update `gmMap_API.md` (v2) and root readme alias table

