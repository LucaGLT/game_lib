---
mode: agent
description: "Analyse existing code for style/naming violations and produce a structured REVIEW.md correction plan."
---

# Review Existing Code

Analyse one or more existing `{{LANGUAGE}}` files (or an entire module) for compliance with
`{{PROJECT_NAME}}` style and coding rules. Produce a structured `REVIEW.md` correction plan.

Apply all rules from `.github/instructions/code-review.instructions.md`.
Use `.github/specs/review-plan-schema.yml` for the exact output format.
Use `.github/specs/coding-style.yml` as the authoritative rule reference.

---

## Input Required

1. **Scope** — choose one:
   - `file` — review one or more specific files (provide paths).
   - `module` — review all source files inside a given module folder.
   - `workspace` — review all `{{LANGUAGE}}` files in the workspace.
2. **Paths** — file paths or module name (if scope is `file` or `module`).
3. **Categories** — which rule categories to check (default: all).
   Categories: `naming`, `guards`, `formatting`, `spacing`, `control-flow`,
   `signatures`, `includes`, `preprocessor`, `documentation`, `constraints`.
4. **Output location** — where to write `REVIEW.md` (default: inside the reviewed
   module folder, or workspace root for a workspace-wide review).

---

## Review Process

Follow the step-by-step process in `.github/instructions/code-review.instructions.md`:

1. Read each file in the requested scope completely.
2. Apply the rule categories checklist (CAT-1 through CAT-10) to each file.
3. Record every violation: rule ID, file, symbol or line range, description, correction.
4. Group findings by category. Assign severity (🔴 Error / 🟡 Warning / 🔵 Info).
5. Write `REVIEW.md` following the schema.

**Important:**

- Do NOT apply fixes silently. Produce the plan first.
- Do NOT report false positives. Only flag real, unambiguous violations.
- Scan the entire scope before writing any output.

---

## Output Requirements

Generate a single `REVIEW.md` file at the requested output location.

The file must contain (in order):

1. **Header block** — module or scope reviewed, date, reviewer (AI), rule set version.
2. **Summary table** — one row per category: category name, findings count, severity breakdown.
3. **Findings** — grouped by category (CAT-1 … CAT-10); each finding has:
   - Rule ID and severity badge.
   - File path and symbol or line range.
   - Description of the violation.
   - Concrete correction (show the fixed code snippet).
4. **Correction Plan** — ordered list of all files that need changes, grouped by severity.
   Each file entry lists the rules violated and the estimated number of touch-points.
5. **Statistics** — total findings, breakdown by severity, % of files with violations.

---

## Constraints

- Do not modify any source file — output only `REVIEW.md`.
- If the scope has zero violations, still produce `REVIEW.md` with a "No violations found" summary.
- Keep code snippets in the findings short (≤ 10 lines). Show only the relevant part,
  not whole function bodies.
