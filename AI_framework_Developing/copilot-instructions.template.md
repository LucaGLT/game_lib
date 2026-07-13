<!--
  TEMPLATE NOTE — remove this comment block during instantiation.
  This file becomes `.github/copilot-instructions.md` after every placeholder token
  (double-curly-brace syntax, e.g. {{PROJECT_NAME}}) has been resolved and substituted.
-->

# {{PROJECT_NAME}} — GitHub Copilot Workspace Instructions

**Client:** {{CLIENT_NAME}}
**Scope:** All files in this workspace.

---

## Project Overview

`{{PROJECT_NAME}}` is developed for **{{CLIENT_NAME}}**.

This module/library in scope:

| Module          | Namespace              | Purpose                                  |
|-----------------|-------------------------|-------------------------------------------|
| `{{MODULE_NAME}}` | `{{NAMESPACE_EXAMPLE}}` | *(one-sentence purpose — fill in during setup)* |

> If the repository hosts multiple modules, add one row per module and keep this table in sync
> with `PROJECT_PARAMETERS.md`.

---

## Key Reference Files

| Topic                        | File                                       |
|------------------------------|--------------------------------------------|
| Coding style rules            | `.github/instructions/coding-style.instructions.md` |
| Machine-readable style rules  | `.github/specs/coding-style.yml`            |
| Module structure schema       | `.github/specs/module-structure.yml`        |
| `PLAN.md` schema               | `.github/specs/plan-schema.yml`             |
| Review plan schema             | `.github/specs/review-plan-schema.yml`      |
| Planning instructions          | `.github/instructions/planning.instructions.md` |
| Documentation instructions     | `.github/instructions/documentation.instructions.md` |
| Markdown style instructions    | `.github/instructions/markdown-style.instructions.md` |
| Code review instructions       | `.github/instructions/code-review.instructions.md` |
| Environment setup instructions | `.github/instructions/environment-setup.instructions.md` |

<!-- If {{HAS_GUI_MODULE}} == true, also add: -->
<!--
| GUI theme spec (machine-readable) | `.github/specs/gui-theme.yml` |
| Python/Qt GUI style instructions   | `.github/instructions/gui-style.instructions.md` |
-->

---

## Permanent Rules (apply to every response)

1. **Language:** {{LANGUAGE_STANDARD}} — do not use later/unsupported language features.
2. **External dependencies:** {{ALLOW_EXTERNAL_DEPENDENCIES}} (`false` = standard library/runtime only).
3. **Types:** Always use explicit types where the language supports/requires them; avoid inferred
   return types in public prototypes unless the language idiom favors it.
4. **Naming:** Follow `.github/instructions/coding-style.instructions.md` exactly. Consult
   `.github/specs/coding-style.yml` for the machine-readable version.
5. **Formatting:** Indentation `{{INDENT_STYLE}}` (width `{{INDENT_WIDTH}}`), braces
   `{{BRACE_STYLE}}` (if applicable), `{{LINE_LENGTH_LIMIT}}`-column line limit.
6. **Style (Markdown):** Follow markdownlint conventions. Use spaces (not tabs) for Markdown
   indentation.
7. **Cross-module links:** {{CROSS_MODULE_POLICY}}.
8. **Error handling:** Each module has one base exception class
   (`{{EXCEPTION_PREFIX}}<Topic>{{EXCEPTION_BASE_SUFFIX}}`). Throw on contract violations, not as
   control flow.
9. **TODO markers:** Use `// TODO: Phase N — description` format (adapt comment syntax to
   `{{LANGUAGE}}`). Phase number is mandatory.
10. **Include guards / module boundaries:** Only if `{{USE_INCLUDE_GUARDS}}` is `true` —
    `#ifndef NAMESPACE_FILENAME_HPP` style guards (no `#pragma once`).

---

## When Planning New Development

Follow `.github/instructions/planning.instructions.md` and use the YAML schema in
`.github/specs/plan-schema.yml`.

Use the reusable prompts: `.github/prompts/new-module.prompt.md` or
`.github/prompts/update-plan.prompt.md`.

---

## When Writing or Reviewing Code

Apply `.github/instructions/coding-style.instructions.md` automatically for `{{LANGUAGE}}` source
files.

Use `.github/prompts/new-component.prompt.md` to scaffold a new class/component from scratch.

---

## When Writing Documentation

Apply `.github/instructions/documentation.instructions.md` for any `{{DOC_COMMENT_STYLE}}`
comment block.

Always create or update a human-written API manual in Markdown when public
classes/functions are added or changed. The API manual must live in the module
root (for example `{{MODULE_ROOT_PATH}}{{MODULE_NAME}}{{API_DOC_SUFFIX}}`) and must document
class purpose, public methods, parameters, return values, errors, and a minimal usage example.
Use Mermaid diagrams for architecture/flow sections.

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

## When Preparing a New Environment

Apply `.github/instructions/environment-setup.instructions.md` whenever
`PROJECT_PARAMETERS.md` (or `.yml`) is created or updated.

Use `.github/prompts/setup-environment.prompt.md` to (re-)interview the Designer and
(re-)instantiate this framework.

<!-- If {{HAS_GUI_MODULE}} == true, also add a "When Writing GUI Code" section referencing
     gui-style.instructions.md, gui-theme.yml, new-gui-widget/new-gui-theme/apply-gui-theme
     prompts (see optional-modules/gui-theming/README.md for the exact block to paste). -->
