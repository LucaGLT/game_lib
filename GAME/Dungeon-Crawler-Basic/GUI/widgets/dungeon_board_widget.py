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
        from PySide6.QtWidgets import QGridLayout, QPushButton, QLabel
        from PySide6.QtCore import Qt
        outer_layout = __import__('PySide6.QtWidgets', fromlist=['QVBoxLayout']).QVBoxLayout(self)
        outer_layout.setContentsMargins(4, 4, 4, 4)
        self._grid_widget = QWidget()
        self._grid = QGridLayout(self._grid_widget)
        self._grid.setSpacing(4)
        outer_layout.addWidget(self._grid_widget)
        outer_layout.addStretch()
        self._rooms: dict[str, list] = {}       # room_id -> [button, row, col]
        self._actor_locations: dict[str, str] = {}  # actor_id -> room_id
        self._hero_id: str = ""
        self._hero_room: str = ""
        self._adjacent_rooms: set[str] = set()

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and updates the board display."""
        type_id = msg.get("typeId", "")
        data = msg.get("data", {})
        if type_id == "dungeon.map.snapshot":
            self.reset()
            self._build_from_snapshot(data)
        elif type_id == "dungeon.actor.snapshot":
            for actor in data.get("actors", []):
                self._actor_locations[actor["id"]] = actor.get("location", "")
                if actor.get("kind") == "HERO":
                    self._hero_id = actor["id"]
                    self._hero_room = actor.get("location", "")
            self._refresh_adjacency()
            self._refresh_room_styles()
        elif type_id == "dungeon.actor.moved":
            actor_id = data.get("actor_id", "")
            dest = data.get("to", "")
            self._actor_locations[actor_id] = dest
            if actor_id == self._hero_id:
                self._hero_room = dest
                self._refresh_adjacency()
            self._refresh_room_styles()
        elif type_id == "dungeon.game.over":
            self._set_all_clickable(False)

    def _build_from_snapshot(self, data: dict) -> None:
        from PySide6.QtWidgets import QPushButton
        rooms = data.get("rooms", [])
        # Lay rooms out in a simple row (order from snapshot).
        for col, room in enumerate(rooms):
            room_id = room["id"]
            btn = QPushButton(room_id)
            btn.setMinimumWidth(80)
            btn.setMinimumHeight(50)
            btn.clicked.connect(lambda _checked=False, rid=room_id: self._on_room_clicked(rid))
            self._grid.addWidget(btn, 0, col)
            self._rooms[room_id] = [btn, 0, col, room.get("adjacent", [])]
        self._refresh_room_styles()

    def _refresh_adjacency(self) -> None:
        if self._hero_room and self._hero_room in self._rooms:
            self._adjacent_rooms = set(self._rooms[self._hero_room][3])
        else:
            self._adjacent_rooms = set()

    def _refresh_room_styles(self) -> None:
        hero_rooms_with_actors = set(self._actor_locations.values())
        for room_id, room_data in self._rooms.items():
            btn = room_data[0]
            actors_here = [aid for aid, loc in self._actor_locations.items() if loc == room_id]
            label = room_id
            if actors_here:
                label += "\n" + ", ".join(actors_here)
            btn.setText(label)
            is_hero_room = (room_id == self._hero_room)
            is_adjacent = room_id in self._adjacent_rooms
            btn.setEnabled(is_adjacent and not is_hero_room)

    def _set_all_clickable(self, enabled: bool) -> None:
        for room_data in self._rooms.values():
            room_data[0].setEnabled(enabled)

    def _on_room_clicked(self, room_id: str) -> None:
        """Emits move_requested when the player clicks an adjacent room."""
        if self._hero_id and room_id in self._adjacent_rooms:
            self.move_requested.emit(self._hero_id, room_id)

    def reset(self) -> None:
        """Clears all rooms and resets the display to an empty state."""
        for room_data in self._rooms.values():
            btn = room_data[0]
            self._grid.removeWidget(btn)
            btn.deleteLater()
        self._rooms.clear()
        self._actor_locations.clear()
        self._hero_id = ""
        self._hero_room = ""
        self._adjacent_rooms = set()
