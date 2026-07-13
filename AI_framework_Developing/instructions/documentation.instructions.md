---
applyTo: "**/*.{{{LANGUAGE_FILE_EXTENSIONS}}}"
---

# Documentation Instructions — {{PROJECT_NAME}}

> Apply these instructions whenever writing or reviewing `{{DOC_COMMENT_STYLE}}` comment blocks.
> Language: English only (Rule DOC-2), unless the project explicitly overrides this.

---

## When to Write Doc-Comments

Write a full `{{DOC_COMMENT_STYLE}}` block for every:

- Public class or struct.
- Public method or free function.
- Public type alias.
- Enum type and its enumerators (at minimum a brief per value).

Do NOT write full doc-comment blocks for:

- Private or internal-only members (a short inline comment is sufficient).
- Implementation-only helper types inside an internal/detail namespace or module.
- Trivial getters/setters (one-liner description is enough).

---

## Block Format (reference example — adapt syntax to `{{DOC_COMMENT_STYLE}}`)

The example below uses Doxygen syntax as the reference implementation. If
`{{DOC_COMMENT_STYLE}}` is different (e.g. JSDoc, Python docstring, JavaDoc, XML doc), keep the
same required content — brief, parameters, return, thrown errors — translated into that
convention's syntax.

```cpp
/**
 * @brief One concise sentence describing what this does (ends with `.`).
 *
 * Optional longer description. Explain the why, not just the what.
 * Use plain English; no abbreviations without definition.
 *
 * @param name_of_param  Description of the parameter.
 * @param second_param   Another description.
 * @return               What the function returns (omit for void).
 * @throws {{EXCEPTION_PREFIX}}LibError  When this error condition occurs.
 * @note                 Optional implementation note or warning.
 */
ReturnType method_name(ParamType name_of_param, ParamType second_param);
```

Alignment:

- Align parameter descriptions to the column after the longest parameter name.
- One blank line between the brief and the optional long description.
- One blank line between the long description and the first parameter entry.

---

## Class / Struct Block

```cpp
/**
 * @brief Short description of the class responsibility.
 *
 * Longer description if needed. Include:
 *   - the role this class plays in the module architecture,
 *   - thread-safety guarantees (or lack thereof),
 *   - ownership model for any resources held.
 *
 * @note Not thread-safe unless stated otherwise.
 */
class ExampleType
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
 * Implementors receive a message object via send() and forward it to a concrete output
 * destination (stdout, file, socket, etc.).
 *
 * @note Each implementation must be thread-safe if used concurrently.
 */
class {{INTERFACE_PREFIX}}Channel
{
public:
	virtual ~{{INTERFACE_PREFIX}}Channel() = default;

	/**
	 * @brief Sends a message to this channel's output destination.
	 *
	 * @param message  The message to deliver.
	 */
	virtual void send(const Message& message) = 0;
};
```

---

## Enum Block

```cpp
/**
 * @brief Identifies the zones used by the engine.
 */
enum class ZoneId
{
	MAIN, ///< Default zone.
	SECONDARY, ///< Secondary zone.
	NOT_FOUND  ///< Sentinel value returned when a zone is not found.
};
```

Use a short inline description for simple enumerators. Use a full block for enumerators with
non-obvious meaning.

---

## Exception Class Block

```cpp
/**
 * @brief Base exception for all {{MODULE_NAME}} errors.
 *
 * Catch this type to handle any error originating from the {{MODULE_NAME}} module.
 */
class {{EXCEPTION_PREFIX}}{{MODULE_NAME}}{{EXCEPTION_BASE_SUFFIX}} : public std::runtime_error
{
public:
	/**
	 * @brief Constructs the exception with a diagnostic message.
	 * @param msg  Human-readable description of the error.
	 */
	explicit {{EXCEPTION_PREFIX}}{{MODULE_NAME}}{{EXCEPTION_BASE_SUFFIX}}(const std::string& msg)
		: std::runtime_error(msg) {}
};
```

---

## `@throws` / Error Documentation Rules

- Document every exception/error the function can raise, including inherited ones.
- Use the concrete exception/error type name, not just the base.
- Describe the condition that triggers the error.

---

## File-Level Block (top of every source/header file)

Place a minimal file block at the top of the file (after the include guard `#define`, if
applicable):

```cpp
/**
 * @file  ExampleType.hpp
 * @brief Top-level description of what this file provides for {{MODULE_NAME}}.
 */
```

---

## Style Constraints

- Brief description must fit on one line (≤ `{{LINE_LENGTH_LIMIT}}` columns including indent).
- No HTML tags in doc-comments.
- Cross-references use the doc-comment style's native reference syntax (e.g. `@ref ClassName`).
- Group related free functions only when they form a logical API cluster.

---

## API Markdown Companion (Required)

Whenever you add or modify a public API (class, struct, public method, public free function),
you MUST also create or update a Markdown API manual.

Rules:

- The file must be stored in the module root and follow `{{MODULE_NAME}}{{API_DOC_SUFFIX}}`
  naming (example: `{{MODULE_ROOT_PATH}}{{MODULE_NAME}}{{API_DOC_SUFFIX}}`).
- The file must be human-readable and complete without requiring any documentation-generator
  tool execution or HTML/PDF output.
- Architecture and flow diagrams in the API manual must be written in Mermaid fenced blocks (no
  ASCII/text diagrams for those sections).
- At minimum, include:
  - Overview and scope.
  - Mermaid architecture/flow diagrams.
  - Public classes/functions.
  - Method signatures with parameter and return semantics.
  - Function-level details extracted from doc-comments (brief, detailed behaviour, parameters,
    return, thrown errors).
  - Error/exception behaviour.
  - Minimal usage examples.
- Keep this file in sync with source updates; treat the API Markdown as part of the definition
  of done.
