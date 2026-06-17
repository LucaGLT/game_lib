"""Entry point for the Tic-Tac-Toe PySide6 GUI.

Run from the ``GUI`` folder::

    python main.py

The window starts an event server on port 9000 and connects commands to the
C++ CoreEngine on port 9001. Press *Reload* to start a new match.
"""
from __future__ import annotations

import sys
from pathlib import Path

# Make the local ``app`` and ``widgets`` packages importable regardless of the
# current working directory (the parent folders contain characters that are not
# valid Python package names, so we put this folder itself on sys.path).
sys.path.insert(0, str(Path(__file__).resolve().parent))

from PySide6.QtWidgets import QApplication  # noqa: E402

from app.tris_window import TrisWindow  # noqa: E402


def main() -> int:
    """Creates the Qt application and shows the main window."""
    app = QApplication(sys.argv)
    window = TrisWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
