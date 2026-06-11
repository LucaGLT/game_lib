# game_lib — Naming and Style Rules

**Version:** 1.0  
**Date:** 2026-06-11  
**Scope:** All C++ libraries in this workspace (`gmLog`, `gmSave`, `gmMap`, `gmDispatch`, `gmDeck`, `gmCompDeck`)

---

## Background — Current Inconsistencies

A quick audit of the codebase reveals three different namespace conventions in active use:

| Library | Current namespace | Style used |
| ------- | ----------------- | ---------- |
| `gmLog` | `GmLog` | PascalCase with `Gm` prefix |
| `gmSave` | `GmSave` | PascalCase with `Gm` prefix |
| `gmDispatch` | `GmDispatch` | PascalCase with `Gm` prefix |
| `gmMap` | `GameMap` | PascalCase, no `gm` prefix, full word |
| `gmDeck` | `gmFate` | camelCase, lowercase `gm`, thematic name |
| `gmCompDeck` | `gmFate` | camelCase, lowercase `gm`, thematic name |

These rules establish a single canonical style to be applied to all **new** code and progressively to refactors.

---

## 1. Namespace Convention

### Rule NS-1 — Prefix `gm`, camelCase

All namespaces use the `gm` prefix in **lowercase**, followed by a short **PascalCase** descriptor.

```
gm<Descriptor>
```

| Library | Correct namespace | Old (to migrate) |
| ------- | ----------------- | ---------------- |
| `gmLog` | `gmLog` | `GmLog` |
| `gmSave` | `gmSave` | `GmSave` |
| `gmDispatch` | `gmDispatch` | `GmDispatch` |
| `gmMap` | `gmMap` | `GameMap` |
| `gmDeck` | `gmDeck` | `gmFate` |
| `gmCompDeck` | `gmDeck` (same library) | `gmFate` |

> **Migration note:** existing libraries keep their current namespace until a dedicated refactor is planned. Do not rename in the middle of a feature branch.

### Rule NS-2 — No nested namespaces for public API

Nested namespaces (e.g. `gm::detail`) are allowed only for internal implementation helpers, never for public-facing types.

```cpp
namespace gmSave {

namespace detail {          // OK — internal only
    // ...
}

class SaveError { ... };    // public API lives at top level

} // namespace gmSave
```

---

## 2. Class and Type Names

### Rule CL-1 — PascalCase for all types

Classes, structs, enums, type aliases, and concepts use **PascalCase** with no prefix.

```cpp
// Correct
class LogRecord { };
struct DispatcherConfig { };
enum class ZoneId { MAIN_DECK, HAND };
using TokenId = uint32_t;

// Wrong
class log_record { };       // snake_case
class gmLogRecord { };      // namespace prefix repeated
class TLogRecord { };       // Hungarian T-prefix
```

### Rule CL-2 — Library prefix on the main façade class only

The primary user-facing class of each library carries the `gm` prefix to avoid collisions when used without `using namespace`:

```cpp
gmLog::gmLogger    // main façade
gmLog::LogRecord   // supporting type — no gm prefix
gmLog::LogLevel    // supporting type — no gm prefix
```

> Current examples that already follow this: `gmDeck`, `gmCompDeck`, `gmMap`.

---

## 3. Function and Method Names

### Rule FN-1 — snake_case for all functions and methods

```cpp
// Correct
void draw_to_hand(int count);
bool is_empty() const;
uint32_t draw_specific(uint32_t token_id);

// Wrong
void DrawToHand(int count);   // PascalCase
void drawToHand(int count);   // camelCase
```

### Rule FN-2 — Boolean queries use `is_`, `has_`, `can_` prefix

```cpp
bool is_empty() const;
bool has_location(LocationId id) const;
bool can_shuffle() const;
```

### Rule FN-3 — Factory free-functions and static factories use `create_` prefix

```cpp
static gmLogger create_file_logger(const LoggerConfig& cfg);
static gmLogger create_stdout_logger(const std::string& name);
```

---

## 4. Variable and Parameter Names

### Rule VAR-1 — snake_case for all local variables and parameters

```cpp
int remaining_count = deck.remaining_count();
const std::string& owner_name = player.owner_name();
```

### Rule VAR-2 — Private member variables use `_` suffix

