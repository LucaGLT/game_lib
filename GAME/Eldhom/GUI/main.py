"""Entry point for the Le Pergamene di Eldhom PySide6 GUI.

Run from the ``GUI`` folder::

    python main.py

The GUI starts an event server on port 9210 and sends commands to the C++
CoreEngine on port 9211.  A mission-selection dialog appears on startup.
"""
from __future__ import annotations

import sys
from pathlib import Path

# ── sys.path bootstrap ────────────────────────────────────────────────────────
_GUI_DIR   = Path(__file__).resolve().parent
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"

for _path in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from PySide6.QtWidgets import QApplication  # noqa: E402

from app.eldhom_main_window import EldhomMainWindow  # noqa: E402


def main() -> int:
    """Creates the Qt application and shows the Eldhom main window."""
    app = QApplication(sys.argv)
    window = EldhomMainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
