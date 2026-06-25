# game_lib — GitHub Copilot Workspace Instructions

**Scope:** All files in this workspace.

---

## Project Overview

`game_lib` is a collection of independent C++17 game-engine libraries:

| Library       | Namespace      | Purpose                                        |
|---------------|----------------|------------------------------------------------|
| `gmAlea`      | `gmAlea`       | Token deck management (GmDeck, GmCompDeck, GmDice) |
| `gmDispatch`  | `gmDispatch`   | Generic message/event dispatch (sync + async)  |
| `gmFlow`      | `gmFlow`       | Flow controller and timeline engine            |
| `gmLog`       | `gmLog`        | Structured logging                             |
| `gmMap`       | `gmMap`        | Spatial map management                         |
| `gmRules`     | `gmRules`      | Game rules engine                              |
| `gmSave`      | `gmSave`       | Persistence and snapshot                       |
| `gmGui`       | —              | PySide6 GUI front-end for game-master tooling  |

Each library is fully standalone (no cross-library `#include` except through explicit bridge adapters).

---

## Key Reference Files

| Topic                        | File                                       |
|------------------------------|--------------------------------------------|
| Naming and style rules       | `style-rules.md`                           |
| Machine-readable style rules | `.github/specs/cpp-style.yml`              |
| Library structure schema     | `.github/specs/library-structure.yml`      |
| PLAN.md schema               | `.github/specs/plan-schema.yml`            |
| Review plan schema           | `.github/specs/review-plan-schema.yml`     |
| C++ style instructions       | `.github/instructions/cpp-style.instructions.md` |
| Markdown style instructions  | `.github/instructions/markdown-style.instructions.md` |
| Planning instructions        | `.github/instructions/planning.instructions.md`  |
| Documentation instructions   | `.github/instructions/documentation.instructions.md` |
| Code review instructions     | `.github/instructions/code-review.instructions.md` |
| GUI visual style source      | `.github/GUI style.md`                             |
| Python GUI style instructions | `.github/instructions/python-gui-style.instructions.md` |
| GUI theme spec (machine-readable) | `.github/specs/gui-theme.yml`               |

---

## Permanent Rules (apply to every response)

1. **Language:** C++17 standard — no C++20 or later features.
2. **No external dependencies** — only the C++17 standard library inside each lib.
3. **Types:** Always use explicit types; never use `auto` as a return type in public prototypes.
4. **Naming:** Follow `style-rules.md` exactly. Consult `.github/specs/cpp-style.yml` for the machine-readable version.
5. **Style (C++ only):** Tabs for indentation (width 4), Allman braces, 100-column line limit.
6. **Style (Markdown):** Follow markdownlint conventions. Use spaces (not tabs) for Markdown indentation.
7. **Cross-library links:** Add adapters/bridges in a `bridges/` subfolder; never add `#include` of another library into a library header.
8. **Error handling:** Each library has one base exception class (Rule EX-2). Throw on contract violations, not as control flow.
9. **TODO markers:** Use `// TODO: Phase N — description` format. Phase number is mandatory.
10. **Include guards:** `#ifndef NAMESPACE_FILENAME_HPP` (Rule IG-1). `#pragma once` is not used.

---

## When Planning New Development

Follow `.github/instructions/planning.instructions.md` and use the YAML schema in
`.github/specs/plan-schema.yml`.

Use the reusable prompt: `.github/prompts/new-library.prompt.md` or `update-plan.prompt.md`.

---

## When Writing or Reviewing Code

Apply `.github/instructions/cpp-style.instructions.md` automatically for any `*.cpp` or `*.hpp` file.

Use `.github/prompts/new-class.prompt.md` to scaffold a new class from scratch.

---

## When Writing Documentation

Apply `.github/instructions/documentation.instructions.md` for any Doxygen comment block.

Always create or update a human-written API manual in Markdown when public
classes/functions are added or changed. The API manual must live in the module
root (for example `gmFlow/gmFlow_API.md`) and must document class purpose,
public methods, parameters, return values, errors, and a minimal usage example.
Use Mermaid diagrams for architecture/flow sections. Extract function-level
details from Doxygen comments (`@brief`, `@param`, `@return`, `@throws`) into
the API Markdown reference tables.
Do not require PDF/HTML generation and do not require running Doxygen tools for
this deliverable.

For Markdown files (`*.md`), apply `.github/instructions/markdown-style.instructions.md`
and enforce markdownlint-compatible formatting.

Use `.github/prompts/generate-docs.prompt.md` to generate docs for an existing file.

---

## When Reviewing Existing Code

Apply `.github/instructions/code-review.instructions.md` to analyse compliance.
Use `.github/specs/review-plan-schema.yml` for the output structure.

Use the reusable prompt: `.github/prompts/review-code.prompt.md`.

The review never modifies source files. It always produces a `REVIEW.md` correction plan first.

---

## When Writing Python GUI Code

Apply `.github/instructions/python-gui-style.instructions.md` automatically for any `*.py` file.
Validate theme token usage against `.github/specs/gui-theme.yml`.

Use `.github/prompts/new-gui-widget.prompt.md` to scaffold a new PySide6 widget.
Use `.github/prompts/new-gui-theme.prompt.md` to define a new visual theme.
Use `.github/prompts/apply-gui-theme.prompt.md` to audit or refactor an existing widget for compliance.
