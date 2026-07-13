---
mode: agent
description: "Define a new visual theme compliant with the gmGui theme system."
---

# New GUI Theme

Define a new visual theme for the `gmGui` library.

Validate the new theme against `.github/specs/gui-theme.yml`.
Apply all rules in `.github/instructions/python-gui-style.instructions.md`.

---

## Input Required

1. **Theme id** — lowercase `snake_case` identifier (e.g. `forest`, `ocean_depths`).
2. **Display name** — human-readable title (e.g. `"Forest"`, `"Ocean Depths"`).
3. **Concept** — one sentence describing the visual inspiration.
4. **Keywords** — 4–6 adjectives describing the mood (e.g. `[calm, natural, deep, cool]`).
5. **Color palette** — hex values for the 5 mandatory tokens:
   - `background` — base window / application background.
   - `panel` — widget group / panel background (lighter or darker than background).
   - `border` — widget and panel border color.
   - `accent` — primary action color (buttons, active highlights).
   - `text` — primary text color.
6. **Corner radius** — choose from the allowed scale: `0, 2, 4, 8, 12, 16` px.
7. **Mood** — 2–5 evocative phrases (e.g. `["Deep forest", "Ancient oak"]`).

---

## Output Requirements

### YAML Entry

A complete theme entry in the format defined by `.github/specs/gui-theme.yml`
under the `themes:` list. Include all mandatory fields:
`id`, `display_name`, `concept`, `keywords`, `colors`, `corner_radius_px`, `mood`.

### Complete QSS Block

A QSS string covering **all widget types** listed in `gui-theme.yml` under `widgets:`:

- Dialog, Panel, Label
- PushButton — all three variants (`primary`, `secondary`, `danger`)
- ToggleButton (ON / OFF states)
- CheckBox, RadioButton
- ComboBox, TextBox (QLineEdit), SpinBox
- ListView (QListView), TreeView (QTreeView), TableView (QTableView)
- TabWidget, ScrollBar, ProgressBar, ToolTip

For each widget, define all required states from the widget specification
(`normal`, `:hover`, `:pressed`, `:focus`, `:disabled`, `:selected`, etc.).

Constraints on the QSS block:

- Use **only** the 5 palette colors provided; no additional hex constants.
- `border-radius` must equal `corner_radius_px` everywhere; no other radius values.
- Border thickness: `1px`, `2px`, or `3px` only.
- Padding and margin values: from `[4, 8, 16, 24, 32]` px only.
- Derived tones (e.g. hover lightening) must use QSS `rgba()` alpha overlays,
  not new hex constants.

### Contrast Check

Report the contrast ratio between `text` and `background`.
The ratio must be ≥ 4.5:1 for body text (WCAG AA).
If the ratio is below 4.5:1, propose an adjusted `text` or `background` value
and explain the change.

---

## Constraints

- No colors beyond the 5-color palette.
- The theme must be self-contained: no reference to other themes' color values.
- Domain or game-logic information must not appear in color variable names or comments.
- The new theme must produce correct rendering for all widgets without any
  modification to widget Python code.
