---
applyTo: "PROJECT_PARAMETERS.{md,yml,yaml}"
---

# Environment Setup Instructions — {{PROJECT_NAME}}

> Apply these instructions whenever `PROJECT_PARAMETERS.md` (or `.yml`) is created or updated,
> and whenever the `setup-environment` prompt is invoked.
> Reference parameter registry: `.github/specs/parameters.yml`
> (or `AI_framework_Developing/specs/parameters.yml` before instantiation).

---

## Purpose

Before any template in this framework can be instantiated into a real `.github/` folder, every
placeholder token must be resolved to a correct, Designer-confirmed value. This file defines the
**policy** for how that interview must be conducted. The actual interactive workflow lives in
`prompts/setup-environment.prompt.md`.

"Designer" means the person setting up or extending the project environment (could be a tech
lead, architect, or the developer bootstrapping a new module).

---

## Core Rules

1. **Never invent a value for a `required: true` parameter.** Always ask the Designer.
2. **Optional parameters may be pre-filled with their `default`**, but must still be shown to the
   Designer for explicit confirmation or override — do not apply defaults silently.
3. **Respect `depends_on` conditions.** Skip a parameter's question entirely if its dependency
   condition is false (e.g. do not ask GUI-related questions when `HAS_GUI_MODULE` is `false`).
4. **Ask grouped, not scattered.** Follow the group order defined in `parameters.yml`:
   `identity` → `naming` → `language_style` → `documentation` → `structure` → `versioning` →
   `gui_optional`.
5. **Always summarize before writing.** Present a full table of resolved values and get an
   explicit go-ahead before generating or overwriting any file.
6. **Incremental re-runs.** If `PROJECT_PARAMETERS.md` already exists, read it first and only
   ask about missing or explicitly-changed values — do not re-interview from scratch.
7. **Traceability.** Persist every resolved value (including defaults that were accepted as-is)
   into `PROJECT_PARAMETERS.md` at the project root.

---

## Question Blueprint (by group)

### Identity

- What is the project name? (`PROJECT_NAME`)
- Who is the client/customer for this project? Use "Internal" if not applicable. (`CLIENT_NAME`)
- What is the name of the library/module being set up? (`MODULE_NAME`)
- What short acronym/prefix identifies it (used in namespace/facade naming)? (`MODULE_PREFIX`)
- Who is the responsible team or author? (`TEAM_OR_AUTHOR`, optional)

### Naming

- Confirm or override the derived namespace pattern and example. (`NAMESPACE_PATTERN`,
  `NAMESPACE_EXAMPLE`)
- Does this module use a dedicated facade-class prefix? If so, which? (`FACADE_PREFIX`, optional)
- Confirm interface/exception/private-member/factory/boolean-query/constant naming conventions,
  offering the registry defaults as a starting point (all optional, but must be shown).

### Language & Style

- What is the primary language? (`LANGUAGE`, required)
- What language standard/version must be enforced? (`LANGUAGE_STANDARD`, required)
- Confirm indentation style/width, brace style, and line-length limit (optional, show defaults).
- If the language is C/C++-family, confirm include-guard and preprocessor rules apply
  (`USE_INCLUDE_GUARDS`, `HAS_PREPROCESSOR`).

### Documentation

- Which doc-comment convention should be used? (`DOC_COMMENT_STYLE`, default `Doxygen`)
- Confirm the API manual filename suffix. (`API_DOC_SUFFIX`, default `_API.md`)

### Structure

- Confirm the module root path. (`MODULE_ROOT_PATH`, default `{{MODULE_NAME}}/`)
- Are external/third-party dependencies allowed? (`ALLOW_EXTERNAL_DEPENDENCIES`, default `false`)
- What is the cross-module dependency policy? (`CROSS_MODULE_POLICY`, default
  `bridges/ adapters only`)

### Versioning

- Confirm the versioning scheme. (`VERSIONING_SCHEME`, default `semver`)

### Optional GUI Module

- Does this project include a themed GUI? (`HAS_GUI_MODULE`, default `false`)
- If yes: which GUI toolkit? (`GUI_TOOLKIT`, default `PySide6`) and which theme identifiers must
  be supported? (`THEME_IDS`, default the 5-theme reference set)

---

## Validation Rules

- `MODULE_PREFIX` must match `^[a-zA-Z][a-zA-Z0-9]{0,9}$` (short, alphanumeric, starts with a
  letter).
- `PROJECT_NAME`, `CLIENT_NAME`, `MODULE_NAME` must be non-empty.
- `LANGUAGE_STANDARD` must be consistent with `LANGUAGE` (e.g. do not accept "C++17 Standard"
  when `LANGUAGE` is "Python").
- If `HAS_GUI_MODULE` is `true`, `GUI_TOOLKIT` and `THEME_IDS` become required for that section.
- Reject and re-ask on any answer that fails validation instead of silently coercing it.

---

## Completion Criteria

The interview is complete only when:

- Every `required: true` parameter has a confirmed value.
- Every shown default has been explicitly accepted or overridden.
- The Designer has approved the final summary table.
- `PROJECT_PARAMETERS.md` has been written or updated at the project root.

Only after these conditions hold may the agent proceed to instantiate templates into
`.github/` (see `prompts/setup-environment.prompt.md` for that step).
