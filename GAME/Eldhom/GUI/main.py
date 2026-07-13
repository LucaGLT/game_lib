"""Entry point for the Le Pergamene di Eldhom PySide6 GUI.

Run from the ``GUI`` folder::

    python main.py

The GUI starts an event server on port 9210 and sends commands to the C++
CoreEngine on port 9211.  The engine must already be running (start it first
via run_eldhom.bat).
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

from app.eldhom_bridge import EldhomBridge           # noqa: E402
from app.eldhom_main_window import EldhomMainWindow  # noqa: E402

try:
    from gmGui.theme_manager import ThemeManager
    _HAS_THEME = True
except ImportError:
    _HAS_THEME = False


def main() -> int:
    """Creates the Qt application and shows the Eldhom main window."""
    app = QApplication(sys.argv)
    if _HAS_THEME:
        ThemeManager(app).apply_theme("dark_moon")

    # ── Avvia il receiver PRIMA di creare la finestra ─────────────────────────
    # L'engine (C++) parte per PRIMO (run_eldhom.bat). La sua IpSocketChannel
    # tenta di connettersi al receiver GUI (porta 9210) alla prima send().
    # Avviando il receiver qui, la porta 9210 è in ascolto entro ~100 ms
    # dall'avvio del processo Python — ben prima che l'utente selezioni una
    # missione e l'engine tenti la connessione.
    bridge = EldhomBridge()
    bridge.receiver.start()
    print("[EldhomGUI] Event receiver avviato su porta 9210", flush=True)

    window = EldhomMainWindow(bridge=bridge)
    print("[EldhomGUI] Finestra pronta. Usa Gioca > Inizia Nuova Missione.", flush=True)
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())

