"""ErrorBarWidget — bottom message bar (figure region *Footer*: "Messaggi di errore")."""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

_IDLE_STYLE = "padding: 6px; color: #555;"
_ERROR_STYLE = "padding: 6px; color: #b00020; font-weight: bold;"


class ErrorBarWidget(QLabel):
    """Single-line bar that shows the last error or an idle placeholder."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        self.clear_error()

    def show_error(self, text: str) -> None:
        """Displays an error message in red."""
        self.setStyleSheet(_ERROR_STYLE)
        self.setText(text)

    def clear_error(self) -> None:
        """Resets the bar to its idle placeholder."""
        self.setStyleSheet(_IDLE_STYLE)
        self.setText("Nessun errore.")
