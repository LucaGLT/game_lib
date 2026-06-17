"""Entry point for the Tic-Tac-Toe PySide6 GUI (gmGui hybrid).

Run from the ``GUI`` folder::

    python main.py

The window starts an event server on port 9100 and connects commands to the
C++ CoreEngine on port 9001.  The interactive 3x3 board is hosted as the central
widget, while the generic gmGui dashboards (Flow / Actor / Dice) are docked
around it.  Press *Nuova partita* to start a new match.
"""
from __future__ import annotations

import sys
from pathlib import Path

# ── sys.path bootstrap ────────────────────────────────────────────────────────
# 1. This GUI folder, so the local ``app`` / ``widgets`` / ``modules`` packages
#    import regardless of the working directory (the parent folders contain
#    characters that are not valid Python package names).
# 2. The ``pyLib`` folder, so the generic ``gmGui`` package (and its modules,
#    which use intra-package relative imports) can be imported.
_GUI_DIR = Path(__file__).resolve().parent
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"
for _path in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from PySide6.QtWidgets import QApplication  # noqa: E402

from app.tris_main_window import TrisMainWindow  # noqa: E402


def main() -> int:
    """Creates the Qt application and shows the main window."""
    app = QApplication(sys.argv)
    window = TrisMainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
