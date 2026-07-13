"""Entry point for the Dungeon Crawler Basic PySide6 GUI.

Run from the ``GUI`` folder::

    python main.py

The GUI starts an event server on port 9200 and sends commands to the C++
CoreEngine on port 9201.  Send ``dungeon.new_game`` from the GUI toolbar to
start a session.
"""
from __future__ import annotations

import sys
from pathlib import Path

# ── sys.path bootstrap ────────────────────────────────────────────────────────
# 1. This GUI folder — so local ``app`` / ``widgets`` packages import correctly
#    regardless of the working directory.
# 2. The workspace ``pyLib`` folder — so the shared ``gmGui`` engine_bridge
#    framing layer can be imported.
_GUI_DIR  = Path(__file__).resolve().parent
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"

for _path in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from PySide6.QtWidgets import QApplication  # noqa: E402

from app.dungeon_main_window import DungeonMainWindow  # noqa: E402


def main() -> int:
    """Creates the Qt application and shows the Dungeon Crawler main window."""
    app = QApplication(sys.argv)
    window = DungeonMainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
