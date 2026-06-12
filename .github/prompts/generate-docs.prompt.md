---
mode: agent
description: "Generate or complete Doxygen documentation blocks for an existing C++ file."
---

# Generate Documentation

Generate or complete Doxygen documentation blocks for an existing C++ file.

Apply all rules in `.github/instructions/documentation.instructions.md`.
Do not change any logic, signatures, or member names — only add or improve comments.

---

## Input Required

1. **File path** — which `.hpp` or `.cpp` file to document?
2. **Scope** — choose one:
   - `all-public` — document every public symbol (classes, methods, enums, type aliases).
   - `missing-only` — add blocks only where no Doxygen comment exists yet.
   - `specific` — document only the symbols listed below.
3. **Symbols** (only if scope is `specific`) — list each symbol name.

---

## Output Requirements

- **Header file:** add or replace comment blocks for every requested symbol.
  - Class/struct: full block with `@brief`, optional long description, `@note` for thread-safety.
  - Method: `@brief`, `@param` for each parameter, `@return`, `@throws` for each thrown type.
  - Enum: `@brief` on the enum; `///<` or full block for each enumerator.
  - File header: `@file` + `@brief` immediately after `#define` guard line.
- **Source file:** do not add Doxygen blocks — documentation lives in the header only.
- Align `@param` descriptions to the column after the longest parameter name in each block.
- Keep `@brief` within 100 columns (including indentation).
- Use `@ref ClassName` or `@ref method_name()` for cross-references, not raw names.

---

## What NOT to Change

- Method signatures, return types, parameter names.
- Include order or include guard macros.
- Any `// TODO:` markers.
- Private member names or types.
