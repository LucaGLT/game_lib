# gmSave – Development Plan

## Phase 1 – Structure & API Design
- [x] Define exception hierarchy (`SaveError` and subclasses)
- [x] Design free function template API (`save`, `load`, `try_load`, `save_versioned`, `load_versioned`, `peek_version`)
- [x] Write Doxygen comments on all declarations
- [x] Create `gmSave.hpp` (declarations + inline stub bodies)
- [x] Create `gmSave.cpp` (non-template bodies + template note)
- [x] Create `PLAN.md`

---

## Phase 2 – Dependency Setup
- [x] Download `json.hpp` (nlohmann/json single-header, >= 3.11)
  - Source: https://github.com/nlohmann/json/releases → `single_include/nlohmann/json.hpp`
- [x] Place `json.hpp` in `gmSave/` alongside `gmSave.hpp`
- [x] Verify `#include "json.hpp"` compiles without errors

---

## Phase 3 – Exception Bodies
- [x] Implement `VersionMismatchError(uint32_t expected, uint32_t found)`
  - Build message: `"Version mismatch: expected X, found Y"`
  - Store `expected_version` and `found_version` fields

---

## Phase 4 – Core I/O Helpers (internal, non-template)
> Defined in `gmSave.cpp` under `namespace GmSave::detail`;
> declared in `gmSave.hpp` under `namespace GmSave::detail`.

- [x] Implement `detail::write_file(filepath, content)` — opens file, writes string, throws `FileWriteError`
- [x] Implement `detail::read_file(filepath)` — opens file, reads full content, throws `FileReadError`
- [x] Implement `detail::parse_json(content, filepath)` — parses string to `nlohmann::json`, throws `JsonParseError`

---

## Phase 5 – `save()`
- [x] Implement `save<T>(filepath, data, indent)`
  - Calls `nlohmann::json j = data` (triggers ADL `to_json`)
  - Serializes with `j.dump(indent)`
  - Writes via `detail::write_file`

---

## Phase 6 – `load()`
- [x] Implement `load<T>(filepath)`
  - Reads via `detail::read_file`
  - Parses via `detail::parse_json`
  - Returns `j.get<T>()` (triggers ADL `from_json`)

---

## Phase 7 – `try_load()`
- [x] Implement `try_load<T>(filepath, out) noexcept`
  - Wraps `load<T>` in a `try/catch(...)`
  - Populates `out` only on success
  - Returns `false` on any exception, never throws

---

## Phase 8 – `save_versioned()`
- [x] Implement `save_versioned<T>(filepath, data, version, indent)`
  - Builds envelope: `{ "_version": version, "payload": <serialized data> }`
  - Delegates write to `detail::write_file`

---

## Phase 9 – `load_versioned()`
- [x] Implement `load_versioned<T>(filepath, expected_version)`
  - Reads and parses file
  - Validates `_version` field presence → throws `JsonParseError` if missing
  - Compares version → throws `VersionMismatchError` if mismatch
  - Deserializes `"payload"` field into `T`

---

## Phase 10 – `peek_version()`

- [x] Implement `peek_version(filepath)`
  - Reads and parses file (no deserialization of payload)
  - Returns `_version` as `std::optional<uint32_t>`
  - Returns `std::nullopt` on any I/O or parse error (non-throwing)

---

## Phase 11 – Unit Testing

- [x] Test `save` + `load` round-trip on flat struct
- [x] Test `save` + `load` round-trip on nested struct
- [x] Test `save` + `load` round-trip with `std::vector<T>` field
- [x] Test `save` + `load` round-trip with `std::optional<T>` field (present)
- [x] Test `save` + `load` round-trip with `std::optional<T>` field (absent)
- [x] Test `try_load` returns `false` on missing file (no throw)
- [x] Test `try_load` returns `false` on malformed JSON (no throw)
- [x] Test `save_versioned` + `load_versioned` round-trip (correct version)
- [x] Test `load_versioned` throws `VersionMismatchError` on wrong version
- [x] Test `load_versioned` throws `JsonParseError` on missing `_version` field
- [x] Test `peek_version` returns correct version
- [x] Test `peek_version` returns `nullopt` on plain (non-versioned) file
- [x] Test `FileReadError` thrown on non-existent file
- [x] Test `FileWriteError` thrown on unwritable path
- [x] Test `JsonParseError` thrown on invalid JSON content
- [x] Test compact output with `indent = -1`

---

## Phase 12 – Documentation

- [ ] Configure `Doxyfile` for `gmSave`
- [ ] Generate Doxygen HTML docs
- [ ] Write `gmSave_API.md` (usage examples)
- [ ] Update root `README.md`
