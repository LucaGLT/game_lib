---
mode: agent
description: "Audit and refactor an existing GUI source file to comply with the theme system."
---

# Apply GUI Theme Compliance

Audit and optionally refactor an existing GUI source file to comply with the
`{{PROJECT_NAME}}` visual theme system.

Apply all rules in `.github/instructions/gui-style.instructions.md`.
Validate corrections against `.github/specs/gui-theme.yml`.

---

## Input Required

1. **File path** — which file to audit and/or refactor?
2. **Scope** — choose one:
   - `audit-only` — report all violations; do not modify the file.
   - `refactor` — fix all violations; produce the corrected file.
3. **Target themes** (optional) — comma-separated theme ids to validate against
   (subset of `{{THEME_IDS}}`). Leave blank to validate against all of them.

---

## Audit Checklist

For each violation found, report a table row:

| Field     | Description |
|-----------|--------------|
| Rule ID   | e.g. `GUI-2` |
| Location  | file path + line number |
| Violation | exact code snippet that violates the rule |
| Fix       | corrected code or the required approach |

Rules to check (from `gui-theme.yml` `mandatory_rules`):

- **GUI-1** — All styling goes through QSS; no direct palette/color-role assignments used for
  theming.
- **GUI-2** — No literal hex colors or raw color constructors in widget code.
- **GUI-3** — No hardcoded font names or sizes.
- **GUI-4** — No hardcoded border style strings in code.
- **GUI-5** — No visual logic in business or event-handling methods.
- **GUI-6** — Every visual value is traceable to an active theme token.
- **GUI-7** — No theme-specific branching in widget code (`if theme == ...`).
- **GUI-8** — No widget modification needed when switching themes.
- **GUI-9** — No domain or business-logic field drives a visual value directly.
- **GUI-10** — Widget structure supports all themes in `{{THEME_IDS}}`.

Also check:

- **SP-1** — Spacing values are from `[4, 8, 16, 24, 32]` px only.
- **CR-1** — `border-radius` values are from `[0, 2, 4, 8, 12, 16]` px only.
- **BD-1** — Border thickness is `1px`, `2px`, or `3px` only.
- **TY-1** — Typography is set only via QSS; no hardcoded font calls.
- **WS-1** — Widget states implemented via QSS pseudo-classes or dynamic properties.
- **SH-1** — Shadow blur radius does not exceed `8 px`.

---

## Output Requirements

### Audit Report (always)

A table of every violation with columns: Rule ID, Location, Violation, Fix.

A summary line at the end:

```text
N violation(s) found: M GUI-*, K SP-*, ...
```

### Refactored File (only when scope is `refactor`)

- The complete corrected source file.
- All hardcoded visual values replaced with QSS-based theme tokens.
- No logic, data flow, or structural changes — only visual separation fixes.
- A diff summary at the end: total lines changed, list of violations resolved.

---

## What NOT to Change

- Signal definitions, connections, or handler signatures.
- Business logic and event-handling code.
- Layout structure (which widgets exist and how they are nested).
- Public method signatures or return types.
- Any `# TODO:` markers.
- `setObjectName()` calls (unless the name encodes a visual value, which is a GUI-9 violation).
