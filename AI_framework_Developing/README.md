# AI Framework Developing — Reusable Template

> Source: extracted and generalized from `game_lib/.github` (see `../AI-Framework.md` for the
> original analysis report). This folder is a **template staging area**, not an active
> Copilot configuration.

---

## Purpose

This folder contains a **parametric, project-agnostic AI framework**: instructions, prompts and
machine-readable specs that were identified as reusable across any software project or module,
regardless of the specific product (`game_lib`) they were extracted from.

Everything here uses **placeholder tokens** (e.g. `{{PROJECT_NAME}}`) instead of hard-coded
project values. The canonical list of tokens — with description, default value and whether they
are required — lives in [`specs/parameters.yml`](specs/parameters.yml).

## Why This Is Not Active Yet

Files in this folder intentionally reuse the same naming/format as real Copilot customization
files (`*.instructions.md`, `*.prompt.md`, `*.yml` specs). They are **not** picked up
automatically because they live outside `.github/`. They become active only after being
**instantiated** (placeholders resolved) into a target project's `.github/` folder.

## Quickstart

1. Open the target project/repository (can be `myname_lib` itself for a new module, or a brand
   new repository).
2. Run the prompt [`prompts/setup-environment.prompt.md`](prompts/setup-environment.prompt.md).
3. Answer the interview questions (Project, Client, Module/Library name, acronym, language,
   style, documentation convention, optional GUI module, etc.) — this is governed by
   [`instructions/environment-setup.instructions.md`](instructions/environment-setup.instructions.md).
4. The prompt writes a `PROJECT_PARAMETERS.md` file at the project root and then copies every
   template file into `.github/`, substituting every placeholder token with its confirmed value
   (`specs/parameters.yml` itself is the registry and is not copied).
5. From that point on, the generated `.github/instructions`, `.github/prompts` and
   `.github/specs` behave exactly like any hand-written Copilot customization set.

## Folder Map

| Path | Content |
|------|---------|
| [`copilot-instructions.template.md`](copilot-instructions.template.md) | Parametric root workspace policy (becomes `.github/copilot-instructions.md`) |
| [`specs/parameters.yml`](specs/parameters.yml) | Canonical registry of every placeholder token |
| [`specs/plan-schema.yml`](specs/plan-schema.yml) | Schema for `PLAN.md` roadmap files |
| [`specs/review-plan-schema.yml`](specs/review-plan-schema.yml) | Schema for `REVIEW.md` code-review output |
| [`specs/coding-style.yml`](specs/coding-style.yml) | Machine-readable coding/naming/formatting rules |
| [`specs/module-structure.yml`](specs/module-structure.yml) | Expected folder/file layout for a module or library |
| `instructions/` | Prose rule sets, auto-applied via `applyTo` glob patterns |
| `prompts/` | On-demand agent workflows (`mode: agent`) |
| `optional-modules/gui-theming/` | Optional add-on for Python/Qt themed GUI projects |

## Core Instructions

| File | Applies To | Reused From |
|------|------------|-------------|
| [`instructions/environment-setup.instructions.md`](instructions/environment-setup.instructions.md) | `PROJECT_PARAMETERS.*` | New |
| [`instructions/planning.instructions.md`](instructions/planning.instructions.md) | `**/PLAN.md` | `planning.instructions.md` |
| [`instructions/code-review.instructions.md`](instructions/code-review.instructions.md) | source files (per `{{LANGUAGE}}`) | `code-review.instructions.md` |
| [`instructions/documentation.instructions.md`](instructions/documentation.instructions.md) | source files (per `{{LANGUAGE}}`) | `documentation.instructions.md` |
| [`instructions/markdown-style.instructions.md`](instructions/markdown-style.instructions.md) | `**/*.md` | `markdown-style.instructions.md` |
| [`instructions/coding-style.instructions.md`](instructions/coding-style.instructions.md) | source files (per `{{LANGUAGE}}`) | `cpp-style.instructions.md` |

## Core Prompts

| File | Purpose | Reused From |
|------|---------|-------------|
| [`prompts/setup-environment.prompt.md`](prompts/setup-environment.prompt.md) | Interview the Designer and instantiate the framework | New |
| [`prompts/new-module.prompt.md`](prompts/new-module.prompt.md) | Generate a new module/library `PLAN.md` | `new-library.prompt.md` |
| [`prompts/update-plan.prompt.md`](prompts/update-plan.prompt.md) | Update an existing `PLAN.md` | `update-plan.prompt.md` |
| [`prompts/new-component.prompt.md`](prompts/new-component.prompt.md) | Scaffold a new class/component | `new-class.prompt.md` |
| [`prompts/review-code.prompt.md`](prompts/review-code.prompt.md) | Structured code review → `REVIEW.md` | `review-code.prompt.md` |
| [`prompts/generate-docs.prompt.md`](prompts/generate-docs.prompt.md) | Generate/complete doc-comment blocks | `generate-docs.prompt.md` |

## Optional Modules

| Module | Include When | Contents |
|--------|--------------|----------|
| [`optional-modules/gui-theming/`](optional-modules/gui-theming/README.md) | `{{HAS_GUI_MODULE}} = true` | Themed PySide6/Qt GUI style rules, theme spec, widget/theme prompts |

Optional modules are self-contained (their own `instructions/`, `specs/`, `prompts/`) so they can
be copied in or left out without touching the core framework.

## Placeholder Convention

- Tokens use double curly braces: `{{TOKEN_NAME}}`.
- Every token is documented in [`specs/parameters.yml`](specs/parameters.yml) with an id,
  description, example, `required` flag, `default` (if optional) and `depends_on` (if
  conditional).
- Unresolved tokens must never be left in a file that is instantiated into a real `.github/`
  folder — the `setup-environment` prompt must flag any placeholder it cannot resolve instead of
  guessing a value.
