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

from typing import Callable

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QBrush
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
from ..theme_manager import resolve_semantic_color
from .base_module import BaseModule

# ── Tree column indices ────────────────────────────────────────────────────────
_COL_NAME: int = 0
_COL_HP: int = 1
_COL_STATE: int = 2
_TOGGLE_EXPANDED_ICON: str = "▾"
_TOGGLE_COLLAPSED_ICON: str = "▸"

class GmActorModule(BaseModule):
    """Visualises gmActor state: actor tree, HP bar, statuses, equipment.

    TypeIds from ``ActorEvents.hpp``.

    Callbacks:
        on_actor_selected(actor_id): invoked when the user selects an actor in
        the tree.  Set by the owner after instantiation.
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
            "gmActor.actor.resource_changed",
            "gmActor.actor.moved_area",
            "gmActor.actor.life_state_changed",
            "gmActor.actor.item_equipped",
            "gmActor.actor.item_unequipped",
            "gmActor.actor.removed",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        # ── In-memory data model ──────────────────────────────────────────────
        self._actor_items: dict[str, QTreeWidgetItem] = {}
        self._faction_items: dict[str, QTreeWidgetItem] = {}
        self._actor_data: dict[str, dict] = {}
        # Callback invoked when the user clicks an actor row in the tree.
        # Owner (e.g. HeroPanelWidget) sets this after calling widget().
        self.on_actor_selected: Callable[[str], None] | None = None

        container = QWidget()
        container.setObjectName("gm_actor_module")

        splitter = QSplitter(Qt.Orientation.Horizontal)
        outer = QHBoxLayout(container)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(splitter)

        # ── Left pane ─────────────────────────────────────────────────────────
        left = QWidget()
        self._left_panel: QWidget = left
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
        self._status_expanded: bool = True
        self._equip_expanded: bool = True
        vbox_right = QVBoxLayout(right)
        vbox_right.setContentsMargins(8, 8, 8, 8)
        vbox_right.setSpacing(8)

        header_card = QFrame()
        header_card.setObjectName("actor_header_card")
        header_layout = QVBoxLayout(header_card)
        header_layout.setContentsMargins(16, 8, 16, 8)
        header_layout.setSpacing(4)

        name_row = QHBoxLayout()
        self._detail_name: QLabel = QLabel("Seleziona un attore")
        self._detail_name.setObjectName("actor_detail_name")
        self._detail_name.setProperty("text_role", "title")
        name_row.addWidget(self._detail_name)
        name_row.addStretch()
        # The actor list is collapsible and hidden by default; the toggle lives
        # in the detail header so it stays reachable when the left pane is gone.
        self._toggle_tree_btn: QPushButton = QPushButton(_TOGGLE_EXPANDED_ICON)
        self._toggle_tree_btn.setToolTip("Mostra/Nascondi lista attori")
        self._toggle_tree_btn.setProperty("toggle_icon", "true")
        self._toggle_tree_btn.setFixedWidth(20)
        self._toggle_tree_btn.clicked.connect(self._toggle_actor_tree)
        name_row.addWidget(self._toggle_tree_btn)
        header_layout.addLayout(name_row)

        meta_row = QHBoxLayout()
        self._detail_faction: QLabel = QLabel("—")
        self._detail_faction.setObjectName("actor_detail_faction")
        self._detail_faction.setProperty("chip", "true")
        meta_row.addWidget(self._detail_faction)
        self._detail_state: QLabel = QLabel("ALIVE")
        self._detail_state.setObjectName("actor_detail_state")
        self._detail_state.setProperty("chip", "true")
        meta_row.addWidget(self._detail_state)
        meta_row.addStretch()
        header_layout.addLayout(meta_row)
        vbox_right.addWidget(header_card)

        self._apply_detail_state("ALIVE")

        self._hp_bar: HpBar = HpBar()
        vbox_right.addWidget(self._hp_bar)

        status_group = QGroupBox()
        vbox_status = QVBoxLayout(status_group)
        status_header = QHBoxLayout()
        status_title = QLabel("Status")
        status_title.setProperty("text_role", "subtitle")
        status_header.addWidget(status_title)
        status_header.addStretch()
        self._btn_toggle_status: QPushButton = QPushButton(_TOGGLE_EXPANDED_ICON)
        self._btn_toggle_status.setToolTip("Mostra/Nascondi sezione Status")
        self._btn_toggle_status.setProperty("toggle_icon", "true")
        self._btn_toggle_status.setFixedWidth(20)
        self._btn_toggle_status.clicked.connect(self._toggle_status_section)
        status_header.addWidget(self._btn_toggle_status)
        vbox_status.addLayout(status_header)
        self._status_list: QListWidget = QListWidget()
        self._status_list.setMinimumHeight(90)
        vbox_status.addWidget(self._status_list)
        vbox_right.addWidget(status_group)

        # ── Resources section ──────────────────────────────────────────────
        resource_group = QGroupBox()
        vbox_res = QVBoxLayout(resource_group)
        res_header = QHBoxLayout()
        res_title = QLabel("Risorse")
        res_title.setProperty("text_role", "subtitle")
        res_header.addWidget(res_title)
        res_header.addStretch()
        self._resource_expanded: bool = True
        self._btn_toggle_resource: QPushButton = QPushButton(_TOGGLE_EXPANDED_ICON)
        self._btn_toggle_resource.setToolTip("Mostra/Nascondi sezione Risorse")
        self._btn_toggle_resource.setProperty("toggle_icon", "true")
        self._btn_toggle_resource.setFixedWidth(20)
        self._btn_toggle_resource.clicked.connect(self._toggle_resource_section)
        res_header.addWidget(self._btn_toggle_resource)
        vbox_res.addLayout(res_header)
        self._resource_list: QListWidget = QListWidget()
        self._resource_list.setMinimumHeight(70)
        vbox_res.addWidget(self._resource_list)
        vbox_right.addWidget(resource_group)

        equip_group = QGroupBox()
        vbox_equip = QVBoxLayout(equip_group)
        equip_header = QHBoxLayout()
        equip_title = QLabel("Equipaggiamento")
        equip_title.setProperty("text_role", "subtitle")
        equip_header.addWidget(equip_title)
        equip_header.addStretch()
        self._btn_toggle_equip: QPushButton = QPushButton(_TOGGLE_EXPANDED_ICON)
        self._btn_toggle_equip.setToolTip("Mostra/Nascondi sezione Equipaggiamento")
        self._btn_toggle_equip.setProperty("toggle_icon", "true")
        self._btn_toggle_equip.setFixedWidth(20)
        self._btn_toggle_equip.clicked.connect(self._toggle_equip_section)
        equip_header.addWidget(self._btn_toggle_equip)
        vbox_equip.addLayout(equip_header)
        self._equip_list: QListWidget = QListWidget()
        self._equip_list.setMinimumHeight(90)
        vbox_equip.addWidget(self._equip_list)
        vbox_right.addWidget(equip_group)

        splitter.addWidget(right)
        self._splitter: QSplitter = splitter
        # Default state: actor list visible alongside the detail panel.
        self._tree_visible: bool = True
        self._left_panel.setVisible(True)
        splitter.setSizes([220, 320])
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 2)

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
        elif tid == "gmActor.actor.resource_changed":
            self._handle_resource_changed(data)
        elif tid == "gmActor.actor.moved_area":
            self._handle_moved_area(data)
        elif tid == "gmActor.actor.life_state_changed":
            self._handle_life_state_changed(data)
        elif tid == "gmActor.actor.item_equipped":
            self._handle_item_equipped(data)
        elif tid == "gmActor.actor.item_unequipped":
            self._handle_item_unequipped(data)
        elif tid == "gmActor.actor.removed":
            self._handle_actor_removed(data)

    # ── Private handlers ──────────────────────────────────────────────────────

    def _handle_snapshot(self, data: dict) -> None:
        # Merge snapshot content into current tree to preserve existing factions/actors
        # when partial snapshots are received.
        self._filter_combo.blockSignals(True)
        current_filter = self._filter_combo.currentText()
        known_factions = {
            str(self._filter_combo.itemText(i))
            for i in range(self._filter_combo.count())
            if str(self._filter_combo.itemText(i)) != "Tutti"
        }
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
                "resources": dict(actor.get("resources", {})),
                "equipment": dict(actor.get("equipment", {})),
                "area_id": str(actor.get("area_id", "")),
            }

            # Faction root item — create once
            if faction_id not in self._faction_items:
                fitem = QTreeWidgetItem(self._tree)
                fitem.setText(_COL_NAME, faction_id)
                self._faction_items[faction_id] = fitem
                if faction_id not in known_factions:
                    self._filter_combo.addItem(faction_id)
                    known_factions.add(faction_id)

            # Actor leaf item
            item: QTreeWidgetItem | None = self._actor_items.get(actor_id)
            if item is None:
                item = QTreeWidgetItem(self._faction_items[faction_id])
                item.setData(_COL_NAME, Qt.ItemDataRole.UserRole, actor_id)
                self._actor_items[actor_id] = item
            elif item.parent() is not self._faction_items[faction_id]:
                previous_parent = item.parent()
                if previous_parent is not None:
                    previous_parent.removeChild(item)
                self._faction_items[faction_id].addChild(item)

            item.setText(_COL_NAME, name)
            item.setText(_COL_HP, f"{current_hp}/{max_hp}")
            self._update_state_column(actor_id)
            self._apply_life_state_color(item, life_state)

        self._refresh_faction_labels()
        if current_filter and self._filter_combo.findText(current_filter) >= 0:
            self._filter_combo.setCurrentText(current_filter)
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

    def _handle_resource_changed(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        resource_id: str = str(data.get("resource_id", ""))
        new_value: int = int(data.get("new_value", 0))
        self._actor_data[actor_id].setdefault("resources", {})[resource_id] = new_value
        if self._selected_actor_id() == actor_id:
            self._refresh_resource_list(actor_id)

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

    def _handle_actor_removed(self, data: dict) -> None:
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id not in self._actor_data:
            return
        # Remove from tree
        item: QTreeWidgetItem | None = self._actor_items.pop(actor_id, None)
        if item is not None:
            parent = item.parent()
            if parent is not None:
                parent.removeChild(item)
        self._actor_data.pop(actor_id, None)
        self._refresh_faction_labels()
        # If this actor was selected, clear the detail panel
        if self._selected_actor_id() is None:
            self._detail_name.setText("Seleziona un attore")
            self._detail_faction.setText("—")
            self._apply_detail_state("ALIVE")
            self._hp_bar.set_hp(0, 1)
            self._status_list.clear()
            self._resource_list.clear()
            self._equip_list.clear()

    # ── UI helpers ────────────────────────────────────────────────────────────

    def _on_filter_changed(self, faction: str) -> None:
        """Applies faction filter combined with text search."""
        _ = faction
        self._apply_filters()

    def _on_search_changed(self, _: str) -> None:
        """Applies text search combined with faction filter."""
        self._apply_filters()

    def _toggle_actor_tree(self) -> None:
        """Shows/hides the whole actor-list pane (filters + tree, hidden by default)."""
        self._tree_visible = not self._tree_visible
        self._apply_tree_visibility(self._tree_visible)

    def _apply_tree_visibility(self, visible: bool) -> None:
        """Applies the actor-list pane visibility and collapses its width to 0."""
        self._left_panel.setVisible(visible)
        if visible:
            self._splitter.setSizes([260, 320])
            self._splitter.setStretchFactor(0, 2)
        else:
            self._splitter.setSizes([0, 1])
            self._splitter.setStretchFactor(0, 0)
        self._toggle_tree_btn.setText(
            _TOGGLE_EXPANDED_ICON if visible else _TOGGLE_COLLAPSED_ICON
        )

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns the actor-tree visibility for QSettings persistence."""
        if self._widget is None:
            return {}
        return {"tree_visible": self._tree_visible}

    def restore_state(self, state: dict) -> None:
        """Restores the actor-tree visibility from a previously saved state dict."""
        if self._widget is None:
            return
        visible = bool(state.get("tree_visible", True))
        self._tree_visible = visible
        self._apply_tree_visibility(visible)

    def _toggle_status_section(self) -> None:
        """Collapses/expands the Status section content."""
        self._status_expanded = not self._status_expanded
        self._status_list.setVisible(self._status_expanded)
        self._btn_toggle_status.setText(
            _TOGGLE_EXPANDED_ICON if self._status_expanded else _TOGGLE_COLLAPSED_ICON
        )

    def _toggle_resource_section(self) -> None:
        """Collapses/expands the Risorse section content."""
        self._resource_expanded = not self._resource_expanded
        self._resource_list.setVisible(self._resource_expanded)
        self._btn_toggle_resource.setText(
            _TOGGLE_EXPANDED_ICON if self._resource_expanded else _TOGGLE_COLLAPSED_ICON
        )

    def _toggle_equip_section(self) -> None:
        """Collapses/expands the Equipaggiamento section content."""
        self._equip_expanded = not self._equip_expanded
        self._equip_list.setVisible(self._equip_expanded)
        self._btn_toggle_equip.setText(
            _TOGGLE_EXPANDED_ICON if self._equip_expanded else _TOGGLE_COLLAPSED_ICON
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
            if self.on_actor_selected is not None:
                self.on_actor_selected(actor_id)
        else:
            self._detail_name.setText("Seleziona un attore")
            self._detail_faction.setText("—")
            self._apply_detail_state("ALIVE")
            self._hp_bar.set_hp(0, 1)

            self._status_list.clear()
            self._equip_list.clear()

    def _selected_actor_id(self) -> str | None:
        """Returns the actor_id of the currently selected tree item, or None."""
        items: list[QTreeWidgetItem] = self._tree.selectedItems()
        if not items:
            return None
        return items[0].data(_COL_NAME, Qt.ItemDataRole.UserRole)

    def select_actor(self, actor_id: str) -> None:
        """Programmatically selects the tree row for *actor_id* (no-op if absent)."""
        item = self._actor_items.get(actor_id)
        if item is not None:
            self._tree.setCurrentItem(item)

    def _refresh_detail(self, actor_id: str) -> None:
        d: dict = self._actor_data[actor_id]
        self._detail_name.setText(d["name"])
        self._detail_faction.setText(d["faction_id"])
        state = str(d["life_state"])
        self._apply_detail_state(state)
        self._hp_bar.set_hp(d["current_hp"], d["max_hp"])
        self._refresh_status_list(actor_id)
        self._refresh_resource_list(actor_id)
        self._refresh_equip_list(actor_id)

    def _apply_detail_state(self, state: str) -> None:
        """Sets a semantic life-state property for theme-driven styling."""
        normalized = state if state in ("ALIVE", "DYING", "DEAD") else "ALIVE"
        self._detail_state.setText(normalized)
        self._detail_state.setProperty("life_state", normalized.lower())
        self._detail_state.style().unpolish(self._detail_state)
        self._detail_state.style().polish(self._detail_state)

    def _refresh_status_list(self, actor_id: str) -> None:
        self._status_list.clear()
        for status_id, stacks in self._actor_data[actor_id]["statuses"].items():
            self._status_list.addItem(f"{status_id} x{stacks}")

    def _refresh_resource_list(self, actor_id: str) -> None:
        self._resource_list.clear()
        resources: dict = self._actor_data[actor_id].get("resources", {})
        if not resources:
            self._resource_list.addItem("(nessuna risorsa)")
            return
        for res_id, value in sorted(resources.items()):
            self._resource_list.addItem(f"{res_id}: {value}")

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
        """Applies semantic life-state row color from theme tokens."""
        color = None
        if life_state == "DYING":
            color = resolve_semantic_color("state_error")
        elif life_state == "DEAD":
            color = resolve_semantic_color("state_disabled")
        role = int(Qt.ItemDataRole.UserRole) + 1
        for col in range(3):
            item.setForeground(col, QBrush(color) if color is not None else QBrush())
            item.setData(col, role, life_state)
