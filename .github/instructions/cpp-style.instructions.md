---
applyTo: "**/*.{cpp,hpp,h,c,cc,cxx}"
---

# C++ Style Instructions — game_lib

> These instructions apply automatically to every C++ source and header file.
> The authoritative human-readable version is `style-rules.md`.
> The machine-readable version is `.github/specs/cpp-style.yml`.

---

## Naming

| Symbol kind              | Convention          | Example                          |
|--------------------------|---------------------|----------------------------------|
| Namespace                | `gm` + PascalCase   | `gmDeck`, `gmDispatch`           |
| Class / struct / enum    | PascalCase          | `LogRecord`, `DispatcherConfig`  |
| Main façade class        | `Gm` prefix         | `GmLogger`, `GmMap`              |
| Interface header/class   | `I` prefix          | `IChannel`, `ILogSink`           |
| Function / method        | snake_case          | `draw_to_hand()`, `is_empty()`   |
| Boolean query            | `is_`, `has_`, `can_` prefix | `is_valid()`, `has_route()` |
| Factory function         | `create_` prefix    | `create_file_logger()`           |
| Local variable / param   | snake_case          | `remaining_count`, `token_id`    |
| Private member           | `_` prefix          | `_deck`, `_allow_duplicates`     |
| `constexpr` constant     | SCREAMING_SNAKE_CASE | `MAX_HAND_SIZE`                 |
| `enum class` enumerator  | SCREAMING_SNAKE_CASE | `ZoneId::MAIN_DECK`             |
| Exception class          | `E` prefix + PascalCase | `EDeckEmptyError`           |
| Library base exception   | `E` + topic + `Error` | `ESaveError`, `EDispatchError` |

Never use: `auto` as return type in public prototypes, Hungarian prefixes (`m_`, `T`),
`#pragma once` (use `#ifndef` guards only).

---

## Include Guards

```cpp
#ifndef GMLOG_LOGRECORD_HPP
#define GMLOG_LOGRECORD_HPP
// ...
#endif // GMLOG_LOGRECORD_HPP
```

Pattern: `<NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP`

---

## Formatting

- **Indentation:** real tab characters, visual width 4.
- **Braces:** Allman style — opening brace always on its own line.
- **Line limit:** 100 columns.
- **Pointers/references:** `Type* ptr`, `Type& ref` — attached to the type.
- **Single-statement blocks:** allowed without `{}` only for obviously trivial cases.

```cpp
// Correct Allman
if (is_ready())
{
	process();
}

class Dispatcher
{
public:
	void run();
};
```

---

## Spacing

- One space around binary operators: `a + b`, `x == y`, `flags |= mask`.
- No space for unary operators: `!flag`, `++i`, `*ptr`.
- One space after comma, never before: `f(a, b, c)`.
- Space before `(` in control statements, not in calls: `if (x)` vs `f(x)`.

---

## Multiline Signatures

Wrap when the signature exceeds 100 columns.
Use one parameter per line, aligned under the opening `(`.
Put `)` on its own line; `{` follows on the next line (Allman).

```cpp
void process_batch(const std::vector<Event>& events,
                   const DispatcherConfig&   config,
                   bool                      fail_fast
)
{
	// ...
}
```

---

## Switch / Case

`case` is indented one level inside `switch`.
Statements inside each `case` are indented one further level.
Implicit fallthrough is not allowed; use `[[fallthrough]];` when intentional.

```cpp
switch (zone_id)
{
	case ZoneId::HAND:
		process_hand();
		break;

	default:
		throw EMapError("Unknown zone");
}
```

---

## Exception Classes

Each library has exactly one base exception class named `E<Topic>Error`.
All library exceptions inherit from it.

```cpp
class EDispatchError : public std::runtime_error
{
public:
	explicit EDispatchError(const std::string& msg)
		: std::runtime_error(msg) {}
};

class ERouteNotFound : public EDispatchError
{
public:
	explicit ERouteNotFound(const std::string& type_id)
		: EDispatchError("No route for typeId: " + type_id) {}
};
```

---

## Include Order (in `.cpp` files)

1. Corresponding header (first)
2. C++ standard library headers
3. Third-party headers
4. Other project headers

One blank line between groups, alphabetical within each group.

```cpp
#include "Dispatcher.hpp"

#include <mutex>
#include <string>
#include <vector>

#include "dispatchers/SyncDispatcher.hpp"
#include "routers/SyncRouter.hpp"
```

---

## TODO Markers

```cpp
// TODO: Phase 3 — add FileChannel support
```

Phase number is mandatory. Format exactly: `// TODO: Phase N — description`

---

## Constraints

- C++17 only. No C++20 features.
- No external dependencies inside a library folder.
- Cross-library dependencies only via `bridges/` adapters.
- No `#include` of another library's header inside a library header.
