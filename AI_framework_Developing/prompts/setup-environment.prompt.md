---
mode: agent
description: "Interview the Designer to collect project parameters, then instantiate the AI_framework_Developing templates into this project's .github folder."
---

# Setup Environment

Prepare a new (or extend an existing) project environment by resolving every parameter defined
in `specs/parameters.yml` and instantiating the reusable AI framework templates into the target
project's `.github/` folder.

Follow `instructions/environment-setup.instructions.md` exactly for interview policy and
validation rules.

---

## Step 0 — Detect Existing Setup

1. Look for `PROJECT_PARAMETERS.md` (or `.yml`) at the target project root.
2. If it exists, read it and treat every parameter already resolved there as pre-filled.
   Only ask about parameters that are missing, or that the user explicitly wants to change.
3. If it does not exist, this is a first-time setup — ask the full interview.

---

## Input Required (Interview)

Ask questions grouped exactly as in `specs/parameters.yml`, in this order. For the full
question wording, validation patterns and conditional (`depends_on`) rules, follow
`instructions/environment-setup.instructions.md`.

1. **Identity** — `PROJECT_NAME`, `CLIENT_NAME`, `MODULE_NAME`, `MODULE_PREFIX`,
   `TEAM_OR_AUTHOR`.
2. **Naming** — `NAMESPACE_PATTERN`, `NAMESPACE_EXAMPLE`, `FACADE_PREFIX`, `INTERFACE_PREFIX`,
   `EXCEPTION_PREFIX`, `EXCEPTION_BASE_SUFFIX`, `PRIVATE_MEMBER_CONVENTION`, `FACTORY_PREFIX`,
   `BOOLEAN_QUERY_PREFIXES`, `FUNCTION_NAMING_CONVENTION`, `CONSTANT_NAMING_CONVENTION`.
3. **Language & Style** — `LANGUAGE`, `LANGUAGE_STANDARD`, `LANGUAGE_FILE_EXTENSIONS`,
   `INDENT_STYLE`, `INDENT_WIDTH`, `BRACE_STYLE`, `LINE_LENGTH_LIMIT`, `USE_INCLUDE_GUARDS`,
   `HAS_PREPROCESSOR`.
4. **Documentation** — `DOC_COMMENT_STYLE`, `API_DOC_SUFFIX`.
5. **Structure** — `MODULE_ROOT_PATH`, `ALLOW_EXTERNAL_DEPENDENCIES`, `CROSS_MODULE_POLICY`.
6. **Versioning** — `VERSIONING_SCHEME`.
7. **Optional GUI Module** — `HAS_GUI_MODULE`, and only if `true`: `GUI_TOOLKIT`, `THEME_IDS`.

Rules while asking:

- Never invent a value for a `required: true` parameter — always ask.
- For optional parameters, propose the registry `default` and ask for confirmation/override in
  the same question rather than skipping it silently.
- Skip any parameter whose `depends_on` condition is false.
- Compute derived tokens (e.g. `MODULE_PREFIX_UPPER`) automatically — do not ask for them.

---

## Step 1 — Summarize and Confirm

Present a single summary table with every resolved parameter (id → value → source: *asked*,
*default accepted*, or *carried over*). Ask for explicit confirmation before writing or
generating anything. If the Designer requests changes, update the table and re-confirm.

---

## Step 2 — Persist Parameters

Write (or update) `PROJECT_PARAMETERS.md` at the target project root with the full resolved
parameter table, grouped by section, in human-readable Markdown form.

---

## Step 3 — Instantiate Core Framework

Copy the following into the target project, substituting every registered parameter token
(from `specs/parameters.yml`, derived tokens included) with its resolved value:

- `copilot-instructions.template.md` → `.github/copilot-instructions.md` (strip the leading
  "TEMPLATE NOTE" comment block — it must not appear in the generated file)
- `instructions/*.instructions.md` → `.github/instructions/*.instructions.md`
- `prompts/*.prompt.md` → `.github/prompts/*.prompt.md`, **including this file itself**, so the
  environment can be re-run or extended later (e.g. to add a module or change a parameter)
- `specs/*.yml` → `.github/specs/*.yml`, **except `parameters.yml`**, which is the template's
  own parameter registry and stays in `AI_framework_Developing/` — it is not part of the
  generated `.github/` output

When substituting tokens nested inside glob brace syntax (e.g. `**/*.{{{LANGUAGE_FILE_EXTENSIONS}}}`),
replace only the inner `{{TOKEN}}` and preserve the outer single brace on each side, so the
`applyTo` pattern remains valid glob syntax after substitution.

---

## Step 4 — Instantiate Optional Modules

If `HAS_GUI_MODULE` is `true`, also copy `optional-modules/gui-theming/` contents:

- `instructions/gui-style.instructions.md` → `.github/instructions/gui-style.instructions.md`
- `specs/gui-theme.yml` → `.github/specs/gui-theme.yml`
- `prompts/new-gui-widget.prompt.md`, `new-gui-theme.prompt.md`, `apply-gui-theme.prompt.md` →
  `.github/prompts/`

Then append the commented-out GUI section from `copilot-instructions.template.md` (the block
after `<!-- If {{HAS_GUI_MODULE}} == true ... -->`) into the generated
`.github/copilot-instructions.md`, uncommented and with tokens resolved.

If `HAS_GUI_MODULE` is `false`, skip this step entirely — do not create GUI-related files.

---

## Step 5 — Report

Produce a short report listing:

- Every file created or updated, with its target path.
- Any occurrence of a *registered* parameter token (per `specs/parameters.yml`) that could not
  be resolved (must be zero before declaring success). Incidental prose mentions of the
  double-curly-brace syntax itself (e.g. an example inside an explanatory sentence) are not
  violations as long as no registered token id is left unresolved.
- Whether the optional GUI module was included.

---

## Constraints

- Never overwrite an existing `.github` file without explicit confirmation from the Designer.
- Never leave an unresolved `{{TOKEN}}` in a generated `.github` file — if a value is missing,
  stop and ask rather than guessing.
- Do not modify any file outside the target project's root `PROJECT_PARAMETERS.md` and its
  `.github/` folder during instantiation.
- Preserve the source templates in `AI_framework_Developing/` unchanged — this is a copy-and-
  substitute operation, not a move.
