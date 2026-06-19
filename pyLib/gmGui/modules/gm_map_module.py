"""GmMapModule — Location graph and actor-position visualiser.

Subscribes to gmMap structural events and gmActor movement events,
and renders them via :class:`~gmGui.widgets.map_scene.MapScene`.
"""
from __future__ import annotations

import json as _json

from PySide6.QtCore import Qt
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
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        self._map_scene: MapScene = MapScene()
        self._zoom_level: float = 1.0

        # Tolerant zone/region indices populated from ``gmMap.map.loaded``.
        # Empty when the payload predates the Region/Zone schema (Fase A-D).
        self._location_zone: dict[int, str] = {}
        self._location_region: dict[int, str] = {}

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
        self._layer_combo: QComboBox = QComboBox()
        for layer in ("terrain", "items", "actors", "zone", "region"):
            self._layer_combo.addItem(layer)
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

        return container

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns zoom level and active layer for QSettings persistence."""
        if self._widget is None:
            return {}
        return {
            "zoom_level": self._zoom_level,
            "layer": self._layer_combo.currentText(),
        }

    def restore_state(self, state: dict) -> None:
        """Restores zoom level and active layer from a previously saved state dict."""
        if self._widget is None:
            return
        zoom = state.get("zoom_level")
        if isinstance(zoom, (int, float)) and zoom > 0:
            self._map_view.resetTransform()
            self._zoom_level = 1.0
            self._zoom(float(zoom))
        layer = state.get("layer", "")
        idx = self._layer_combo.findText(layer)
        if idx >= 0:
            self._layer_combo.setCurrentIndex(idx)

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

    # ── Zone / Region indexing ────────────────────────────────────────────────

    def _index_zone_region(self, locations: list) -> None:
        """Populates the zone/region indices from a ``gmMap.map.loaded`` payload.

        Parsing is **tolerant and additive**: each location entry is inspected
        for optional ``zone_id``/``region_id`` keys. Entries that predate the
        Region/Zone schema (Fase A-D) simply leave the indices empty. The scene
        is intentionally **not** recoloured here — the indices are exposed for
        future overlays and for the ``zone``/``region`` layer selectors.
        """
        self._location_zone.clear()
        self._location_region.clear()
        for entry in locations:
            if not isinstance(entry, dict):
                continue
            raw_id = entry.get("id", entry.get("location_id"))
            if raw_id is None:
                continue
            try:
                loc_id = int(raw_id)
            except (TypeError, ValueError):
                continue
            zone = entry.get("zone_id")
            if zone is not None:
                self._location_zone[loc_id] = str(zone)
            region = entry.get("region_id")
            if region is not None:
                self._location_region[loc_id] = str(region)

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
            self._index_zone_region(locations)
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

