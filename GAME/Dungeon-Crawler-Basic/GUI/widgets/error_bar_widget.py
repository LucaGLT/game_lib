"""Dungeon Crawler Basic — error/feedback bar widget.

ErrorBarWidget displays short-lived validation and feedback messages at the
bottom of the main window (embedded in the status bar). It shows engine
rejection reasons (``dungeon.action.rejected``) and other non-fatal
notifications in a single line.

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class ErrorBarWidget(QWidget):
    """Single-line feedback bar for action rejections and engine notifications.

    Messages are shown for a fixed duration and then cleared automatically.
    The bar is visually distinct when showing an error (styled via QSS
    property changes) and neutral when empty.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the bar in its empty, neutral state."""
        super().__init__(parent)
        from PySide6.QtWidgets import QHBoxLayout, QLabel
        from PySide6.QtCore import QTimer
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 0, 4, 0)
        self._label = QLabel()
        self._label.setWordWrap(False)
        layout.addWidget(self._label)
        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self.clear)

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and shows feedback if relevant."""
        if msg.get("typeId") == "dungeon.action.rejected":
            data = msg.get("data", {})
            self.show_error(data.get("reason", "Action rejected"))

    def show_error(self, message: str) -> None:
        """Displays an error message for 4 seconds."""
        self._label.setText(f"⚠ {message}")
        self._timer.start(4000)

    def show_info(self, message: str) -> None:
        """Displays a neutral informational message for 3 seconds."""
        self._label.setText(message)
        self._timer.start(3000)

    def clear(self) -> None:
        """Hides any displayed message."""
        self._label.clear()
