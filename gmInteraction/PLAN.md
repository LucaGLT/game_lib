# gmInteraction — PLAN

- **Library:** gmInteraction
- **Version:** 1.0
- **Status:** Phase 1 — Complete ✅
- **Language:** C++17 Standard
- **Namespace:** gmInteraction

---

## Goal

`gmInteraction` provides an authoritative registry for *interactable objects*
(doors, levers, chests, …) that live on a game map. It owns the object data
(type, lifecycle `InteractionState`, free-form metadata) while delegating
spatial placement to `gmMap` through opaque `InteractableObjectId` values.

The library is fully standalone: it depends only on `gmSave` for versioned
snapshots, and touches `gmMap` exclusively through the adapter in `bridges/`,
keeping both libraries decoupled.

---

## Architecture

```text
spawn_object / objects_at (bridge)
        │
        ├── InteractableObjectStore.create / get / set_state   (object data)
        │           │
        │           └── gmSave::save_versioned / load_versioned (snapshot v1)
        │
        └── gmMap::gmMap<ItemT>::place_interactable / interactables_at (placement)
```

---

## File structure

```text
gmInteraction/
├── GmInteractionError.hpp          ← base + derived exceptions
├── InteractionState.hpp            ← enum + string conversions
├── InteractableObject.hpp          ← POD record + id/metadata aliases
├── InteractableObjectStore.hpp/.cpp ← in-memory registry + snapshot
├── gmInteraction.hpp/.cpp          ← facade + version()
├── bridges/
│   └── MapInteractionBridge.hpp    ← only cross-include of gmMap
├── tests/
│   └── test_gmInteraction.cpp      ← self-contained runner (29 cases)
└── CMakeLists.txt                  ← STATIC lib + test target
```

---

## Development phases

### Phase 1 — Interfaces & Stubs ✅

- [x] Define `InteractableObjectId`, `Metadata`, `InteractableObject`.
- [x] Define `InteractionState` enum + `to_string`/`from_string`.
- [x] Base exception `EInteractionError : std::runtime_error` and derived
  (`EDuplicateObjectError`, `EUnknownObjectError`, `EUnknownMetaKeyError`).
- [x] Public facade `gmInteraction.hpp` + `version()`.
- [x] Smoke test: build + create/get/has round-trip PASS.

### Phase 2 — Store & Metadata ✅

- [x] `InteractableObjectStore`: create/remove/has/get/count/all_ids/clear.
- [x] State API: `set_state`/`state_of`/`type_of`.
- [x] Metadata API: `set_meta`/`get_meta`/`has_meta`/`remove_meta`/`metadata`.
- [x] Contract violations throw the appropriate exceptions.
- [x] Smoke test: state + metadata cases PASS.

### Phase 3 — Persistence ✅

- [x] Versioned snapshot (`SNAPSHOT_VERSION = 1`) via `gmSave`.
- [x] `export_snapshot_json` / `import_snapshot_json` with ADL-correct
  `to_json`/`from_json` for `InteractableObject`.
- [x] Smoke test: export → clear → import round-trip equality PASS.

### Phase 4 — gmMap bridge ✅

- [x] `MapInteractionBridge`: `spawn_object` / `despawn_object` / `objects_at`
  templated on the `gmMap` item type.
- [x] Only `bridges/` includes `gmMap` (no direct cross-include elsewhere).
- [x] Integration test with `gmMap<std::string>` PASS (29/29 total).

---

## Key design decisions

1. **Opaque ids shared with `gmMap`** — `InteractableObjectId = std::uint64_t`,
   identical alias semantics to `gmMap`, so placement and data stay in sync
   without coupling the two libraries.
2. **Store owns data, map owns placement** — the registry never stores a
   location; the bridge keeps the two views consistent and silently skips ids
   present on the map but absent from the store.
3. **`remove_meta` is idempotent** — removing an absent key is a no-op, not an
   error (error handling only at genuine contract boundaries).
4. **ADL-correct JSON** — `to_json`/`from_json` for the public
   `InteractableObject` live in `namespace gmInteraction` (not anonymous), so
   `gmSave`/nlohmann serialisation resolves them correctly.
