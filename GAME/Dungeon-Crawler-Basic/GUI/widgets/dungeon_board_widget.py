"""Dungeon Crawler Basic — dungeon board widget.

DungeonBoardWidget renders the dungeon map as a grid of clickable room tiles.
Each tile represents a room loaded from the JSON map file; clicking a tile
adjacent to the hero triggers a ``dungeon.move`` command.

All visual styling is applied exclusively through QSS; no literal color or
font values appear in this file.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Signal


class DungeonBoardWidget(QWidget):
    """Clickable dungeon map tile grid.

    Displays rooms loaded from a ``dungeon.map.snapshot`` event and updates
    actor positions on ``dungeon.actor.moved`` events. Emits :attr:`move_requested`
    when the player clicks a valid adjacent room.

    Signals:
        move_requested(hero_id, destination): Emitted when the player clicks a
            room adjacent to the hero.
    """

    move_requested: Signal = Signal(str, str)

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the widget with an empty board."""
        super().__init__(parent)
        # ToBeImplemented //

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and updates the board display.

        Handles: ``dungeon.map.snapshot``, ``dungeon.actor.moved``,
        ``dungeon.actor.snapshot``, ``dungeon.game.over``.

        Args:
            msg: Decoded event dict with ``typeId`` and ``data`` keys.
        """
        # ToBeImplemented //

    def _on_room_clicked(self, room_id: str) -> None:
        """Internal handler for a room tile click.

        Args:
            room_id: Identifier of the clicked room.
        """
        # ToBeImplemented //

    def reset(self) -> None:
        """Clears all rooms and resets the display to an empty state."""
        # ToBeImplemented //