```cpp
class gmDeck {
private:
    std::vector<uint32_t> _deck;
    std::optional<unsigned int> _seed;
    bool _allow_duplicates;
};
```

> **Not** `m_deck`, `mDeck`, or `deck_` — the suffix `_` is the project standard.

---

## 5. Constants and Enumerators

### Rule CONST-1 — `constexpr` variables use SCREAMING_SNAKE_CASE

```cpp
static constexpr int MAX_HAND_SIZE = 10;
static constexpr bool can_shuffle = false;   // exception: policy flags are property-style
```

### Rule CONST-2 — `enum class` enumerators use SCREAMING_SNAKE_CASE

```cpp
enum class ZoneId {
    MAIN_DECK,
    HAND,
    PLAY_AREA,
    DISCARD,
    BANISHED,
    NOT_FOUND
};
```

---

## 6. File Names

### Rule FILE-1 — File names match the primary class they define, PascalCase

```
Dispatcher.hpp / Dispatcher.cpp     → class Dispatcher
EventBusChannel.hpp                 → class EventBusChannel
PolicyBasedDeck.hpp                 → template class PolicyBasedDeck<Policy>
```

### Rule FILE-2 — Library-named files use lowercase `gm` prefix

```
gmDeck.hpp / gmDeck.cpp
gmMap.hpp  / gmMap.cpp
gmSave.hpp / gmSave.cpp
```

### Rule FILE-3 — Interface headers use `I` prefix, PascalCase

```
IChannel.hpp        → interface IChannel
ILogSink.hpp        → interface ILogSink
IRouter.hpp         → interface IRouter
```

---

## 7. Include Guards

### Rule IG-1 — Pattern `GMLIBNAME_CLASSNAME_HPP`, all uppercase

```cpp
#ifndef GMFATE_GMDECK_HPP
#define GMFATE_GMDECK_HPP
// ...
#endif // GMFATE_GMDECK_HPP
```

Format: `<NAMESPACE_UPPERCASE>_<FILENAME_UPPERCASE>_HPP`

---

## 8. Exception Classes

### Rule EX-1 — Exception names end with `Error`, PascalCase

```cpp
class DeckEmptyError      : public DeckAdapterError { };
class TokenNotFoundError  : public DeckAdapterError { };
class VersionMismatchError: public SaveError        { };
```

### Rule EX-2 — Each library has one base exception class

```
gmLog     → LogError  (base)
gmSave    → SaveError (base)
gmMap     → MapError  (base)
gmDispatch→ DispatchError (base)
gmDeck    → DeckAdapterError (base)
```

---

## 9. Documentation Comments

### Rule DOC-1 — Doxygen for all public API symbols

```cpp
/**
 * @brief One-line summary of what this does.
 *
 * Optional longer description.
 *
 * @param token_id  The token to remove.
 * @return          The removed token ID.
 * @throws TokenNotFoundError if the token is not present.
 */
uint32_t draw_specific(uint32_t token_id);
```

### Rule DOC-2 — Language: English only

All comments, docstrings, and error messages must be written in English.

---

## 10. Summary Quick-Reference

| Element | Convention | Example |
| ------- | ---------- | ------- |
| Namespace | `gm` + PascalCase | `gmLog`, `gmDeck` |
| Façade class | `gm` + PascalCase | `gmDeck`, `gmMap` |
| Supporting class | PascalCase | `LogRecord`, `Envelope` |
| Method / function | snake_case | `draw_to_hand()` |
| Private member | snake_case + `_` suffix | `_allow_duplicates` |
| Parameter / local | snake_case | `token_id`, `owner_name` |
| `enum class` value | SCREAMING_SNAKE_CASE | `MAIN_DECK`, `NOT_FOUND` |
| `constexpr` constant | SCREAMING_SNAKE_CASE | `MAX_HAND_SIZE` |
| File (class) | PascalCase | `EventBusChannel.hpp` |
| File (library entry) | `gm` lowercase + PascalCase | `gmDeck.hpp` |
| Interface file | `I` + PascalCase | `IChannel.hpp` |
| Include guard | NAMESPACE\_FILE\_HPP | `GMFATE_GMDECK_HPP` |
| Exception class | PascalCase + `Error` | `TokenNotFoundError` |
