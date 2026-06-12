---
mode: agent
description: "Update or extend an existing PLAN.md: mark items done, add new phases, bump version."
---

# Update Existing PLAN.md

Update the `PLAN.md` of an existing library.

Follow `.github/instructions/planning.instructions.md` exactly.
Validate all updates against `.github/specs/plan-schema.yml`.
Never delete or rewrite completed phases — only append or annotate.
Only update the target library's `PLAN.md` file.

---

## Input Required

1. **Library name** — which library's PLAN.md are we updating?
2. **Action** — choose one or more:
   - `complete-items` — mark one or more checklist items as `[x]`.
   - `add-notes` — add a `**Notes:**` block to an existing phase.
   - `add-phase` — append a new phase at the end.
   - `update-status` — update the header Status field.
3. **Details** — for each action, provide the specific content to add or change.

---

## Rules

- **Bump version:** minor for a new phase, patch for notes/fixes.
- **Status derivation (required):**
  - If all planned phases are complete, set `**Status:** Phase N – Complete ✅`.
  - If at least one phase has open work, set `**Status:** Phase N – In progress 🔧`.
  - If no phase has started, set `**Status:** Phase 1 – Planned ⏳`.
  - `N` is the current phase number reflected by the active/last phase.
- **Checklist items:** use `[x]` for done, `[ ]` for not yet done.
- **Notes block:** add after the checklist; each bullet is one decision or finding.
- **New phase:** append after all existing phases; follow the phase block format from
  `.github/instructions/planning.instructions.md`.
- **Never renumber** existing phases.

---

## Action-Specific Consistency Rules

- **complete-items:**
  - Mark only the requested checklist items as `[x]`.
  - If all items in a phase become checked, set that phase marker to `✅`.
  - If a phase has at least one unchecked item, its marker must be `🔧` or `⏳`.

- **add-notes:**
  - Add or extend `**Notes:**` in the target phase without removing previous notes.
  - Keep one bullet per decision/finding.

- **add-phase:**
  - Append exactly one new phase at the end with the next sequential number.
  - Include at least one checklist item.
  - Include at least one smoke/integration test checklist item.
  - Do not modify numbering or content of previous phases.

- **update-status:**
  - Recompute status from actual phase state using the Status derivation rules above.

---

## Validation Checklist (must pass before final output)

- Version was bumped for every edit.
- Header format is valid (`Version`, `Status`, `Language`, `Namespace`).
- Phase numbers are sequential with no gaps and no renumbering.
- Phase/checklist consistency is valid:
  - `✅` phase has only `[x]` items.
  - `🔧` phase has at least one `[ ]` item.
- Each phase contains at least one test-oriented checklist item (smoke/integration/PASS).
- Changes are limited to the target library `PLAN.md`.

---

## Execution Policy

- If required input is missing or ambiguous, stop and request the missing fields explicitly.
- If input is complete, apply deterministic edits directly to the target `PLAN.md`.
- Preserve existing formatting and history; append or annotate only.
