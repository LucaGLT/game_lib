# Optional Module — GUI Theming

> Include this module only when `{{HAS_GUI_MODULE}}` is `true`.

## Purpose

Provides a complete, reusable framework for building themed GUI applications with strict
separation between visual styling and business/domain logic. Originally proven on PySide6/Qt
projects, but the principles (no hardcoded visual values, theme-driven styling, state-based
QSS/CSS) apply to any styleable-widget toolkit.

## Contents

| Path | Purpose |
|------|---------|
| [`instructions/gui-style.instructions.md`](instructions/gui-style.instructions.md) | Auto-applied rules for GUI source files (`applyTo` on `**/*.py` or the project's GUI file extension) |
| [`specs/gui-theme.yml`](specs/gui-theme.yml) | Machine-readable theme/token contract |
| [`prompts/new-gui-widget.prompt.md`](prompts/new-gui-widget.prompt.md) | Scaffold a new themed widget |
| [`prompts/new-gui-theme.prompt.md`](prompts/new-gui-theme.prompt.md) | Define a new visual theme |
| [`prompts/apply-gui-theme.prompt.md`](prompts/apply-gui-theme.prompt.md) | Audit/refactor an existing widget file for theme compliance |

## Integration Steps (performed by `setup-environment.prompt.md`)

1. Copy `instructions/gui-style.instructions.md` → `.github/instructions/gui-style.instructions.md`.
2. Copy `specs/gui-theme.yml` → `.github/specs/gui-theme.yml`.
3. Copy all three prompt files → `.github/prompts/`.
4. Append the commented-out GUI section from `copilot-instructions.template.md` into the
   generated root `.github/copilot-instructions.md`, with tokens resolved.

## Parameters Used

- `{{GUI_TOOLKIT}}` — target toolkit (default `PySide6`; also works with `PyQt6`/`PyQt5`).
- `{{THEME_IDS}}` — comma-separated list of theme identifiers the GUI must support (default:
  the 5-theme reference set `scroll, stone, dark_moon, blood, techno` — replace with the
  project's own theme names if different).
