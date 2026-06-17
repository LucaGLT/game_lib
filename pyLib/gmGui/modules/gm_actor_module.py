"""GmActorModule — Actor tree and detail panel.

Subscribes to ``gmActor.snapshot`` (initial state) and the seven actor
lifecycle events defined in ``ActorEvents.hpp``.

Layout
------
- Left pane:  faction filter :class:`QComboBox` + 3-column
  :class:`QTreeWidget` (Name / HP / State), rows grouped by faction.
- Right pane: detail panel — actor name, :class:`~gmGui.widgets.hp_bar.HpBar`,
  status-effects list, equipment list.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QColor
from PySide6.QtWidgets import (
    QComboBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QSplitter,
    QTreeWidget,
    QTreeWidgetItem,
    QVBoxLayout,
    QWidget,
)

from ..widgets.hp_bar import HpBar
from .base_module import BaseModule

# ── Tree column indices ────────────────────────────────────────────────────────
_COL_NAME: int = 0
_COL_HP: int = 1
_COL_STATE: int = 2

# ── Life-state → row foreground colour ───────────────────────────────────────
_LIFE_STATE_COLORS: dict[str, QColor | None] = {
    "ALIVE": None,
    "DYING": QColor(Qt.GlobalColor.red),
    "DEAD": QColor(Qt.GlobalColor.gray),
}


class GmActorModule(BaseModule):
    """Visualises gmActor state: actor tree, HP bar, statuses, equipment.

    TypeIds from ``ActorEvents.hpp``.
    """

    @property
    def module_id(self) -> str:
        return "gm_actor"

    @property
    def title(self) -> str:
        return "Actors"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.RightDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmActor.snapshot",
            "gmActor.actor.hp_changed",
            "gmActor.actor.status_added",
            "gmActor.actor.status_removed",
            "gmActor.actor.moved_area",
            "gmActor.actor.life_state_changed",
            "gmActor.actor.item_equipped",
            "gmActor.actor.item_unequipped",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        # ── In-memory data model ──────────────────────────────────────────────
        self._actor_items: dict[str, QTreeWidgetItem] = {}
        self._faction_items: dict[str, QTreeWidgetItem] = {}
        self._actor_data: dict[str, dict] = {}

        container = QWidget()
        splitter = QSplitter(Qt.Orientation.Horizontal)
        outer = QHBoxLayout(container)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(splitter)

        # ── Left pane ─────────────────────────────────────────────────────────
        left = QWidget()
        vbox_left = QVBoxLayout(left)
        vbox_left.setContentsMargins(4, 4, 4, 4)

        self._filter_combo: QComboBox = QComboBox()
        self._filter_combo.addItem("Tutti")
        self._filter_combo.currentTextChanged.connect(self._on_filter_changed)
        vbox_left.addWidget(self._filter_combo)

        self._tree: QTreeWidget = QTreeWidget()
        self._tree.setColumnCount(3)
        self._tree.setHeaderLabels(["Nome", "HP", "Stato"])
        self._tree.header().setStretchLastSection(True)
        self._tree.itemSelectionChanged.connect(self._on_selection_changed)
        vbox_left.addWidget(self._tree)

        splitter.addWidget(left)

        # ── Right pane: detail panel ──────────────────────────────────────────
        right = QWidget()
        vbox_right = QVBoxLayout(right)
        vbox_right.setContentsMargins(4, 4, 4, 4)

        self._detail_name: QLabel = QLabel("—")
        vbox_right.addWidget(self._detail_name)

        self._hp_bar: HpBar = HpBar()
        vbox_right.addWidget(self._hp_bar)

        status_group = QGroupBox("Status")
        vbox_status = QVBoxLayout(status_group)
        self._status_list: QListWidget = QListWidget()
        vbox_status.addWidget(self._status_list)
        vbox_right.addWidget(status_group)

        equip_group = QGroupBox("Equipaggiamento")
        vbox_equip = QVBoxLayout(equip_group)
        self._equip_list: QListWidget = QListWidget()
        vbox_equip.addWidget(self._equip_list)
        vbox_right.addWidget(equip_group)

        splitter.addWidget(right)
        splitter.setSizes([300, 200])

        return container

    # ── Envelope handler ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {})

        if tid == "gmActor.snapshot":
            self._handle_snapshot(data)
        elif tid == "gmActor.actor.hp_changed":
            self._handle_hp_changed(data)
        elif tid == "gmActor.actor.status_added":
            self._handle_status_added(data)
        elif tid == "gmActor.actor.status_removed":
            self._handle_status_removed(data)
        elif tid == "gmActor.actor.moved_area":
            self._handle_moved_area(data)
        elif tid == "gmActor.actor.life_state_changed":
            self._handle_life_state_changed(data)
        elif tid == "gmActor.actor.item_equipped":
            self._handle_item_equipped(data)
        elif tid == "gmActor.actor.item_unequipped":
            self._handle_item_unequipped(data)

    # ── Private handlers ──────────────────────────────────────────────────────

    def _handle_snapshot(self, data: dict) -> None:
        for actor in data.get("actors", []):
            actor_id: str = str(actor.get("actor_id", ""))
            if not actor_id:
                continue
            faction_id: str = str(actor.get("faction_id", "unknown"))
            name: str = str(actor.get("name", actor_id))
            current_hp: int = int(actor.get("current_hp", 0))
            max_hp: int = int(actor.get("max_hp", 1))
            life_state: str = str(actor.get("life_state", "ALIVE"))

            self._actor_data[actor_id] = {
                "name": name,
                "faction_id": faction_id,
                "current_hp": current_hp,
                "max_hp": max_hp,
                "life_state": life_state,
                "statuses": dict(actor.get("statuses", {})),
                "equipment": dict(actor.get("equipment", {})),
                "area_id": str(actor.get("area_id", "")),
            }

            # Faction root item — create once
            if faction_id not in self._faction_items:
                fitem = QTreeWidgetItem(self._tree)
                fitem.setText(_COL_NAME, faction_id)
                self._faction_items[faction_id] = fitem
                if self._filter_combo.findText(faction_id) < 0:
                    self._filter_combo.addItem(faction_id)

            # Actor leaf item
            item = QTreeWidgetItem(self._faction_items[faction_id])
            item.setText(_COL_NAME, name)
            item.setText(_COL_HP, f"{current_hp}/{max_hp}")
            item.setData(_COL_NAME, Qt.ItemDataRole.UserRole, actor_id)
            self._actor_items[actor_id] = item
            self._update_state_column(actor_id)
            self._apply_life_state_color(item, life_state)

    def _handle_hp_changed(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        current_hp: int = int(data.get("new_hp", 0))
        max_hp: int = int(data.get("max_hp", self._actor_data[actor_id]["max_hp"]))
        self._actor_data[actor_id]["current_hp"] = current_hp
        self._actor_data[actor_id]["max_hp"] = max_hp
        self._actor_items[actor_id].setText(_COL_HP, f"{current_hp}/{max_hp}")
        if self._selected_actor_id() == actor_id:
            self._hp_bar.set_hp(current_hp, max_hp)

    def _handle_status_added(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        status_id: str = str(data.get("status_id", ""))
        stacks: int = int(data.get("stacks", 1))
        self._actor_data[actor_id]["statuses"][status_id] = stacks
        self._update_state_column(actor_id)
        if self._selected_actor_id() == actor_id:
            self._refresh_status_list(actor_id)

    def _handle_status_removed(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        self._actor_data[actor_id]["statuses"].pop(str(data.get("status_id", "")), None)
        self._update_state_column(actor_id)
        if self._selected_actor_id() == actor_id:
            self._refresh_status_list(actor_id)

    def _handle_moved_area(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        new_area: str = str(data.get("new_area", ""))
        self._actor_data[actor_id]["area_id"] = new_area
        self._actor_items[actor_id].setToolTip(_COL_NAME, f"Area: {new_area}")

    def _handle_life_state_changed(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        new_state: str = str(data.get("new_state", "ALIVE"))
        self._actor_data[actor_id]["life_state"] = new_state
        self._update_state_column(actor_id)
        self._apply_life_state_color(self._actor_items[actor_id], new_state)

    def _handle_item_equipped(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        slot: str = str(data.get("slot", ""))
        item_id: str = str(data.get("item_instance_id", ""))
        self._actor_data[actor_id]["equipment"][slot] = item_id
        if self._selected_actor_id() == actor_id:
            self._refresh_equip_list(actor_id)

    def _handle_item_unequipped(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        self._actor_data[actor_id]["equipment"].pop(str(data.get("slot", "")), None)
        if self._selected_actor_id() == actor_id:
            self._refresh_equip_list(actor_id)

    # ── UI helpers ────────────────────────────────────────────────────────────

    def _on_filter_changed(self, faction: str) -> None:
        """Shows only the selected faction group; 'Tutti' shows all."""
        for fid, fitem in self._faction_items.items():
            fitem.setHidden(faction != "Tutti" and fid != faction)

    def _on_selection_changed(self) -> None:
        actor_id: str | None = self._selected_actor_id()
        if actor_id is not None and actor_id in self._actor_data:
            self._refresh_detail(actor_id)
        else:
            self._detail_name.setText("—")
            self._hp_bar.set_hp(0, 1)
            self._status_list.clear()
            self._equip_list.clear()

    def _selected_actor_id(self) -> str | None:
        """Returns the actor_id of the currently selected tree item, or None."""
        items: list[QTreeWidgetItem] = self._tree.selectedItems()
        if not items:
            return None
        return items[0].data(_COL_NAME, Qt.ItemDataRole.UserRole)

    def _refresh_detail(self, actor_id: str) -> None:
        d: dict = self._actor_data[actor_id]
        self._detail_name.setText(d["name"])
        self._hp_bar.set_hp(d["current_hp"], d["max_hp"])
        self._refresh_status_list(actor_id)
        self._refresh_equip_list(actor_id)

    def _refresh_status_list(self, actor_id: str) -> None:
        self._status_list.clear()
        for status_id, stacks in self._actor_data[actor_id]["statuses"].items():
            self._status_list.addItem(f"{status_id} x{stacks}")

    def _refresh_equip_list(self, actor_id: str) -> None:
        self._equip_list.clear()
        for slot, item_id in self._actor_data[actor_id]["equipment"].items():
            self._equip_list.addItem(f"{slot}: {item_id}")

    def _update_state_column(self, actor_id: str) -> None:
        """Updates the State column: 'ALIVE (N)' when N statuses are active."""
        life_state: str = self._actor_data[actor_id]["life_state"]
        count: int = len(self._actor_data[actor_id]["statuses"])
        text: str = life_state if count == 0 else f"{life_state} ({count})"
        self._actor_items[actor_id].setText(_COL_STATE, text)

    @staticmethod
    def _apply_life_state_color(item: QTreeWidgetItem, life_state: str) -> None:
        """Applies DYING→red / DEAD→gray / ALIVE→default foreground to all columns."""
        color: QColor | None = _LIFE_STATE_COLORS.get(life_state)
        for col in range(3):
            item.setForeground(col, QBrush(color) if color is not None else QBrush())
