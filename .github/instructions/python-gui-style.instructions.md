---
applyTo: "**/*.py"
---

# Python GUI Style Instructions — game_lib / gmGui

> These instructions apply automatically to every Python source file.
> The authoritative source specification is `.github/GUI style.md`.
> The machine-readable rule set is `.github/specs/gui-theme.yml`.

---

## Fundamental Principle

Appearance must be **completely separated** from functionality.

In every Python/PySide6 widget class:

- No hardcoded color values (`"#A8873A"`, `QColor(168, 135, 58)`, etc.)
- No hardcoded font names or sizes (`setFont(QFont("Arial", 12))`)
- No hardcoded border style strings (`"border: 1px solid black"`)
- No inline `setStyleSheet(...)` calls that contain literal visual values

All visual decisions must derive from the **active theme** via QSS.

---

## QSS Rules

| Rule  | Requirement |
|-------|-------------|
| GUI-1 | Styling applied exclusively through QSS strings or `.qss` files. |
| GUI-2 | No literal hex colors or `QColor(r, g, b)` in widget Python code. |
| GUI-3 | No `setFont(QFont("name", size))` or equivalent in widget code. |
| GUI-4 | No hardcoded border style strings in Python widget code. |
| GUI-5 | No visual logic mixed into business or event-handling methods. |
| GUI-6 | Every visual value is traceable to the active theme token. |
| GUI-7 | No theme-specific branching in widget code (`if theme == ...`). |
| GUI-8 | Theme switching requires no modification to widget code. |
| GUI-9 | Domain or game-logic information must never drive a visual value. |
| GUI-10 | Every widget renders correctly under all 5 defined themes. |

---

## Spacing System

Use only values from the allowed set:

```
4 px   8 px   16 px   24 px   32 px
```

- `setContentsMargins`, `setSpacing`, `setFixedWidth/Height` must use only these values
  or their exact multiples (Rule SP-1).
- Arbitrary pixel values such as `3`, `5`, `6`, `10`, `15`, `20`, `23` are forbidden.

---

## Corner Radius Scale

Allowed values in QSS `border-radius`:

```
0 px   2 px   4 px   8 px   12 px   16 px
```

Use the per-theme default when no explicit override is needed (Rule CR-1).

---

## Border Thickness

Maximum `3 px`. Allowed: `1px`, `2px`, `3px` only (Rule BD-1).

---

## Typography

| Role        | Usage                                         | Requirement                         |
|-------------|-----------------------------------------------|-------------------------------------|
| `title`     | Dialog titles, panel headings, section labels | Largest, bold, high contrast        |
| `subtitle`  | Section headers, group labels, categories     | Medium size, medium emphasis        |
| `body`      | Labels, descriptions, content text            | Readable size, normal weight        |
| `secondary` | Hints, timestamps, metadata                   | Smallest, lower contrast            |

Typography must be controlled exclusively via QSS (Rule TY-1).
Never call `label.setFont(...)` with hardcoded values in widget code.

---

## Widget Visual States

All widgets that carry a visual state must implement every applicable state from:

```
normal   hover   focused   selected   active   disabled   warning   error   success
```

States are expressed through QSS pseudo-classes (`:hover`, `:focus`, `:disabled`, etc.)
or via `setProperty` + `style().polish()`.
Never implement state changes via Python color assignments (Rule WS-1).

---

## Widget Code Structure

```python
class MyWidget(QWidget):
    """Single-sentence description — no visual concerns here."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._build_layout()   # structural only; theme applied externally

    def _build_layout(self):
        # Spacing only from the allowed scale: 4, 8, 16, 24, 32 px
        layout = QVBoxLayout()
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)
        self.setLayout(layout)
```

- `setObjectName()` values must use `snake_case` and describe structural role only.
  Forbidden: names tied to color, theme, or appearance (`"red_button"`, `"dark_panel"`).
- No inline method bodies that perform visual configuration via Python properties.

---

## Theme Switching

- A `ThemeManager` (or equivalent) applies a global QSS sheet; individual widgets
  hold no theme state (Rule GUI-8).
- Widgets must render correctly under all 5 defined themes without any code change:
  `scroll`, `stone`, `dark_moon`, `blood`, `techno`.

---

## Domain Isolation

Domain or game-logic values must never drive a visual output directly (Rule GUI-9).

```python
# Forbidden
if actor.faction == "evil":
    button.setStyleSheet("background-color: #8B0000;")

# Correct — state exposed via Qt dynamic property; QSS reacts
button.setProperty("variant", "danger")
button.style().unpolish(button)
button.style().polish(button)
```

---

## Shadows

Shadows are **decorative only**, never structural (Rule SH-1).
`QGraphicsDropShadowEffect` blur radius must not exceed `8 px`.
Do not add shadow effects unless explicitly required by the active theme specification.
