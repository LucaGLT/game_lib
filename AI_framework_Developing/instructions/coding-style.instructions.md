---
applyTo: "**/*.{{{LANGUAGE_FILE_EXTENSIONS}}}"
---

# Coding Style Instructions — {{PROJECT_NAME}}

> These instructions apply automatically to every `{{LANGUAGE}}` source and header/module file.
> The machine-readable version is `.github/specs/coding-style.yml`.
> Reference example syntax below uses C++-style code; adapt syntax to `{{LANGUAGE}}` idioms while
> preserving the same naming/formatting semantics.

---

## Naming

| Symbol kind              | Convention                                | Example                          |
|---------------------------|-------------------------------------------|-----------------------------------|
| Namespace/module          | `{{NAMESPACE_PATTERN}}`                    | `{{NAMESPACE_EXAMPLE}}`          |
| Class / struct / enum     | project convention (see `coding-style.yml`) | *(fill in during setup)*        |
| Main façade class         | `{{FACADE_PREFIX}}` prefix (if used)       | `{{FACADE_PREFIX}}{{MODULE_NAME}}` |
| Interface header/class    | `{{INTERFACE_PREFIX}}` prefix              | `{{INTERFACE_PREFIX}}Channel`    |
| Function / method         | `{{FUNCTION_NAMING_CONVENTION}}`           | `draw_to_hand()`, `is_empty()`   |
| Boolean query              | `{{BOOLEAN_QUERY_PREFIXES}}` prefix        | `is_valid()`, `has_route()`      |
| Factory function           | `{{FACTORY_PREFIX}}` prefix                | `create_file_logger()`           |
| Local variable / param     | project convention                        | *(fill in during setup)*         |
| Private member              | `{{PRIVATE_MEMBER_CONVENTION}}`           | `_deck`, `_allow_duplicates`      |
| Constant                    | `{{CONSTANT_NAMING_CONVENTION}}`          | `MAX_HAND_SIZE`                  |
| Enum enumerator             | `{{CONSTANT_NAMING_CONVENTION}}`          | `ZoneId::MAIN_DECK`              |
| Exception class             | `{{EXCEPTION_PREFIX}}` prefix             | `{{EXCEPTION_PREFIX}}DeckEmptyError` |
| Module base exception       | `{{EXCEPTION_PREFIX}}` + topic + `{{EXCEPTION_BASE_SUFFIX}}` | `{{EXCEPTION_PREFIX}}SaveError` |

Never use: inferred return types in public prototypes (if disallowed by the project convention),
Hungarian prefixes (`m_`, `T`), or a header-guard style other than the one confirmed during setup.

---

## Include Guards / Module Boundaries (conditional — only if `{{USE_INCLUDE_GUARDS}}`)

```cpp
#ifndef {{MODULE_PREFIX_UPPER}}_EXAMPLETYPE_HPP
#define {{MODULE_PREFIX_UPPER}}_EXAMPLETYPE_HPP
// ...
#endif // {{MODULE_PREFIX_UPPER}}_EXAMPLETYPE_HPP
```

Pattern: `<NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP` (or the equivalent module-boundary mechanism
for `{{LANGUAGE}}`, e.g. explicit exports/module declarations).

---

## Formatting

- **Indentation:** `{{INDENT_STYLE}}`, visual width `{{INDENT_WIDTH}}`.
- **Braces:** `{{BRACE_STYLE}}` — apply consistently to all constructs, if the language uses
  brace-delimited blocks.
- **Line limit:** `{{LINE_LENGTH_LIMIT}}` columns.
- **Pointers/references (if applicable):** attach symbol to the type consistently.
- **Single-statement blocks:** allowed without braces only for obviously trivial cases.

```cpp
// Example (Allman reference)
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

Wrap when the signature exceeds `{{LINE_LENGTH_LIMIT}}` columns.
Use one parameter per line, aligned under the opening bracket.
Closing bracket / opening brace placement follows `{{BRACE_STYLE}}`.

```cpp
void process_batch(const std::vector<Event>& events,
                   const Config&             config,
                   bool                      fail_fast
)
{
	// ...
}
```

---

## Control Flow Constructs (conditional)

`case`/`switch`-equivalent blocks are indented one level; statements inside each case are
indented one further level. Implicit fallthrough is not allowed; use an explicit marker
(e.g. `[[fallthrough]];`) when intentional.

```cpp
switch (zone_id)
{
	case ZoneId::MAIN:
		process_main();
		break;

	default:
		throw {{EXCEPTION_PREFIX}}{{MODULE_NAME}}{{EXCEPTION_BASE_SUFFIX}}("Unknown zone");
}
```

---

## Exception Classes

Each module has exactly one base exception class named
`{{EXCEPTION_PREFIX}}<Topic>{{EXCEPTION_BASE_SUFFIX}}`. All module-specific exceptions inherit
from it.

```cpp
class {{EXCEPTION_PREFIX}}{{MODULE_NAME}}{{EXCEPTION_BASE_SUFFIX}} : public std::runtime_error
{
public:
	explicit {{EXCEPTION_PREFIX}}{{MODULE_NAME}}{{EXCEPTION_BASE_SUFFIX}}(const std::string& msg)
		: std::runtime_error(msg) {}
};
```

---

## Include / Import Order

1. Corresponding header/module (first)
2. Standard library
3. Third-party (only if `{{ALLOW_EXTERNAL_DEPENDENCIES}}` is `true`)
4. Other project modules

One blank line between groups, alphabetical within each group.

---

## TODO Markers

```text
// TODO: Phase 3 — add FileChannel support
```

Phase number is mandatory. Format exactly: `// TODO: Phase N — description` (adapt comment
syntax to `{{LANGUAGE}}`).

---

## Constraints

- `{{LANGUAGE_STANDARD}}` only — no unapproved later-version language features.
- External dependencies: `{{ALLOW_EXTERNAL_DEPENDENCIES}}`.
- Cross-module dependencies only via: `{{CROSS_MODULE_POLICY}}`.
- No direct inclusion of another module's internals outside the agreed cross-module policy.
