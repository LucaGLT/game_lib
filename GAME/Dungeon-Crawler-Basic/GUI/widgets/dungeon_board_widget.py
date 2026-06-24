"""Dungeon Crawler Basic — dungeon board adapter.

DungeonBoardWidget keeps the public Dungeon Crawler board API but delegates the
visual rendering to the shared :class:`gmGui.modules.gm_map_module.GmMapModule`
widget so the dungeon map reuses the existing GameLib GUI scene and look.
"""
from __future__ import annotations

import json as _json

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QSizePolicy, QVBoxLayout, QWidget

from gmGui.modules.gm_map_module import GmMapModule
from gmGui.widgets.map_scene import LocationNode


class DungeonBoardWidget(QWidget):
    """Adapter widget that maps dungeon map events onto gmMap events.

    Signals:
        area_selected(area_id): Emitted when the player clicks any room. The
            click is view-only and never triggers a gameplay action; the main
            window uses it to request the area contents from the engine.
        move_requested(hero_id, destination): Retained for the explicit Move
            action; emitted by :meth:`request_move` only.
    """

    area_selected:  Signal = Signal(str)
    move_requested: Signal = Signal(str, str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self._layout = QVBoxLayout(self)
        self._layout.setContentsMargins(4, 4, 4, 4)
        self._layout.setSpacing(4)
        self._module: GmMapModule = GmMapModule()
        self._module_widget = self._module.widget()
        self._layout.addWidget(self._module_widget)
        self._room_index_by_id: dict[str, int] = {}
        self._room_id_by_index: dict[int, str] = {}
        self._adjacent_by_room: dict[str, set[str]] = {}
        self._hero_rooms: dict[str, str] = {}   # hero_id → room_id
        self._active_hero_id: str = ""          # whose turn it is
        self._selected_room: str = ""
        self._interaction_enabled: bool = True
        try:
            self._module._map_scene.selectionChanged.disconnect(self._module._on_selection_changed)
        except Exception:
            pass
        self._module._map_scene.selectionChanged.connect(self._on_selection_changed)

    def on_envelope(self, msg: dict) -> None:
        """Receives a dungeon envelope and forwards the translated map event."""
        type_id = str(msg.get("typeId", ""))
        data = msg.get("data", {})

        if type_id == "dungeon.session.started":
            self.reset()
            return

        if type_id == "dungeon.map.snapshot":
            self._room_index_by_id.clear()
            self._room_id_by_index.clear()
            self._adjacent_by_room.clear()
            translated = self._translate_map_snapshot(data)
            self._module.on_envelope(
                {
                    "typeId": "gmMap.map.loaded",
                    "headers": {"data": _json.dumps(translated)},
                    "data": translated,
                }
            )
            return

        if type_id == "dungeon.actor.snapshot":
            for envelope in self._translate_actor_snapshot(data):
                self._module.on_envelope(envelope)
            return

        if type_id == "dungeon.actor.moved":
            translated = self._translate_actor_moved(data)
            if translated is not None:
                self._module.on_envelope(translated)
            return

        if type_id == "dungeon.game.over":
            self._interaction_enabled = False
            self._module_widget.setEnabled(False)

    def reset(self) -> None:
        """Clears cached adapter state and recreates the embedded gmGui module."""
        self._room_index_by_id.clear()
        self._room_id_by_index.clear()
        self._adjacent_by_room.clear()
        self._hero_rooms.clear()
        self._active_hero_id = ""
        self._selected_room = ""
        self._interaction_enabled = True
        self._module = GmMapModule()
        self._layout.removeWidget(self._module_widget)
        self._module_widget.setParent(None)
        self._module_widget.deleteLater()
        self._module_widget = self._module.widget()
        self._layout.addWidget(self._module_widget)
        try:
            self._module._map_scene.selectionChanged.disconnect(self._module._on_selection_changed)
        except Exception:
            pass
        self._module._map_scene.selectionChanged.connect(self._on_selection_changed)

    def set_active_hero(self, hero_id: str) -> None:
        """Sets the hero whose turn it is (used by move_destination for adjacency)."""
        self._active_hero_id = hero_id

    def _translate_map_snapshot(self, data: dict) -> dict:
        locations: list[dict] = []
        edges: list[list[int]] = []
        seen_edges: set[tuple[int, int]] = set()

        for room in data.get("rooms", []):
            room_id = str(room.get("id", ""))
            if not room_id:
                continue
            room_index = self._room_index(room_id)
            self._adjacent_by_room[room_id] = {str(adjacent) for adjacent in room.get("adjacent", [])}
            tags = [str(tag) for tag in room.get("tags", [])]
            locations.append(
                {
                    "location_id": room_index,
                    "metadata": {
                        "terrain": tags[0] if tags else "",
                        "items": [room_id],
                    },
                }
            )

        for room_id, neighbours in self._adjacent_by_room.items():
            room_index = self._room_index(room_id)
            for neighbour in neighbours:
                neighbour_index = self._room_index(neighbour)
                edge = tuple(sorted((room_index, neighbour_index)))
                if edge not in seen_edges:
                    edges.append([edge[0], edge[1]])
                    seen_edges.add(edge)

        return {"locations": locations, "edges": edges}

    def _translate_actor_snapshot(self, data: dict) -> list[dict]:
        events: list[dict] = []
        for actor in data.get("actors", []):
            actor_id = str(actor.get("id", ""))
            room_id = str(actor.get("location", ""))
            if not actor_id or not room_id:
                continue
            self._room_index(room_id)
            if str(actor.get("kind", "")) == "HERO":
                self._hero_rooms[actor_id] = room_id
            events.append(
                {
                    "typeId": "gmActor.actor.position_changed",
                    "headers": {
                        "data": _json.dumps(
                            {
                                "actor_id": actor_id,
                                "new_location_id": self._room_index(room_id),
                            }
                        )
                    },
                    "data": {
                        "actor_id": actor_id,
                        "new_location_id": self._room_index(room_id),
                    },
                }
            )
        return events

    def _translate_actor_moved(self, data: dict) -> dict | None:
        actor_id = str(data.get("actor_id", ""))
        destination = str(data.get("to", ""))
        if not actor_id or not destination:
            return None
        if actor_id in self._hero_rooms:
            self._hero_rooms[actor_id] = destination
        return {
            "typeId": "gmActor.actor.position_changed",
            "headers": {
                "data": _json.dumps(
                    {
                        "actor_id": actor_id,
                        "new_location_id": self._room_index(destination),
                    }
                )
            },
            "data": {
                "actor_id": actor_id,
                "new_location_id": self._room_index(destination),
            },
        }

    def _selected_room_id(self) -> str | None:
        items = self._module._map_scene.selectedItems()
        if not items:
            return None
        node = items[0]
        if not isinstance(node, LocationNode):
            return None
        return self._room_id_by_index.get(node.loc_id)

    def _on_selection_changed(self) -> None:
        if not self._interaction_enabled:
            return
        destination = self._selected_room_id()
        if destination is None:
            self._selected_room = ""
            return
        # A click is view-only: remember the selection and ask the engine for
        # the area contents. It never triggers a gameplay action.
        self._selected_room = destination
        self.area_selected.emit(destination)

    def move_destination(self) -> str:
        """Returns the selected room if it is a valid adjacent move target, else ""."""
        hero_room = self._hero_rooms.get(self._active_hero_id, "")
        if not self._interaction_enabled or not self._active_hero_id or not hero_room:
            return ""
        destination = self._selected_room
        if not destination or destination == hero_room:
            return ""
        if destination in self._adjacent_by_room.get(hero_room, set()):
            return destination
        return ""

    def request_move(self) -> None:
        """Emits move_requested for the current valid selection (explicit Move action)."""
        destination = self.move_destination()
        if destination:
            self.move_requested.emit(self._active_hero_id, destination)

    def _room_index(self, room_id: str) -> int:
        if room_id not in self._room_index_by_id:
            index = len(self._room_index_by_id)
            self._room_index_by_id[room_id] = index
            self._room_id_by_index[index] = room_id
        return self._room_index_by_id[room_id]
