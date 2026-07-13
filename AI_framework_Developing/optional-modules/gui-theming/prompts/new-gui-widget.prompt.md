---
mode: agent
description: "Scaffold a new themed widget for the GUI module."
---

# New GUI Widget

Scaffold a new themed widget for the `{{PROJECT_NAME}}` GUI module (`{{GUI_TOOLKIT}}`).

Apply all rules in `.github/instructions/gui-style.instructions.md`.
Validate theme token usage against `.github/specs/gui-theme.yml`.

---

## Input Required

1. **Widget name** — PascalCase class name (e.g. `ActorStatusPanel`).
2. **Purpose** — one sentence describing what this widget displays or allows.
3. **Parent class** — which base widget class to inherit from? (e.g. `QWidget`, `QFrame`, `QGroupBox`)
4. **Content** — list the child widgets it contains
   (e.g. `QLabel` for name, `QProgressBar` for a value).
5. **Visual states** — which states from
   `[normal, hover, focused, selected, active, disabled, warning, error, success]`
   must this widget express visually?
6. **Button variants** — if the widget contains push buttons, specify `primary`,
   `secondary`, or `danger` for each one.
7. **Signals emitted** — list any signals with their signature
   (e.g. `item_selected = Signal(str)`).
8. **Event routing** — which event/message types does this widget respond to via its event
   handler? Leave blank if none.

---

## Output Requirements

### `WidgetName.py` (or the toolkit's equivalent source file)

- Inherits from the specified parent class.
- `__init__(self, parent=None)` — structural setup only; no hardcoded visual values.
- `_build_layout()` — creates and arranges child widgets.
  All spacing/size calls must use spacing-scale values only: `4, 8, 16, 24, 32` px.
- An event-handling method — present only if the widget responds to external events;
  routes by event/message type, no visual logic in the routing body.
- Signals declared as class-level attributes, before `__init__`.
- No literal color strings, font names, border values, or radius values anywhere.
- Object names set via `setObjectName()` must use `snake_case` and describe structural
  role, not visual role.

### QSS Block for Each Theme

Provide a QSS block (as a string constant named `QSS_<THEME_ID>`) for **each** theme id listed
in `{{THEME_IDS}}`.

Each block must:

- Define styles for all required visual states listed in input item 5.
- Use only the color values from `.github/specs/gui-theme.yml` for that theme.
- Apply `border-radius` equal to the theme's `corner_radius_px`.
- Respect border thickness: `1px`, `2px`, or `3px` only.

### Test Stub

A test class (`TestWidgetName`) with:

- `test_instantiation` — widget creates without exception; type check.
- One test per emitted signal — verifies the signal fires on the expected trigger.
- One test per routed event/message type, if applicable.

---

## Constraints

- No hardcoded visual values anywhere — all 10 rules GUI-1 through GUI-10 must hold.
- Spacing only from the allowed scale: `4, 8, 16, 24, 32` px.
- Corner radius only from the allowed scale: `0, 2, 4, 8, 12, 16` px (or theme default).
- Widget must render correctly under all themes in `{{THEME_IDS}}` without any code
  modification.
- No cross-module imports beyond the GUI module and the `{{GUI_TOOLKIT}}` toolkit itself.
