---
mode: agent
description: "Scaffold a new class/component (header + source, or language equivalent) for an existing module."
---

# New Component

Scaffold a new class/component for an existing `{{PROJECT_NAME}}` module (`{{MODULE_NAME}}`).

Apply all rules in `.github/instructions/coding-style.instructions.md`.
Apply all documentation rules in `.github/instructions/documentation.instructions.md`.
Validate names against `.github/specs/coding-style.yml`.
Place the files in the correct subfolder according to `.github/specs/module-structure.yml`.

---

## Input Required

1. **Module** — which module does this class/component belong to? (e.g. `{{MODULE_NAME}}`)
2. **Class/component name** — PascalCase (e.g. `PatternRouter`).
3. **Role** — is this a: facade, interface, implementation, config type, exception, bridge adapter?
4. **Base class or interface** — does it inherit/implement something? (e.g. `{{INTERFACE_PREFIX}}Router`)
5. **Public methods** — list each method signature (name, params, return type, const/immutable?).
6. **Private members** — list each member field (type, name, purpose).
7. **Thread-safety** — is this class used concurrently? If yes, describe the locking strategy.
8. **Exceptions/errors** — which exception/error types can its methods throw/raise?

---

## Output Requirements

### Header / declaration file

- Include guard (if `{{USE_INCLUDE_GUARDS}}`): `#ifndef <NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP`.
- File-level doc-comment block at the top.
- All public members with full `{{DOC_COMMENT_STYLE}}` blocks.
- Private members with short inline comments (no full doc-comment block).
- No inline method bodies longer than 2 lines in the header/declaration file, unless the
  language convention favors single-file types.

### Source / implementation file

- Include/import order: own header/module first, standard library, project.
- Stub implementation for each method body with a `// TODO: Phase N — implement` comment
  (adapt comment syntax to `{{LANGUAGE}}`).
- No doc-comments in the implementation file — documentation lives in the
  header/declaration only, unless the language has no separate declaration file.

---

## Constraints

- `{{LANGUAGE_STANDARD}}` only.
- No inferred/implicit return type in public prototypes, if disallowed by project convention.
- External dependencies: `{{ALLOW_EXTERNAL_DEPENDENCIES}}`.
- If the class belongs to a bridge/adapter subfolder, it may reference two modules; otherwise
  reference only symbols from its own module (`{{CROSS_MODULE_POLICY}}`).
