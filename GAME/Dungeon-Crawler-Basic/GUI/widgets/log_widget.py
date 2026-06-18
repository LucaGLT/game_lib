"""Dungeon Crawler Basic — game log widget.

LogWidget displays a scrollable list of game log messages emitted by the
CoreEngine. Each entry shows a timestamp, an actor identifier and a
human-readable description of the event (action performed, rejection reason,
session start/end, etc.).

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class LogWidget(QWidget):
    """Scrollable read-only log of game events.

    Appends one entry per relevant event received from the CoreEngine.
    Older entries are kept visible for the duration of the session.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates an empty log widget."""
        super().__init__(parent)
        # ToBeImplemented //

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and appends a log entry if relevant.

        Handles: ``dungeon.actor.moved``, ``dungeon.actor.healed``,
        ``dungeon.actor.equipped``, ``dungeon.action.rejected``,
        ``dungeon.session.started``, ``dungeon.game.over``.

        Args:
            msg: Decoded event dict with ``typeId`` and ``data`` keys.
        """
        # ToBeImplemented //

    def append_entry(self, text: str) -> None:
        """Appends a formatted text entry to the log.

        Args:
            text: Human-readable log message string.
        """
        # ToBeImplemented //

    def clear(self) -> None:
        """Removes all log entries."""
        # ToBeImplemented //
