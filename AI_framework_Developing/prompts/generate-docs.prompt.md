---
mode: agent
description: "Generate or complete documentation comment blocks for an existing source file."
---

# Generate Documentation

Generate or complete `{{DOC_COMMENT_STYLE}}` documentation blocks for an existing source file.

Apply all rules in `.github/instructions/documentation.instructions.md`.
Do not change any logic, signatures, or member names — only add or improve comments.

---

## Input Required

1. **File path** — which file to document?
2. **Scope** — choose one:
   - `all-public` — document every public symbol (classes, methods, enums, type aliases).
   - `missing-only` — add blocks only where no documentation comment exists yet.
   - `specific` — document only the symbols listed below.
3. **Symbols** (only if scope is `specific`) — list each symbol name.

---

## Output Requirements

- **Header/declaration file:** add or replace comment blocks for every requested symbol.
  - Class/struct: full block with brief, optional long description, thread-safety note.
  - Method: brief, one entry per parameter, return description, one entry per thrown/raised
    error type.
  - Enum: brief on the enum; short inline or full block for each enumerator.
  - File header: brief block at the top of the file (after the include guard, if applicable).
- **Source/implementation file:** do not add documentation blocks — documentation lives in the
  header/declaration only, unless the language has no separate declaration file.
- Align parameter descriptions to the column after the longest parameter name in each block.
- Keep the brief description within `{{LINE_LENGTH_LIMIT}}` columns (including indentation).
- Use the doc-comment style's native cross-reference syntax, not raw symbol names.

---

## What NOT to Change

- Method signatures, return types, parameter names.
- Include/import order or include-guard macros.
- Any `// TODO:` markers.
- Private member names or types.
