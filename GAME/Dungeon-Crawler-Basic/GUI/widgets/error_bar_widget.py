"""Dungeon Crawler Basic — error/feedback bar widget.

ErrorBarWidget displays validation and feedback messages in the Messaggi dock.
Messages stay visible until replaced by the next one.

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class ErrorBarWidget(QWidget):
    """Single-line feedback bar for action rejections and engine notifications.

    Messages remain visible until the next message arrives or clear() is called
    explicitly.  There is no auto-dismiss timer.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the bar in its empty, neutral state."""
        super().__init__(parent)
        from PySide6.QtWidgets import QHBoxLayout, QLabel
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 0, 4, 0)
        self._label = QLabel()
        self._label.setWordWrap(False)
        layout.addWidget(self._label)

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and shows feedback if relevant."""
        if msg.get("typeId") == "dungeon.action.rejected":
            data = msg.get("data", {})
            self.show_error(data.get("reason", "Action rejected"))

    def show_error(self, message: str) -> None:
        """Displays an error message; stays until the next message or clear()."""
        self._label.setText(f"⚠ {message}")

    def show_info(self, message: str) -> None:
        """Displays a neutral informational message; stays until the next message or clear()."""
        self._label.setText(message)

    def clear(self) -> None:
        """Removes any displayed message."""
        self._label.clear()
