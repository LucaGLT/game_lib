"""ErrorBarWidget — bottom message bar (figure region *Footer*: "Messaggi di errore")."""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget


class ErrorBarWidget(QLabel):
    """Single-line bar that shows the last error or an idle placeholder."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setObjectName("tris_error_bar")
        self.setProperty("text_role", "secondary")
        self.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        self.clear_error()

    def show_error(self, text: str) -> None:
        """Displays an error message in red."""
        self.setProperty("severity", "error")
        self.setText(text)
        self._refresh_style()

    def clear_error(self) -> None:
        """Resets the bar to its idle placeholder."""
        self.setProperty("severity", "idle")
        self.setText("Nessun errore.")
        self._refresh_style()

    def _refresh_style(self) -> None:
        """Re-polishes after dynamic property updates."""
        style = self.style()
        if style is not None:
            style.unpolish(self)
            style.polish(self)
        self.update()
