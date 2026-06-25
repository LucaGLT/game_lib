---
mode: agent
description: "Scaffold a new themed PySide6 widget for the gmGui library."
---

# New GUI Widget

Scaffold a new themed PySide6 widget for the `gmGui` library.

Apply all rules in `.github/instructions/python-gui-style.instructions.md`.
Validate theme token usage against `.github/specs/gui-theme.yml`.

---

## Input Required

1. **Widget name** — PascalCase Python class name (e.g. `ActorStatusPanel`).
2. **Purpose** — one sentence describing what this widget displays or allows.
3. **Parent class** — which Qt class to inherit from? (e.g. `QWidget`, `QFrame`, `QGroupBox`)
4. **Content** — list the child widgets it contains
   (e.g. `QLabel` for actor name, `QProgressBar` for HP).
5. **Visual states** — which states from
   `[normal, hover, focused, selected, active, disabled, warning, error, success]`
   must this widget express visually?
6. **Button variants** — if the widget contains push buttons, specify `primary`,
   `secondary`, or `danger` for each one.
7. **Signals emitted** — list any Qt signals with their Python signature
   (e.g. `actor_selected = Signal(str)`).
8. **Envelope routing** — which `typeId` strings from the engine does this widget
   respond to via `on_envelope()`? Leave blank if none.

---

## Output Requirements

### `WidgetName.py`

- Inherits from the specified parent class.
- `__init__(self, parent=None)` — structural setup only; no hardcoded visual values.
- `_build_layout()` — creates and arranges child widgets.
  All `setContentsMargins` / `setSpacing` / `setFixedSize` calls must use spacing-scale
  values only: `4, 8, 16, 24, 32` px.
- `on_envelope(envelope: dict)` — present only if the widget responds to engine events;
  routes by `typeId`, no visual logic in the routing body.
- Signals declared as class-level `Signal(...)` attributes, before `__init__`.
- No literal color strings, font names, border values, or radius values anywhere.
- Object names set via `setObjectName()` must use `snake_case` and describe structural
  role, not visual role.

### QSS Block for Each Theme

Provide a QSS block (as a Python string constant named `QSS_<THEME_ID>`) for **each**
of the 5 defined themes: `scroll`, `stone`, `dark_moon`, `blood`, `techno`.

Each block must:

- Define styles for all required visual states listed in input item 5.
- Use only the color values from `.github/specs/gui-theme.yml` for that theme.
- Apply `border-radius` equal to the theme's `corner_radius_px`.
- Respect border thickness: `1px`, `2px`, or `3px` only.

### Test Stub

A `pytest` test class (`TestWidgetName`) with:

- `test_instantiation` — widget creates without exception; `isinstance` check.
- One test per emitted signal — verifies the signal fires on the expected trigger.
- `test_on_envelope_<type_id>` — one test per routed `typeId` if applicable.

---

## Constraints

- PySide6 import style: `from PySide6.QtWidgets import ...`
- No hardcoded visual values anywhere — all 10 rules GUI-1 through GUI-10 must hold.
- Spacing only from the allowed scale: `4, 8, 16, 24, 32` px.
- Corner radius only from the allowed scale: `0, 2, 4, 8, 12, 16` px (or theme default).
- Widget must render correctly under all 5 defined themes without any code modification.
- No cross-library imports; widget code may import only from `gmGui` and PySide6.
