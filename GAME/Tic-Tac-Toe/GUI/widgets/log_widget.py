"""LogWidget — append-only match log (figure region *R_Panel*: "Log della partita")."""
from __future__ import annotations

from datetime import datetime

from PySide6.QtWidgets import QPlainTextEdit, QWidget


class LogWidget(QPlainTextEdit):
    """Read-only text panel that records every match event with a timestamp."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setReadOnly(True)
        self.setMaximumBlockCount(500)

    def append_line(self, text: str) -> None:
        """Appends one timestamped line to the log."""
        stamp = datetime.now().strftime("%H:%M:%S")
        self.appendPlainText(f"[{stamp}] {text}")

    def clear_log(self) -> None:
        """Empties the log."""
        self.clear()
