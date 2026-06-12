# game_lib — Naming and Style Rules

**Version:** 1.2  
**Date:** 2026-06-12  
**Scope:** All C++ libraries in this workspace

---

## 1. Namespace Convention

### Rule NS-1 — Prefix `gm`, camelCase

All namespaces use the `gm` prefix in **lowercase**, followed by a short **PascalCase** descriptor.

```
gm<Descriptor>
```


### Rule NS-2 — No nested namespaces for public API

Nested namespaces (e.g. `gm::detail` , `gm::helper`) are allowed only for internal implementation helpers, never for public-facing types.

```cpp
namespace gmSave {

namespace helper{          // OK — internal only
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

The primary user-facing class of each library carries the `Gm` prefix to avoid collisions when used without `using namespace`:

```cpp
gmLog::GmLogger    // main façade
gmLog::LogRecord   // supporting type — no gm prefix
gmLog::LogLevel    // supporting type — no gm prefix
```

> Examples : `GmDeck`, `GmCompDeck`, `GmMap`.

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
static GmLogger create_file_logger(const LoggerConfig& cfg);
static GmLogger create_stdout_logger(const std::string& name);
```

---

## 4. Variable and Parameter Names

### Rule VAR-1 — snake_case for all local variables and parameters

```cpp
int remaining_count = deck.remaining_count();
const std::string& owner_name = player.owner_name();
```

### Rule VAR-2 — Private member variables use `_` prefix

```cpp
class GmDeck {
private:
    std::vector<uint32_t> _deck;
    std::optional<unsigned int> _seed;
    bool _allow_duplicates;
};
```

> **Not** `m_deck`, `mDeck`, or `deck_` — the prefix `_` is the project standard.

---

## 5. Constants and Enumerators

### Rule CONST-1 — `constexpr` variables use SCREAMING_SNAKE_CASE

```cpp
static constexpr int MAX_HAND_SIZE = 10;
static constexpr bool CAN_SHUFFLE = false;   
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
GmDispatcher.hpp / GMDispatcher.cpp     → class Dispatcher
EventBusChannel.hpp                     → class EventBusChannel
PolicyBasedDeck.hpp                     → template class PolicyBasedDeck<Policy>
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
#ifndef GMALEA_GMDECK_HPP
#define GMALEA_GMDECK_HPP
// ...
#endif // GMALEA_GMDECK_HPP
```

Format: `<NAMESPACE_UPPERCASE>_<FILENAME_UPPERCASE>_HPP`

---

## 8. Exception Classes

### Rule EX-1 — Exception names have prefix 'E' and, if not clear, end with `Error`, PascalCase

```cpp
class EDeckEmptyError  : public EDeckAdapterError { };
class ETokenNotFound   : public EDeckAdapterError { };
class EVersionMismatch : public ESaveError        { };
```

### Rule EX-2 — Each library has one only base exception class that end with `Error`, PascalCase

Format: `<libName without 'gm'><Error>`

```
gmLog     → ELogError  (base)
gmSave    → ESaveError (base)
gmMap     → EMapError  (base)
gmDispatch→ EDispatchError (base)
gmDeck    → EDeckAdapterError (base)
```

---

## 9. Documentation Comments

### Rule DOC-1 — Doxygen style for all public API symbols

```cpp
/**
 * @brief One-line summary of what this does.
 *
 * Optional longer description.
 *
 * @param token_id  The token to remove.
 * @return          The removed token ID.
 * @throws ETokenNotFound if the token is not present.
 */
uint32_t draw_specific(uint32_t token_id);
```

### Rule DOC-2 — Language: English only

All comments, docstrings, and error messages must be written in English.

---

## 10. Formatting and Indentation

### Rule FMT-1 — Use tabs for indentation

Use real tab characters for each indentation level.

- Tab width: 4 spaces (editor visualization only).
- Do not mix tabs and spaces for indentation in the same block.
- Spaces are allowed for alignment after indentation.

### Rule FMT-2 — Allman braces

Opening braces go on a new line for all control structures, functions, classes, structs, enums,
and namespaces.

```cpp
if (is_ready())
{
    draw_card();
}

class Dispatcher
{
public:
    void run()
    {
        // ...
    }
};
```

### Rule FMT-3 — Maximum line length

Keep lines at a maximum of 100 characters when practical.

- If needed, wrap function parameters and long expressions across multiple lines.
- Prefer readability over clever compact formatting.

### Rule FMT-4 — Pointer and reference style

Attach `*` and `&` to the type, not to the variable name.

```cpp
Token* token = nullptr;
const std::string& name = config.name();
```

### Rule FMT-5 — Single-statement control blocks

Single-line statements without braces are allowed only for truly simple cases.

```cpp
if (is_cached()) return;
```

If there is any chance of future growth or ambiguity, use braces.

### Rule FMT-6 — Vertical alignment

Vertical alignment is allowed when it improves readability.

```cpp
const int         retries = 3;
const std::string source  = "disk";
const bool        cached  = false;
```

Avoid excessive re-alignment churn in unrelated edits.

---

## 11. `switch` and `case` Formatting

### Rule SW-1 — `case` indentation

Indent each `case`/`default` one level inside `switch`.
Indent statements inside each case one additional level.

```cpp
switch (zone_id)
{
    case ZoneId::MAIN_DECK:
        handle_main_deck();
        break;

    case ZoneId::HAND:
        handle_hand();
        break;

    default:
        throw EMapError("Unknown zone");
}
```

### Rule SW-2 — Blocked cases when needed

Use braces inside a `case` when declaring variables with non-trivial lifetime or to scope helpers.

```cpp
switch (event_type)
{
    case EventType::DRAW:
    {
        DrawEvent payload = parse_draw_event(raw);
        process_draw(payload);
        break;
    }

    default:
        break;
}
```

### Rule SW-3 — Fallthrough

Implicit fallthrough is not allowed.
If fallthrough is intentional, annotate explicitly with `[[fallthrough]];`.

---

## 12. Includes and Preprocessor Formatting

### Rule INC-1 — Include order

Use this include order in `.cpp` files:

1. Corresponding header first
2. C++ standard library headers
3. Third-party headers
4. Project headers

Keep one blank line between groups and sort includes alphabetically inside each group.

### Rule INC-2 — Include style

- Use `#include "..."` for project headers.
- Use `#include <...>` for standard and third-party headers.
- Do not use relative include paths containing `..` unless unavoidable.

### Rule PP-1 — Preprocessor indentation and layout

Keep preprocessor directives starting at column 1.
Do not indent `#if`, `#ifdef`, `#else`, `#elif`, `#endif`.

```cpp
#if defined(GMLOG_ENABLE_FILE_SINK)
#include "sinks/FileSink.hpp"
#else
#include "sinks/NullSink.hpp"
#endif
```

### Rule PP-2 — Closing comments for complex conditionals

For long or nested preprocessor blocks, add a trailing comment on `#endif`.

```cpp
#if defined(GMLOG_USE_SYSLOG)
// ...
#endif // GMLOG_USE_SYSLOG
```

