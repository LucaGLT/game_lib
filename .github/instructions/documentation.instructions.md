---
applyTo: "**/*.{cpp,hpp,h}"
---

# Documentation Instructions — game_lib

> Apply these instructions whenever writing or reviewing Doxygen comment blocks.
> Language: English only (Rule DOC-2).

---

## When to Write Doxygen Comments

Write a full Doxygen block for every:
- Public class or struct.
- Public method or free function.
- Public type alias (`using`, `typedef`).
- `enum class` and its enumerators (at minimum a `@brief` per value).

Do NOT write Doxygen blocks for:
- Private or `protected` members (inline `//` comment is sufficient).
- Implementation-only helper types inside `namespace detail`.
- Trivial getters/setters (one-liner `///< description` is enough).

---

## Block Format

```cpp
/**
 * @brief One concise sentence describing what this does (ends with `.`).
 *
 * Optional longer description. Explain the why, not just the what.
 * Use plain English; no abbreviations without definition.
 *
 * @param name_of_param  Description of the parameter.
 * @param second_param   Another description.
 * @return               What the function returns (omit for `void`).
 * @throws ELibError     When this error condition occurs.
 * @note                 Optional implementation note or warning.
 */
ReturnType method_name(ParamType name_of_param, ParamType second_param);
```

Alignment:
- Align `@param` descriptions to the column after the longest parameter name.
- One blank line between `@brief` and the optional long description.
- One blank line between the long description and the first `@param`.

---

## Class / Struct Block

```cpp
/**
 * @brief Short description of the class responsibility.
 *
 * Longer description if needed. Include:
 *   - the role this class plays in the library architecture,
 *   - thread-safety guarantees (or lack thereof),
 *   - ownership model for any resources held.
 *
 * @note Not thread-safe unless stated otherwise.
 */
class DispatcherConfig
{
    // ...
};
```

---

## Interface Block

```cpp
/**
 * @brief Pure-virtual interface for output channels.
 *
 * Implementors receive @ref Envelope objects via @ref send() and forward
 * them to a concrete output destination (stdout, file, socket, etc.).
 *
 * @note Each implementation must be thread-safe if used with AsyncDispatcher.
 */
class IChannel
{
public:
	virtual ~IChannel() = default;

	/**
	 * @brief Sends an envelope to this channel's output destination.
	 *
	 * @param envelope  The message envelope to deliver.
	 */
	virtual void send(const Envelope& envelope) = 0;
};
```

---

## Enum Block

```cpp
/**
 * @brief Identifies the deck zones used by the game engine.
 */
enum class ZoneId
{
	MAIN_DECK, ///< Cards not yet drawn.
	HAND,      ///< Cards currently in the player's hand.
	DISCARD,   ///< Cards removed from play.
	NOT_FOUND  ///< Sentinel value returned when a zone is not found.
};
```

Use `///<` inline for short enumerators. Use a full block `/** @brief ... */`
for enumerators with non-obvious meaning.

---

## Exception Class Block

```cpp
/**
 * @brief Base exception for all gmDispatch errors.
 *
 * Catch this type to handle any error originating from the gmDispatch library.
 */
class EDispatchError : public std::runtime_error
{
public:
	/**
	 * @brief Constructs the exception with a diagnostic message.
	 * @param msg  Human-readable description of the error.
	 */
	explicit EDispatchError(const std::string& msg)
		: std::runtime_error(msg) {}
};
```

---

## `@throws` Rules

- Document every exception the function can throw, including inherited ones.
- Use the concrete exception class name, not just the base.
- Describe the condition that triggers the throw.

```cpp
 * @throws ERouteNotFound  If no channel is subscribed to the given typeId.
 * @throws EDispatchError  If the internal router is in an inconsistent state.
```

---

## File-Level Block (top of every header)

Place a minimal file block immediately after the include guard `#define`:

```cpp
#ifndef GMDISPATCH_IDISPATCHER_HPP
#define GMDISPATCH_IDISPATCHER_HPP

/**
 * @file  IDispatcher.hpp
 * @brief Top-level dispatch interface for the gmDispatch library.
 */
```

---

## Style Constraints

- `@brief` must fit on one line (≤ 100 columns including indent and `* `).
- No HTML tags in Doxygen comments.
- Cross-references use `@ref ClassName` or `@ref method_name()`.
- Group related free functions with `@defgroup` / `@addtogroup` only when they
  form a logical API cluster (e.g. factory functions).

---

## API Markdown Companion (Required)

Whenever you add or modify a public API (`class`, `struct`, public method,
public free function), you MUST also create or update a Markdown API manual.

Rules:
- The file must be stored in the module root and follow `<module>_API.md` naming
	when possible (example: `gmFlow/gmFlow_API.md`).
- The file must be human-readable and complete without requiring Doxygen tool
	execution or HTML/PDF output.
- Architecture and flow diagrams in the API manual must be written in Mermaid
	fenced blocks (no ASCII/text diagrams for those sections).
- At minimum, include:
	- Overview and scope.
	- Mermaid architecture/flow diagrams.
	- Public classes/functions.
	- Method signatures with parameter and return semantics.
	- Function-level details extracted from Doxygen comments (`@brief`, detailed
		behaviour, `@param`, `@return`, `@throws` when present).
	- Error/exception behaviour.
	- Minimal usage examples.
- Keep this file in sync with source updates; treat API Markdown as part of the
	definition of done.
