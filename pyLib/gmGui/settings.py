"""QSettings wrapper for layout and module-state persistence."""
from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .main_window import MainWindow


def save_layout(window: MainWindow) -> None:
    """Saves window geometry, dock layout, and per-module state to QSettings.

    Phase 1 stub: no-op.
    Full implementation: Phase 9.
    """
    # TODO: Phase 9 — QSettings("GameLib", "gmGui"), saveGeometry(), saveState(),
    #                  and mod.save_state() for each module.
    pass


def restore_layout(window: MainWindow) -> None:
    """Restores window geometry, dock layout, and per-module state from QSettings.

    Phase 1 stub: no-op.
    Full implementation: Phase 9.
    """
    # TODO: Phase 9 — restoreGeometry(), restoreState(),
    #                  and mod.restore_state() for each module.
    pass
