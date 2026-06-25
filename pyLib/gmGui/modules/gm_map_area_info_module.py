"""GmMapAreaInfoModule — contents of the currently selected map area.

Shows, in a single dock widget, two separate lists for the area selected on the
map:

- **Actors** present in the area.
- **Interactables** (objects the player can interact with) in the area.

The lists are populated from a :data:`gmGui.message_ids.AREA_INFO_RESPONSE`
envelope sent by the CoreEngine in reply to an area-info request. This module
is fully game-agnostic and reusable by any game that adopts the area-info
contract.
"""
from __future__ import annotations

import json as _json

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QGroupBox,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QVBoxLayout,
    QWidget,
)

from ..message_ids import AREA_INFO_RESPONSE
from .base_module import BaseModule


class GmMapAreaInfoModule(BaseModule):
    """Displays actors and interactables for the selected map area.

    TypeIds: :data:`gmGui.message_ids.AREA_INFO_RESPONSE`.
    """

    @property
    def module_id(self) -> str:
        return "gm_map_area_info"

    @property
    def title(self) -> str:
        return "Area Info"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.RightDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [AREA_INFO_RESPONSE]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        self._current_area_id: str = ""

        container = QWidget()
        container.setObjectName("gm_map_area_info_module")
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(8, 8, 8, 8)
        vbox.setSpacing(8)

        self._area_label: QLabel = QLabel("Nessuna area selezionata")
        self._area_label.setObjectName("area_info_header")
        self._area_label.setProperty("text_role", "title")
        vbox.addWidget(self._area_label)

        actors_group = QGroupBox()
        actors_group.setObjectName("area_info_actors_group")
        actors_box = QVBoxLayout(actors_group)
        actors_box.setContentsMargins(8, 8, 8, 8)
        actors_box.setSpacing(4)
        actors_title = QLabel("Attori nell'area")
        actors_title.setProperty("text_role", "subtitle")
        actors_box.addWidget(actors_title)
        self._actors_list: QListWidget = QListWidget()
        self._actors_list.setObjectName("area_info_actors_list")
        self._actors_list.setMinimumHeight(120)
        actors_box.addWidget(self._actors_list)
        vbox.addWidget(actors_group)

        interactables_group = QGroupBox()
        interactables_group.setObjectName("area_info_interactables_group")
        interactables_box = QVBoxLayout(interactables_group)
        interactables_box.setContentsMargins(8, 8, 8, 8)
        interactables_box.setSpacing(4)
        interactables_title = QLabel("Oggetti interagibili")
        interactables_title.setProperty("text_role", "subtitle")
        interactables_box.addWidget(interactables_title)
        self._interactables_list: QListWidget = QListWidget()
        self._interactables_list.setObjectName("area_info_interactables_list")
        self._interactables_list.setMinimumHeight(120)
        interactables_box.addWidget(self._interactables_list)
        vbox.addWidget(interactables_group)

        vbox.addStretch()
        return container

    # ── Envelope routing ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = str(msg.get("typeId", ""))
        if tid != AREA_INFO_RESPONSE:
            return

        data: dict = self._extract_payload(msg)
        self._populate(data)

    # ── Helpers ────────────────────────────────────────────────────────────────

    @staticmethod
    def _extract_payload(msg: dict) -> dict:
        """Returns the payload dict from either ``data`` or ``headers.data``."""
        data = msg.get("data")
        if isinstance(data, dict) and data:
            return data
        raw = msg.get("headers", {}).get("data", "{}")
        try:
            parsed = _json.loads(raw) if isinstance(raw, str) else raw
        except Exception:
            parsed = {}
        return parsed if isinstance(parsed, dict) else {}

    def _populate(self, data: dict) -> None:
        """Refreshes both lists from an area-info response payload."""
        area_id: str = str(data.get("area_id", ""))
        self._current_area_id = area_id
        self._area_label.setText(
            f"Area: {area_id}" if area_id else "Nessuna area selezionata"
        )

        self._actors_list.clear()
        for actor in data.get("actors", []):
            self._actors_list.addItem(self._format_entry(actor))

        self._interactables_list.clear()
        for item in data.get("interactables", []):
            self._interactables_list.addItem(self._format_entry(item))

    @staticmethod
    def _format_entry(entry: dict) -> QListWidgetItem:
        """Builds a list item from an actor/interactable dict."""
        entry_id: str = str(entry.get("id", "?"))
        name: str = str(entry.get("name", entry_id))
        meta: str = str(entry.get("type", entry.get("faction", "")))
        state: str = str(entry.get("state", ""))
        label: str = name
        if meta:
            label += f" [{meta}]"
        if state:
            label += f" — {state}"
        list_item = QListWidgetItem(label)
        list_item.setData(Qt.ItemDataRole.UserRole, entry_id)
        return list_item

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns the last selected area id for QSettings persistence."""
        if self._widget is None:
            return {}
        return {"area_id": self._current_area_id}

    def restore_state(self, state: dict) -> None:
        """Restores the header label from a previously saved state dict."""
        if self._widget is None:
            return
        area_id = str(state.get("area_id", ""))
        if area_id:
            self._current_area_id = area_id
            self._area_label.setText(f"Area: {area_id}")
