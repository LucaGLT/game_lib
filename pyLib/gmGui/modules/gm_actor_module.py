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
    QFrame,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QPushButton,
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
        container.setStyleSheet(
            "QWidget { background: #f7f9fc; color: #1f2a37; }"
            "QComboBox, QLineEdit {"
            "  background: #ffffff; border: 1px solid #d7deea;"
            "  border-radius: 8px; padding: 6px 10px; }"
            "QPushButton {"
            "  background: #ffffff; border: 1px solid #d7deea;"
            "  border-radius: 8px; padding: 6px 12px; font-weight: 600; }"
            "QPushButton:hover { background: #eef4ff; border-color: #9fc0ff; }"
            "QTreeWidget {"
            "  background: #ffffff; border: 1px solid #d7deea; border-radius: 10px;"
            "  alternate-background-color: #f7faff; }"
            "QTreeWidget::item { padding: 5px; }"
            "QTreeWidget::item:selected { background: #e8f1ff; color: #0b58ca; }"
            "QGroupBox {"
            "  background: #ffffff; border: 1px solid #d7deea; border-radius: 10px;"
            "  margin-top: 10px; padding-top: 8px; font-weight: 600; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QListWidget { background: #ffffff; border: none; }"
        )

        splitter = QSplitter(Qt.Orientation.Horizontal)
        outer = QHBoxLayout(container)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(splitter)

        # ── Left pane ─────────────────────────────────────────────────────────
        left = QWidget()
        vbox_left = QVBoxLayout(left)
        vbox_left.setContentsMargins(8, 8, 8, 8)
        vbox_left.setSpacing(8)

        top_row = QHBoxLayout()
        top_row.setSpacing(8)

        self._filter_combo: QComboBox = QComboBox()
        self._filter_combo.addItem("Tutti")
        self._filter_combo.currentTextChanged.connect(self._on_filter_changed)
        self._filter_combo.setMinimumWidth(150)
        top_row.addWidget(self._filter_combo)

        self._search_edit: QLineEdit = QLineEdit()
        self._search_edit.setPlaceholderText("Cerca attore...")
        self._search_edit.textChanged.connect(self._on_search_changed)
        top_row.addWidget(self._search_edit, 1)

        self._toggle_details_btn: QPushButton = QPushButton("Nascondi dettagli")
        self._toggle_details_btn.clicked.connect(self._toggle_details_panel)
        top_row.addWidget(self._toggle_details_btn)
        vbox_left.addLayout(top_row)

        self._tree: QTreeWidget = QTreeWidget()
        self._tree.setColumnCount(3)
        self._tree.setHeaderLabels(["Nome", "HP", "Stato"])
        self._tree.header().setStretchLastSection(True)
        self._tree.setRootIsDecorated(True)
        self._tree.setAlternatingRowColors(True)
        self._tree.setUniformRowHeights(True)
        self._tree.setColumnWidth(_COL_NAME, 230)
        self._tree.setColumnWidth(_COL_HP, 90)
        self._tree.itemSelectionChanged.connect(self._on_selection_changed)
        vbox_left.addWidget(self._tree)

        splitter.addWidget(left)

        # ── Right pane: detail panel ──────────────────────────────────────────
        right = QWidget()
        self._right_panel: QWidget = right
        self._details_visible: bool = True
        self._status_expanded: bool = True
        self._equip_expanded: bool = True
        vbox_right = QVBoxLayout(right)
        vbox_right.setContentsMargins(8, 8, 8, 8)
        vbox_right.setSpacing(8)

        header_card = QFrame()
        header_card.setStyleSheet(
            "QFrame { background: #ffffff; border: 1px solid #d7deea; border-radius: 10px; }"
        )
        header_layout = QVBoxLayout(header_card)
        header_layout.setContentsMargins(12, 10, 12, 10)
        header_layout.setSpacing(4)

        self._detail_name: QLabel = QLabel("Seleziona un attore")
        self._detail_name.setStyleSheet("font-size: 17px; font-weight: 700; color: #1f3c88;")
        header_layout.addWidget(self._detail_name)

        meta_row = QHBoxLayout()
        self._detail_faction: QLabel = QLabel("—")
        self._detail_faction.setStyleSheet(
            "QLabel { background: #eef4ff; color: #2251b3; border-radius: 10px; padding: 2px 8px; }"
        )
        meta_row.addWidget(self._detail_faction)
        self._detail_state: QLabel = QLabel("ALIVE")
        self._detail_state.setStyleSheet(
            "QLabel { background: #e9f8ef; color: #16814a; border-radius: 10px; padding: 2px 8px; font-weight: 700; }"
        )
        meta_row.addWidget(self._detail_state)
        meta_row.addStretch()
        header_layout.addLayout(meta_row)
        vbox_right.addWidget(header_card)

        self._hp_bar: HpBar = HpBar()
        vbox_right.addWidget(self._hp_bar)

        status_group = QGroupBox()
        vbox_status = QVBoxLayout(status_group)
        status_header = QHBoxLayout()
        status_title = QLabel("Status")
        status_title.setStyleSheet("font-weight: 700; color: #1f2a37;")
        status_header.addWidget(status_title)
        status_header.addStretch()
        self._btn_toggle_status: QPushButton = QPushButton("Nascondi ▲")
        self._btn_toggle_status.setMaximumWidth(110)
        self._btn_toggle_status.clicked.connect(self._toggle_status_section)
        status_header.addWidget(self._btn_toggle_status)
        vbox_status.addLayout(status_header)
        self._status_list: QListWidget = QListWidget()
        self._status_list.setMinimumHeight(90)
        vbox_status.addWidget(self._status_list)
        vbox_right.addWidget(status_group)

        equip_group = QGroupBox()
        vbox_equip = QVBoxLayout(equip_group)
        equip_header = QHBoxLayout()
        equip_title = QLabel("Equipaggiamento")
        equip_title.setStyleSheet("font-weight: 700; color: #1f2a37;")
        equip_header.addWidget(equip_title)
        equip_header.addStretch()
        self._btn_toggle_equip: QPushButton = QPushButton("Nascondi ▲")
        self._btn_toggle_equip.setMaximumWidth(110)
        self._btn_toggle_equip.clicked.connect(self._toggle_equip_section)
        equip_header.addWidget(self._btn_toggle_equip)
        vbox_equip.addLayout(equip_header)
        self._equip_list: QListWidget = QListWidget()
        self._equip_list.setMinimumHeight(90)
        vbox_equip.addWidget(self._equip_list)
        vbox_right.addWidget(equip_group)

        splitter.addWidget(right)
        splitter.setSizes([300, 200])
        splitter.setStretchFactor(0, 2)
        splitter.setStretchFactor(1, 1)

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
        # Full reset: every snapshot is authoritative.
        self._tree.clear()
        self._actor_items = {}
        self._faction_items = {}
        self._actor_data = {}
        self._filter_combo.blockSignals(True)
        current_filter = self._filter_combo.currentText()
        while self._filter_combo.count() > 1:
            self._filter_combo.removeItem(1)
        self._filter_combo.blockSignals(False)

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

        self._refresh_faction_labels()
        # Expand all faction groups and apply filters.
        self._tree.expandAll()
        self._apply_filters()

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
        """Applies faction filter combined with text search."""
        _ = faction
        self._apply_filters()

    def _on_search_changed(self, _: str) -> None:
        """Applies text search combined with faction filter."""
        self._apply_filters()

    def _toggle_details_panel(self) -> None:
        """Shows/hides the right detail panel."""
        self._details_visible = not self._details_visible
        self._right_panel.setVisible(self._details_visible)
        self._toggle_details_btn.setText(
            "Nascondi dettagli" if self._details_visible else "Mostra dettagli"
        )

    def _toggle_status_section(self) -> None:
        """Collapses/expands the Status section content."""
        self._status_expanded = not self._status_expanded
        self._status_list.setVisible(self._status_expanded)
        self._btn_toggle_status.setText(
            "Nascondi ▲" if self._status_expanded else "Mostra ▼"
        )

    def _toggle_equip_section(self) -> None:
        """Collapses/expands the Equipaggiamento section content."""
        self._equip_expanded = not self._equip_expanded
        self._equip_list.setVisible(self._equip_expanded)
        self._btn_toggle_equip.setText(
            "Nascondi ▲" if self._equip_expanded else "Mostra ▼"
        )

    def _apply_filters(self) -> None:
        """Applies faction and text filters to tree groups and actor rows."""
        faction: str = self._filter_combo.currentText()
        query: str = self._search_edit.text().strip().lower()

        for fid, fitem in self._faction_items.items():
            faction_ok = (faction == "Tutti" or fid == faction)
            visible_children = 0

            for i in range(fitem.childCount()):
                actor_item = fitem.child(i)
                actor_id = str(actor_item.data(_COL_NAME, Qt.ItemDataRole.UserRole) or "")
                actor_data = self._actor_data.get(actor_id, {})
                actor_name = str(actor_data.get("name", "")).lower()
                actor_faction = str(actor_data.get("faction_id", "")).lower()

                text_ok = (
                    not query
                    or query in actor_name
                    or query in actor_id.lower()
                    or query in actor_faction
                )
                row_visible = faction_ok and text_ok
                actor_item.setHidden(not row_visible)
                if row_visible:
                    visible_children += 1

            fitem.setHidden(visible_children == 0)
            if visible_children > 0:
                fitem.setExpanded(True)

    def _refresh_faction_labels(self) -> None:
        """Updates faction root captions with actor counts."""
        for faction_id, fitem in self._faction_items.items():
            count = fitem.childCount()
            fitem.setText(_COL_NAME, f"{faction_id} ({count})")

    def _on_selection_changed(self) -> None:
        actor_id: str | None = self._selected_actor_id()
        if actor_id is not None and actor_id in self._actor_data:
            self._refresh_detail(actor_id)
        else:
            self._detail_name.setText("Seleziona un attore")
            self._detail_faction.setText("—")
            self._detail_state.setText("ALIVE")
            self._detail_state.setStyleSheet(
                "QLabel { background: #e9f8ef; color: #16814a; border-radius: 10px; "
                "padding: 2px 8px; font-weight: 700; }"
            )
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
        self._detail_faction.setText(d["faction_id"])
        state = str(d["life_state"])
        self._detail_state.setText(state)
        if state == "ALIVE":
            self._detail_state.setStyleSheet(
                "QLabel { background: #e9f8ef; color: #16814a; border-radius: 10px; "
                "padding: 2px 8px; font-weight: 700; }"
            )
        elif state == "DYING":
            self._detail_state.setStyleSheet(
                "QLabel { background: #fff4e5; color: #9a5a00; border-radius: 10px; "
                "padding: 2px 8px; font-weight: 700; }"
            )
        else:
            self._detail_state.setStyleSheet(
                "QLabel { background: #f0f2f5; color: #58606b; border-radius: 10px; "
                "padding: 2px 8px; font-weight: 700; }"
            )
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
