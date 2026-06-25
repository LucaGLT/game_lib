"""QSettings wrapper for layout and module-state persistence.

Saves and restores:
- Window geometry  (``saveGeometry`` / ``restoreGeometry``)
- Dock widget layout (``saveState``   / ``restoreState``)

Per-module state (Phase 9) will be added when modules implement
``save_state()`` / ``restore_state()``.
"""
from __future__ import annotations

import json
from typing import TYPE_CHECKING

from PySide6.QtCore import QSettings

if TYPE_CHECKING:
    from .main_window import MainWindow

# Application-level QSettings identifiers.
_ORG: str = "GameLib"
_APP: str = "gmGui"


def _get_settings() -> QSettings:
    """Returns a ``QSettings`` instance scoped to GameLib/gmGui (INI format)."""
    return QSettings(_ORG, _APP)


def save_layout(window: MainWindow) -> None:
    """Saves window geometry, dock layout, and per-module state to QSettings.

    Stored keys
    -----------
    - ``geometry``         — ``QByteArray`` from ``saveGeometry()``
    - ``windowState``      — ``QByteArray`` from ``saveState()``
    - ``module/<id>/state`` — JSON string from each module's ``save_state()``
    """
    cfg = _get_settings()
    cfg.setValue("geometry", window.saveGeometry())
    cfg.setValue("windowState", window.saveState())

    for mod in window._modules:
        state = mod.save_state()
        if state:
            cfg.setValue(f"module/{mod.module_id}/state", json.dumps(state))

    cfg.sync()


def restore_layout(window: MainWindow) -> None:
    """Restores window geometry, dock layout, and per-module state from QSettings.

    Missing keys are silently skipped (first launch, or settings cleared).
    """
    cfg = _get_settings()

    geometry = cfg.value("geometry")
    if geometry is not None:
        window.restoreGeometry(geometry)

    state = cfg.value("windowState")
    if state is not None:
        window.restoreState(state)

    for mod in window._modules:
        raw = cfg.value(f"module/{mod.module_id}/state")
        if raw is not None:
            try:
                mod.restore_state(json.loads(raw))
            except (json.JSONDecodeError, TypeError, ValueError):
                pass  # corrupt entry — start fresh for this module
