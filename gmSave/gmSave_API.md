# gmSave – Generic JSON Save/Load Library

**Version:** 1.0
**Status:** Production
**Language:** C++17 Standard
**Namespace:** `gmSave`
**Header:** `gmSave.hpp`
**Dependency:** nlohmann/json >= 3.9 (`json.hpp`, single-header, vendored)

---

## Table of Contents

- [gmSave – Generic JSON Save/Load Library](#gmsave--generic-json-saveload-library)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
    - [Key Features](#key-features)
  - [Design Philosophy](#design-philosophy)
  - [Requirements \& Setup](#requirements--setup)
  - [User Contract](#user-contract)
    - [Flat struct](#flat-struct)
    - [Nested struct](#nested-struct)
    - [std::vector\<T\>](#stdvectort)
    - [std::optional\<T\>](#stdoptionalt)
  - [API Reference](#api-reference)
    - [Exception Hierarchy](#exception-hierarchy)
      - [`ESaveError`](#esaveerror)
      - [`EFileWriteError`](#efilewriteerror)
      - [`EFileReadError`](#efilereaderror)
      - [`EJsonParseError`](#ejsonparseerror)
      - [`EVersionMismatchError`](#eversionmismatcherror)
    - [`save()`](#save)
    - [`load()`](#load)
    - [`try_load()`](#try_load)
    - [`save_versioned()`](#save_versioned)
    - [`load_versioned()`](#load_versioned)
    - [`peek_version()`](#peek_version)
    - [Internal Helpers (detail)](#internal-helpers-detail)
      - [`detail::write_file()`](#detailwrite_file)
      - [`detail::read_file()`](#detailread_file)
      - [`detail::parse_json()`](#detailparse_json)
  - [Versioned Envelope Format](#versioned-envelope-format)
  - [Usage Examples](#usage-examples)
    - [Round-trip: flat struct](#round-trip-flat-struct)
    - [Round-trip: versioned save](#round-trip-versioned-save)
    - [Non-throwing load at startup](#non-throwing-load-at-startup)
    - [Version detection for migration](#version-detection-for-migration)
    - [Nested struct + vector + optional](#nested-struct--vector--optional)
    - [Compact output](#compact-output)
    - [Exception handling](#exception-handling)
  - [Error Handling](#error-handling)
  - [Design Notes](#design-notes)
    - [Why free functions instead of member functions?](#why-free-functions-instead-of-member-functions)
    - [Why nlohmann/json?](#why-nlohmannjson)
    - [Why vendor json.hpp?](#why-vendor-jsonhpp)
    - [Template bodies in the header](#template-bodies-in-the-header)

---

## Overview

**gmSave** is a lightweight, generic JSON persistence library for C++17.  It
provides a handful of free function templates that allow any user-defined struct
to be saved to and loaded from a JSON file with minimal boilerplate.

The library is a thin, type-safe wrapper around
[nlohmann/json](https://github.com/nlohmann/json) that:

- hides raw JSON handling behind a clear, domain-oriented API,
- maps all I/O and parse failures to a dedicated exception hierarchy,
- adds optional versioned save-files with a one-field envelope.

### Key Features

| Feature | Detail |
|---|---|
| **Fully generic** | Works with any struct that defines `to_json` / `from_json` |
| **Versioned saves** | Built-in `_version` envelope; `peek_version` for forward detection |
| **Non-throwing variant** | `try_load` never throws; ideal for startup loading |
| **Heterogeneous fields** | Supports `std::vector<T>`, `std::optional<T>`, nested structs |
| **Compact output** | `indent = -1` produces single-line JSON |
| **Standard C++17** | Zero dependencies beyond nlohmann/json (vendored, header-only) |

---

## Design Philosophy

- **gmSave manages I/O only.**  Field names, types, and schema validation belong
  to the application layer via the `to_json` / `from_json` contract.
- **No intrusion on user structs.**  The two hook functions are free functions
  defined outside the struct, so the struct itself stays clean.
- **Fail loudly.**  Every I/O or parse problem throws a typed exception with a
  human-readable message.  The only silent variant is `try_load`, which is
  explicitly opt-in.
- **Template bodies in the header.**  The C++ template requirement is satisfied
  by placing all template implementations as inline definitions at the bottom of
  `gmSave.hpp`.  The non-template bodies (`EVersionMismatchError` constructor
  and `peek_version`) live in `gmSave.cpp`.

---

## Requirements & Setup

- C++17-compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- `json.hpp` (nlohmann/json single-header) placed in the same directory as
  `gmSave.hpp`

```text
gmSave/
├── gmSave.hpp   ← include this in your project
├── gmSave.cpp   ← compile this translation unit
└── json.hpp     ← nlohmann/json vendored copy
```

Include in your source:

```cpp
#include "gmSave.hpp"
```

---

## User Contract

For each struct `T` that you want to persist, define two free functions in the
**same namespace** as `T`.  nlohmann/json finds them via
[ADL](https://en.cppreference.com/w/cpp/language/adl) — no registration call
needed.

### Flat struct

```cpp
struct Config {
    std::string map_name;
    int         max_players;
    float       time_limit;
};

void to_json(nlohmann::json& j, const Config& c) {
    j = {
        {"map_name",    c.map_name},
        {"max_players", c.max_players},
        {"time_limit",  c.time_limit}
    };
}

void from_json(const nlohmann::json& j, Config& c) {
    j.at("map_name").get_to(c.map_name);
    j.at("max_players").get_to(c.max_players);
    j.at("time_limit").get_to(c.time_limit);
}
```

### Nested struct

If `Config` contains another struct, define `to_json` / `from_json` for the
inner type too. nlohmann calls them recursively:

```cpp
struct Difficulty { int level; bool permadeath; };

void to_json(nlohmann::json& j, const Difficulty& d) {
    j = {{"level", d.level}, {"permadeath", d.permadeath}};
}
void from_json(const nlohmann::json& j, Difficulty& d) {
    j.at("level").get_to(d.level);
    j.at("permadeath").get_to(d.permadeath);
}

struct Config {
    std::string map_name;
    Difficulty  difficulty;   // ← nested
};

void to_json(nlohmann::json& j, const Config& c) {
    j = {{"map_name", c.map_name}, {"difficulty", c.difficulty}};
}
void from_json(const nlohmann::json& j, Config& c) {
    j.at("map_name").get_to(c.map_name);
    j.at("difficulty").get_to(c.difficulty);  // ← automatic recursion
}
```

### std::vector\<T\>

nlohmann handles `std::vector<T>` natively as a JSON array, provided `T` has
its own `to_json` / `from_json`:

```cpp
struct SaveFile {
    std::string              player_name;
    std::vector<std::string> unlocked_levels;
};

void to_json(nlohmann::json& j, const SaveFile& s) {
    j = {{"player_name", s.player_name}, {"unlocked_levels", s.unlocked_levels}};
}
void from_json(const nlohmann::json& j, SaveFile& s) {
    j.at("player_name").get_to(s.player_name);
    j.at("unlocked_levels").get_to(s.unlocked_levels);
}
```

### std::optional\<T\>

nlohmann >= 3.9 serializes `std::optional<T>` natively: present values become
the JSON value; absent values become JSON `null`:

```cpp
struct Hero {
    std::string        name;
    std::optional<int> bonus_hp;   // null if not set
};

void to_json(nlohmann::json& j, const Hero& h) {
    j = {{"name", h.name}, {"bonus_hp", h.bonus_hp}};
}
void from_json(const nlohmann::json& j, Hero& h) {
    j.at("name").get_to(h.name);
    j.at("bonus_hp").get_to(h.bonus_hp);
}
```

---

## API Reference

All symbols are in namespace `gmSave`.

---

### Exception Hierarchy

```text
std::runtime_error
└── ESaveError                  Base class for all gmSave errors
    ├── EFileWriteError         File cannot be opened or written
    ├── EFileReadError          File not found or not readable
    ├── EJsonParseError         Content is not valid JSON, or required
    │                          envelope fields are missing
    └── EVersionMismatchError   _version in file ≠ expected_version
```

---

#### `ESaveError`

```cpp
class ESaveError : public std::runtime_error;
explicit ESaveError(const std::string& message);
```

Base class for all gmSave errors.  The prefix `"ESaveError: "` is prepended to
every message.

---

#### `EFileWriteError`

```cpp
class EFileWriteError : public ESaveError;
explicit EFileWriteError(const std::string& message);
```

Thrown when the destination file cannot be opened or a write operation fails.

---

#### `EFileReadError`

```cpp
class EFileReadError : public ESaveError;
explicit EFileReadError(const std::string& message);
```

Thrown when the source file cannot be found or opened for reading.

---

#### `EJsonParseError`

```cpp
class EJsonParseError : public ESaveError;
explicit EJsonParseError(const std::string& message);
```

Thrown when:
- the file content is not valid JSON, or
- a versioned file is missing the required `_version` or `payload` fields.

---

#### `EVersionMismatchError`

```cpp
class EVersionMismatchError : public ESaveError;
EVersionMismatchError(uint32_t expected, uint32_t found);

uint32_t expected_version;   // version requested by the caller
uint32_t found_version;      // version read from the file
```

Thrown by `load_versioned()` when the `_version` field in the file does not
match the value supplied by the caller.  Both version numbers are accessible
as public members for programmatic handling (e.g. migration logic).

**Example message:**
```text
ESaveError: Version mismatch: expected 2, found 1
```

---

### `save()`

```cpp
template <typename T>
void save(const std::string& filepath, const T& data, int indent = 2);
```

Serializes `data` to a JSON file.

Internally calls `nlohmann::json j = data` (which triggers ADL `to_json`),
then writes `j.dump(indent)` to `filepath`.

| Parameter | Default | Description |
|---|---|---|
| `filepath` | — | Destination file path; created or overwritten. |
| `data` | — | Value to serialize. `T` must have `to_json`. |
| `indent` | `2` | Spaces per indentation level. Use `-1` for compact output. |

**Throws:** `EFileWriteError` if the file cannot be opened or written.

---

### `load()`

```cpp
template <typename T>
T load(const std::string& filepath);
```

Deserializes a value of type `T` from a JSON file.

Reads the file, parses it, then returns `j.get<T>()` (which triggers ADL
`from_json`).

| Parameter | Description |
|---|---|
| `filepath` | Source file path. |

**Returns:** The deserialized value of type `T`.

**Throws:**

- `EFileReadError` if the file cannot be opened.
- `EJsonParseError` if the content is not valid JSON.

---

### `try_load()`

```cpp
template <typename T>
bool try_load(const std::string& filepath, T& out) noexcept;
```

Non-throwing variant of `load()`.  Attempts to load and deserialize the file
into `out`.  On any error `out` is left unchanged and `false` is returned.

| Parameter | Description |
|---|---|
| `filepath` | Source file path. |
| `out` | Output variable; populated only on success. |

**Returns:** `true` on success, `false` on any error.

**Throws:** Never.

> Use this at application startup when a missing or corrupt save file is an
> expected condition that should be handled silently.

---

### `save_versioned()`

```cpp
template <typename T>
void save_versioned(const std::string& filepath,
                    const T&           data,
                    uint32_t           version,
                    int                indent = 2);
```

Serializes `data` inside a version envelope and writes it to a JSON file.

The envelope format is:

```json
{
  "_version": <version>,
  "payload": { ... }
}
```

| Parameter | Default | Description |
|---|---|---|
| `filepath` | — | Destination file path; created or overwritten. |
| `data` | — | Value to serialize. `T` must have `to_json`. |
| `version` | — | Version tag written to the `_version` field. |
| `indent` | `2` | Spaces per indentation level. Use `-1` for compact output. |

**Throws:** `EFileWriteError` if the file cannot be opened or written.

---

### `load_versioned()`

```cpp
template <typename T>
T load_versioned(const std::string& filepath, uint32_t expected_version);
```

Deserializes a value from a versioned JSON file, enforcing the version check.

Steps performed:
1. Read and parse the file.
2. Validate that both `_version` and `payload` fields exist.
3. Compare `_version` with `expected_version`.
4. Deserialize `payload` into `T`.

| Parameter | Description |
|---|---|
| `filepath` | Source file path. |
| `expected_version` | The version the caller requires. |

**Returns:** The deserialized value of type `T`.

**Throws:**

- `EFileReadError` if the file cannot be opened.
- `EJsonParseError` if the content is not valid JSON, or `_version` / `payload` fields are absent.
- `EVersionMismatchError` if `_version != expected_version`.

---

### `peek_version()`

```cpp
std::optional<uint32_t> peek_version(const std::string& filepath);
```

Reads only the `_version` field from a versioned JSON file without
deserializing the payload.

Useful for detecting save-file version at startup before deciding which struct
layout or migration path to apply.

| Parameter | Description |
|---|---|
| `filepath` | Source file path. |

**Returns:** The `_version` value as `std::optional<uint32_t>`, or
`std::nullopt` if:
- the file does not exist,
- the content is not valid JSON,
- the `_version` field is absent or not an unsigned integer.

**Throws:** Never (all errors are silently swallowed into `std::nullopt`).

---

### Internal Helpers (detail)

> These live in `namespace gmSave::detail` and are not part of the public API.
> Documented here for contributors.

#### `detail::write_file()`

```cpp
void write_file(const std::string& filepath, const std::string& content);
```

Opens `filepath` for writing (creating or overwriting), writes `content`, then
checks `ofs.good()`.

**Throws:** `EFileWriteError` on open failure or write error.

---

#### `detail::read_file()`

```cpp
std::string read_file(const std::string& filepath);
```

Opens `filepath` for reading and returns the full content as a `std::string`
using `std::istreambuf_iterator`.

**Throws:** `EFileReadError` if the file cannot be opened.

---

#### `detail::parse_json()`

```cpp
nlohmann::json parse_json(const std::string& content, const std::string& filepath);
```

Calls `nlohmann::json::parse(content)` and maps any `nlohmann::json::parse_error`
to `EJsonParseError` with the file path included in the message.

**Throws:** `EJsonParseError` if `content` is not valid JSON.

---

## Versioned Envelope Format

When using `save_versioned` / `load_versioned`, the file on disk has the
following structure:

```json
{
  "_version": 2,
  "payload": {
    "player_name": "Gandalf",
    "hp": 100,
    "inventory": ["Staff", "Ring"]
  }
}
```

- `_version` is a `uint32_t` written as a JSON unsigned integer.
- `payload` contains the serialized `T` exactly as `save()` would write it.
- No other top-level fields are written by the library; the caller is free to
  add them manually if needed.

---

## Usage Examples

### Round-trip: flat struct

```cpp
#include "gmSave.hpp"

struct Config {
    std::string map_name;
    int         max_players;
};

void to_json(nlohmann::json& j, const Config& c) {
    j = {{"map_name", c.map_name}, {"max_players", c.max_players}};
}
void from_json(const nlohmann::json& j, Config& c) {
    j.at("map_name").get_to(c.map_name);
    j.at("max_players").get_to(c.max_players);
}

// --- Save ---
Config cfg{"dungeon_01", 4};
gmSave::save("config.json", cfg);

// config.json:
// {
//   "map_name": "dungeon_01",
//   "max_players": 4
// }

// --- Load ---
Config loaded = gmSave::load<Config>("config.json");
```

---

### Round-trip: versioned save

```cpp
gmSave::save_versioned("save.json", cfg, /*version=*/2);

// save.json:
// {
//   "_version": 2,
//   "payload": {
//     "map_name": "dungeon_01",
//     "max_players": 4
//   }
// }

Config restored = gmSave::load_versioned<Config>("save.json", /*expected_version=*/2);
```

---

### Non-throwing load at startup

```cpp
Config cfg;  // default values
if (!gmSave::try_load("config.json", cfg)) {
    // file missing or corrupt — use defaults, no exception
    cfg = Config{"default_map", 2};
}
```

---

### Version detection for migration

```cpp
std::optional<uint32_t> ver = gmSave::peek_version("save.json");

if (!ver.has_value()) {
    // plain (non-versioned) file or missing file
    // handle legacy format
} else if (*ver == 1) {
    SaveFileV1 old = gmSave::load_versioned<SaveFileV1>("save.json", 1);
    // migrate old → current format
} else if (*ver == 2) {
    SaveFileV2 current = gmSave::load_versioned<SaveFileV2>("save.json", 2);
}
```

---

### Nested struct + vector + optional

```cpp
struct Item   { std::string name; int damage; };
struct Hero   {
    std::string         name;
    int                 hp;
    std::optional<int>  mana;
    std::vector<Item>   inventory;
};

// to_json / from_json for Item
void to_json(nlohmann::json& j, const Item& i) {
    j = {{"name", i.name}, {"damage", i.damage}};
}
void from_json(const nlohmann::json& j, Item& i) {
    j.at("name").get_to(i.name);
    j.at("damage").get_to(i.damage);
}

// to_json / from_json for Hero
void to_json(nlohmann::json& j, const Hero& h) {
    j = {{"name", h.name}, {"hp", h.hp},
         {"mana", h.mana}, {"inventory", h.inventory}};
}
void from_json(const nlohmann::json& j, Hero& h) {
    j.at("name").get_to(h.name);
    j.at("hp").get_to(h.hp);
    j.at("mana").get_to(h.mana);
    j.at("inventory").get_to(h.inventory);
}

// Save
Hero hero = {"Gandalf", 100, 80, {{"Staff", 15}, {"Fireball", 40}}};
gmSave::save_versioned("hero.json", hero, 1);

// Load
Hero loaded = gmSave::load_versioned<Hero>("hero.json", 1);

// hero.json on disk:
// {
//   "_version": 1,
//   "payload": {
//     "name": "Gandalf",
//     "hp": 100,
//     "mana": 80,
//     "inventory": [
//       {"damage": 15, "name": "Staff"},
//       {"damage": 40, "name": "Fireball"}
//     ]
//   }
// }
```

---

### Compact output

```cpp
gmSave::save("config.json", cfg, /*indent=*/-1);
// {"map_name":"dungeon_01","max_players":4}
```

---

### Exception handling

```cpp
// --- EFileReadError ---
try {
    Config c = gmSave::load<Config>("missing.json");
} catch (const gmSave::EFileReadError& e) {
    // e.what() -> "ESaveError: Cannot open file for reading: missing.json"
}

// --- EJsonParseError ---
try {
    Config c = gmSave::load<Config>("corrupt.json");
} catch (const gmSave::EJsonParseError& e) {
    // e.what() -> "ESaveError: JSON parse error in 'corrupt.json': ..."
}

// --- EVersionMismatchError ---
try {
    Config c = gmSave::load_versioned<Config>("save.json", /*expected=*/3);
} catch (const gmSave::EVersionMismatchError& e) {
    uint32_t expected = e.expected_version;   // 3
    uint32_t found    = e.found_version;      // e.g. 1
}
```

---

## Error Handling

| Situation | Exception |
|---|---|
| Destination file cannot be opened / written | `EFileWriteError` |
| Source file not found or not readable | `EFileReadError` |
| File content is not valid JSON | `EJsonParseError` |
| Versioned file missing `_version` or `payload` | `EJsonParseError` |
| `_version` in file ≠ `expected_version` | `EVersionMismatchError` |
| Any error inside `try_load` | Silent — returns `false` |
| Any error inside `peek_version` | Silent — returns `std::nullopt` |

---

## Design Notes

### Why free functions instead of member functions?

`to_json` / `from_json` are defined outside the struct, so the struct itself
has no dependency on nlohmann or on gmSave.  A `Hero` struct in your game
engine does not need to `#include "json.hpp"` just to be serializable.

### Why nlohmann/json?

It is the de-facto standard C++17 JSON library, ships as a single header,
requires no external build system, and supports `std::optional`, `std::vector`,
and arbitrary nesting out of the box.

### Why vendor json.hpp?

Vendoring one header eliminates version drift, network access requirements in
CI, and build system complexity.  The file is placed next to `gmSave.hpp` so
the include path is simply `"json.hpp"`.

### Template bodies in the header

Because `save`, `load`, `try_load`, `save_versioned`, and `load_versioned` are
function templates, their bodies must be visible at every call site.  They are
therefore defined as inline functions at the bottom of `gmSave.hpp`.  The two
non-template symbols (`EVersionMismatchError` constructor and `peek_version`)
are compiled in `gmSave.cpp` to avoid duplicate-symbol errors.
