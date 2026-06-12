---
mode: agent
description: "Scaffold a new C++ class (header + source) for an existing game_lib library."
---

# New Class

Scaffold a new C++ class (`.hpp` + `.cpp`) inside an existing `game_lib` library.

Apply all rules in `.github/instructions/cpp-style.instructions.md`.
Apply all Doxygen rules in `.github/instructions/documentation.instructions.md`.
Validate names against `.github/specs/cpp-style.yml`.
Place the files in the correct subfolder according to `.github/specs/library-structure.yml`.

---

## Input Required

1. **Library** — which library does this class belong to? (e.g. `gmDispatch`)
2. **Class name** — PascalCase (e.g. `PatternRouter`).
3. **Role** — is this a: facade, interface, implementation, config struct, exception, bridge adapter?
4. **Base class or interface** — does it inherit from something? (e.g. `IRouter`)
5. **Public methods** — list each method signature (name, params, return type, const?).
6. **Private members** — list each member field (type, name, purpose).
7. **Thread-safety** — is this class used concurrently? If yes, describe the locking strategy.
8. **Exceptions** — which exception types can its methods throw?

---

## Output Requirements

### `ClassName.hpp`

- Include guard: `#ifndef <NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP`.
- File-level Doxygen block immediately after `#define`.
- All public members with full Doxygen `/** @brief ... */` blocks.
- Private members with inline `//` comments (no Doxygen).
- No inline method bodies longer than 2 lines in the header.

### `ClassName.cpp`

- Include order: own header first, stdlib, project.
- Stub implementation for each method body with a `// TODO: Phase N — implement` comment.
- No Doxygen in the `.cpp` — documentation lives in the header only.

---

## Constraints

- C++17 only — no `std::span`, `std::ranges`, concepts, or coroutines.
- No `auto` as a return type in public prototypes.
- No external dependencies.
- If the class belongs to `bridges/`, it may include headers from two libraries;
  otherwise include only headers from the same library.
