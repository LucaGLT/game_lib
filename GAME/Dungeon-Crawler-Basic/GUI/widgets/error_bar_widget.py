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
        # ToBeImplemented //

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and shows feedback if relevant.

        Handles: ``dungeon.action.rejected``.

        Args:
            msg: Decoded event dict with ``typeId`` and ``data`` keys.
        """
        # ToBeImplemented //

    def show_error(self, message: str) -> None:
        """Displays an error message in the bar for a short duration.

        Args:
            message: Human-readable error string.
        """
        # ToBeImplemented //

    def show_info(self, message: str) -> None:
        """Displays a neutral informational message in the bar.

        Args:
            message: Human-readable info string.
        """
        # ToBeImplemented //

    def clear(self) -> None:
        """Hides any displayed message and returns the bar to neutral state."""
        # ToBeImplemented //
