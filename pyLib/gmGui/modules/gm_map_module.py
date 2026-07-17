"""GmMapModule — Location graph and actor-position visualiser.

Subscribes to gmMap structural events and gmActor movement events,
and renders them via :class:`~gmGui.widgets.map_scene.MapScene`.
"""
from __future__ import annotations

import json as _json

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QStandardItem, QStandardItemModel
from PySide6.QtWidgets import (
    QComboBox,
    QGraphicsView,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..message_ids import AREA_INFO_REQUEST, AREA_SELECTED
from ..widgets.map_scene import LocationNode, MapScene
from .base_module import BaseModule

_MIN_ZOOM: float = 0.25
_MAX_ZOOM: float = 4.0

_LAYER_NAMES: list[str] = ["terrain", "items", "actors", "zone", "region"]
_DEFAULT_LAYERS: set[str] = {"zone", "actors"}


class CheckableComboBox(QComboBox):
    """QComboBox where each item carries an independent checkbox.

    Signals
    -------
    layers_changed(set)
        Emitted whenever any item's checked state changes.
    """

    layers_changed: Signal = Signal(object)  # set[str]

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self.setEditable(True)
        self.lineEdit().setReadOnly(True)
        self.setModel(QStandardItemModel(self))
        self.view().pressed.connect(self._toggle_pressed)
        self._suppress_hide: bool = False

    def add_layer(self, text: str, *, checked: bool = False) -> None:
        """Adds *text* as a checkable item."""
        item = QStandardItem(text)
        item.setCheckable(True)
        item.setCheckState(
            Qt.CheckState.Checked if checked else Qt.CheckState.Unchecked)
        self.model().appendRow(item)
        self._refresh_display()

    def checked_layers(self) -> set:
        """Returns the set of currently checked layer names."""
        result: set = set()
        for i in range(self.model().rowCount()):
            it = self.model().item(i)
            if it and it.checkState() == Qt.CheckState.Checked:
                result.add(it.text())
        return result

    def set_checked(self, layers: set) -> None:
        """Programmatically sets which layers are checked."""
        for i in range(self.model().rowCount()):
            it = self.model().item(i)
            if it:
                state = (Qt.CheckState.Checked if it.text() in layers
                         else Qt.CheckState.Unchecked)
                it.setCheckState(state)
        self._refresh_display()

    # ── Internal ──────────────────────────────────────────────────────────────

    def _toggle_pressed(self, index) -> None:
        it = self.model().itemFromIndex(index)
        if it is None:
            return
        new_state = (
            Qt.CheckState.Unchecked
            if it.checkState() == Qt.CheckState.Checked
            else Qt.CheckState.Checked
        )
        it.setCheckState(new_state)
        self._suppress_hide = True
        self._refresh_display()
        self.layers_changed.emit(self.checked_layers())

    def hidePopup(self) -> None:
        if self._suppress_hide:
            self._suppress_hide = False
            return
        super().hidePopup()

    def _refresh_display(self) -> None:
        checked = sorted(self.checked_layers())
        self.lineEdit().setText(", ".join(checked) if checked else "")


class GmMapModule(BaseModule):
    """Visualises gmMap locations, adjacency edges, and actor positions.

    Layout:
    - Top bar: [Zoom -] [Zoom +] [Fit] + QComboBox layer selector
    - Centre:  QGraphicsView over MapScene (scroll + wheel zoom)
    - Bottom:  Info label for selected location

    TypeIds: ``gmMap.map.loaded``, ``gmMap.location.item_added``,
    ``gmMap.location.item_removed``, ``gmMap.location.metadata_changed``,
    ``gmActor.actor.moved_area``, ``gmActor.actor.position_changed``.
    """

    @property
    def module_id(self) -> str:
        return "gm_map"

    @property
    def title(self) -> str:
        return "Map"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.LeftDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmMap.map.loaded",
            "gmMap.location.item_added",
            "gmMap.location.item_removed",
            "gmMap.location.metadata_changed",
            "gmActor.actor.moved_area",
            "gmActor.actor.position_changed",
            "gmActor.actor.removed",
            "gmActor.actor.turn_active",
            "gmActor.actor.selected",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        self._map_scene: MapScene = MapScene()
        self._zoom_level: float = 1.0

        container = QWidget()
        container.setObjectName("gm_map_module")
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(4, 4, 4, 4)
        vbox.setSpacing(4)

        # ── Top toolbar ───────────────────────────────────────────────────────
        top_bar = QHBoxLayout()
        top_bar.setSpacing(4)
        self._zoom_out_btn: QPushButton = QPushButton("Zoom -")
        self._zoom_in_btn: QPushButton = QPushButton("Zoom +")
        self._fit_btn: QPushButton = QPushButton("Fit")
        self._layer_combo: CheckableComboBox = CheckableComboBox()
        for layer in _LAYER_NAMES:
            self._layer_combo.add_layer(layer, checked=(layer in _DEFAULT_LAYERS))
        top_bar.addWidget(self._zoom_out_btn)
        top_bar.addWidget(self._zoom_in_btn)
        top_bar.addWidget(self._fit_btn)
        top_bar.addStretch()
        top_bar.addWidget(QLabel("Layer:"))
        top_bar.addWidget(self._layer_combo)
        vbox.addLayout(top_bar)

        # ── Map view ──────────────────────────────────────────────────────────
        self._map_view: QGraphicsView = QGraphicsView(self._map_scene)
        self._map_view.setDragMode(QGraphicsView.DragMode.ScrollHandDrag)
        vbox.addWidget(self._map_view, stretch=1)

        # ── Bottom info bar ───────────────────────────────────────────────────
        self._info_label: QLabel = QLabel("—")
        vbox.addWidget(self._info_label)

        # ── Connections ───────────────────────────────────────────────────────
        self._zoom_in_btn.clicked.connect(lambda: self._zoom(1.25))
        self._zoom_out_btn.clicked.connect(lambda: self._zoom(0.8))
        self._fit_btn.clicked.connect(self._fit_view)
        self._map_scene.selectionChanged.connect(self._on_selection_changed)
        self._layer_combo.layers_changed.connect(self._map_scene.set_active_layers)

        return container

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns zoom level and active layers for QSettings persistence."""
        if self._widget is None:
            return {}
        return {
            "zoom_level": self._zoom_level,
            "layers": sorted(self._layer_combo.checked_layers()),
        }

    def restore_state(self, state: dict) -> None:
        """Restores zoom level and active layers from a previously saved state dict."""
        if self._widget is None:
            return
        zoom = state.get("zoom_level")
        if isinstance(zoom, (int, float)) and zoom > 0:
            self._map_view.resetTransform()
            self._zoom_level = 1.0
            self._zoom(float(zoom))
        # Accept both new-style "layers" list and old-style "layer" string.
        layers = state.get("layers")
        if not layers and "layer" in state:
            layers = [state["layer"]]
        if layers:
            self._layer_combo.set_checked(set(layers))
            self._map_scene.set_active_layers(self._layer_combo.checked_layers())

    # ── Theme ─────────────────────────────────────────────────────────────────

    def refresh_theme(self) -> None:
        """Re-applies the active application theme's colours to the map scene.

        Call this after ``ThemeManager.apply_theme()`` since the scene has no
        automatic notification mechanism for theme changes.
        """
        self._map_scene.refresh_theme()

    # ── Zoom helpers ──────────────────────────────────────────────────────────

    def _zoom(self, factor: float) -> None:
        """Scales the view by *factor*, clamping zoom level to [0.25, 4.0]."""
        new_level = self._zoom_level * factor
        if new_level < _MIN_ZOOM:
            factor = _MIN_ZOOM / self._zoom_level
            new_level = _MIN_ZOOM
        elif new_level > _MAX_ZOOM:
            factor = _MAX_ZOOM / self._zoom_level
            new_level = _MAX_ZOOM
        self._map_view.scale(factor, factor)
        self._zoom_level = new_level

    def _fit_view(self) -> None:
        """Fits the entire scene into the visible area, preserving aspect ratio."""
        self._map_view.fitInView(
            self._map_scene.itemsBoundingRect(),
            Qt.AspectRatioMode.KeepAspectRatio,
        )

    def _on_selection_changed(self) -> None:
        items = self._map_scene.selectedItems()
        if not items:
            self._info_label.setText("—")
            return
        node = items[0]
        if isinstance(node, LocationNode):
            self._info_label.setText(f"Location #{node.loc_id}")
            self._request_area_info(str(node.loc_id))

    def _request_area_info(self, area_id: str) -> None:
        """Sends an area-info request to the Core for *area_id*.

        Selecting an area is a **view-only** interaction: it never triggers an
        actor action. The Core replies with a ``gmMap.area.info.response``
        consumed by the Area Info module.
        """
        if not area_id:
            return
        self.send_command(AREA_SELECTED, {"area_id": area_id})
        self.send_command(AREA_INFO_REQUEST, {"area_id": area_id})

    # ── Envelope routing ──────────────────────────────────────────────────────
    def on_envelope(self, msg: dict) -> None:
        tid = msg.get("typeId", "")
        raw = msg.get("headers", {}).get("data", "{}")
        try:
            data: dict = _json.loads(raw) if isinstance(raw, str) else raw
        except Exception:
            data = {}

        if tid == "gmMap.map.loaded":
            locations = list(data.get("locations", []))
            edges = [tuple(e) for e in data.get("edges", [])]
            self._map_scene.load_map(locations, edges)

        elif tid in ("gmMap.location.item_added", "gmMap.location.item_removed"):
            loc_id = int(data.get("location_id", -1))
            items = data.get("items", [])
            self._map_scene.update_location(loc_id, {"items": items})

        elif tid == "gmMap.location.metadata_changed":
            loc_id = int(data.get("location_id", -1))
            metadata = data.get("metadata", {})
            self._map_scene.update_location(loc_id, metadata)

        elif tid == "gmActor.actor.moved_area":
            actor_id = str(data.get("actor_id", ""))
            new_loc = int(data.get("new_area_id", data.get("location_id", -1)))
            if actor_id and new_loc >= 0:
                self._map_scene.move_actor(actor_id, new_loc)

        elif tid == "gmActor.actor.position_changed":
            actor_id = str(data.get("actor_id", ""))
            new_loc = int(data.get("new_location_id", -1))
            if actor_id and new_loc >= 0:
                self._map_scene.move_actor(actor_id, new_loc)

        elif tid == "gmActor.actor.removed":
            actor_id = str(data.get("actor_id", ""))
            if actor_id:
                self._map_scene.remove_actor(actor_id)

        elif tid == "gmActor.actor.turn_active":
            actor_ids = [str(a) for a in data.get("actor_ids", [])]
            self._map_scene.set_active_actors(actor_ids)

        elif tid == "gmActor.actor.selected":
            actor_id = str(data.get("actor_id", ""))
            self._map_scene.set_selected_actor(actor_id)

