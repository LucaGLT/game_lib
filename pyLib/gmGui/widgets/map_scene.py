"""MapScene — QGraphicsScene for gmMap location-graph visualisation.

Renders gmMap LocationId nodes as ellipses and adjacency edges as lines.
Actor markers reposition in response to movement events.
Each node supports five visual filter modes: terrain, items, actors, zone, region.
"""
from __future__ import annotations

import math

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QColor, QFont, QPen
from PySide6.QtWidgets import (
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
)

from ..theme_manager import resolve_semantic_color

_NODE_DIAMETER: int = 32
_NODE_RADIUS: float = _NODE_DIAMETER / 2.0
_ACTOR_DIAMETER: int = 16
_SAT_DIAMETER: int = 14
_SAT_RADIUS: float = _SAT_DIAMETER / 2.0
_SAT_ORBIT: float = _NODE_RADIUS + _SAT_RADIUS + 2.0

# ── Terrain token map ─────────────────────────────────────────────────────────
_TERRAIN_TOKEN_MAP: dict[str, str] = {
    "stone":   "map_terrain_stone",
    "grass":   "map_terrain_grass",
    "wooden":  "map_terrain_wooden",
    "marble":  "map_terrain_marble",
    "carpet":  "map_terrain_carpet",
    "woods":   "map_terrain_woods",
}

# ── Zone / Region palette token names (10 slots) ──────────────────────────────
_ZONE_PALETTE_TOKENS: list[str] = [f"map_zone_{i}" for i in range(10)]


def _terrain_color_from_tags(tags: list[str]) -> QColor:
    """Returns the terrain fill colour from a tag list, or the default panel colour."""
    for tag in tags:
        if tag.startswith("terrain:"):
            terrain_type = tag[8:]
            token = _TERRAIN_TOKEN_MAP.get(terrain_type)
            if token:
                return resolve_semantic_color(token)
    return resolve_semantic_color("panel")


def _actor_labels(actor_ids: list[str]) -> list[str]:
    """Compact labels: single initial when unique, initial+N when repeated.

    Example: ["hero_1", "hero_2", "boss"] → ["H1", "H2", "B"]
    """
    initials = [a[0].upper() if a else "?" for a in actor_ids]
    counts: dict[str, int] = {}
    for init in initials:
        counts[init] = counts.get(init, 0) + 1
    seq: dict[str, int] = {}
    labels: list[str] = []
    for init in initials:
        if counts[init] == 1:
            labels.append(init)
        else:
            seq[init] = seq.get(init, 0) + 1
            labels.append(f"{init}{seq[init]}")
    return labels


def _satellite_positions(
    n: int, cx: float, cy: float, orbit_r: float
) -> list[tuple[float, float]]:
    """Top-left corners of *n* satellite circles evenly placed on *orbit_r*.

    Starts from top (angle = -π/2), clockwise.  Returns (x, y) for
    ``QGraphicsEllipseItem(x, y, d, d)``.
    """
    positions: list[tuple[float, float]] = []
    for i in range(n):
        angle = 2.0 * math.pi * i / n - math.pi / 2.0
        sx = cx + orbit_r * math.cos(angle) - _SAT_RADIUS
        sy = cy + orbit_r * math.sin(angle) - _SAT_RADIUS
        positions.append((sx, sy))
    return positions


def _build_palette(names: set[str], palette_tokens: list[str]) -> dict[str, QColor]:
    """Assigns a deterministic colour from *palette_tokens* to each name."""
    return {
        name: resolve_semantic_color(palette_tokens[abs(hash(name)) % len(palette_tokens)])
        for name in names
    }


def _circle_positions(n: int, radius: float = 120.0) -> list[tuple[float, float]]:
    """Distributes *n* positions evenly on a circle of the given radius."""
    if n == 0:
        return []
    positions: list[tuple[float, float]] = []
    for i in range(n):
        angle = 2.0 * math.pi * i / n
        positions.append((radius + radius * math.cos(angle),
                          radius + radius * math.sin(angle)))
    return positions


# ── LocationNode ──────────────────────────────────────────────────────────────

class LocationNode(QGraphicsEllipseItem):
    """Circular node representing a single gmMap location.

    Supports five visual filter modes applied via :meth:`apply_filter`.
    Border is thin (1 px) when unselected, thick (3 px) when selected.
    """

    def __init__(
        self,
        loc_id: int,
        cx: float,
        cy: float,
        tags: list[str] | None = None,
        item_ids: list[str] | None = None,
        parent: QGraphicsItem | None = None,
    ) -> None:
        d = float(_NODE_DIAMETER)
        super().__init__(cx - d / 2.0, cy - d / 2.0, d, d, parent)
        self.loc_id: int = loc_id
        self._cx: float = cx
        self._cy: float = cy
        self._tags: list[str] = list(tags or [])
        self._item_ids: list[str] = list(item_ids or [])
        self._actor_ids: list[str] = []
        self._satellites: list[QGraphicsEllipseItem] = []
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsSelectable, True)
        # Default fill; overridden by apply_filter once inserted into scene.
        self.setBrush(QBrush(_terrain_color_from_tags(self._tags)))
        self.setPen(QPen(resolve_semantic_color("border"), 1))
        # Centred location-id label.
        lbl = QGraphicsSimpleTextItem(str(loc_id), self)
        br = lbl.boundingRect()
        lbl.setPos(
            cx - d / 2.0 + (d - br.width()) / 2.0,
            cy - d / 2.0 + (d - br.height()) / 2.0,
        )

    # ── Selection border ──────────────────────────────────────────────────────

    def itemChange(
        self, change: QGraphicsItem.GraphicsItemChange, value: object
    ) -> object:
        if change == QGraphicsItem.GraphicsItemChange.ItemSelectedHasChanged:
            self._sync_border()
        return super().itemChange(change, value)

    def _sync_border(self) -> None:
        """Thick border when selected, thin otherwise."""
        pen = QPen(self.pen())
        pen.setWidth(3 if self.isSelected() else 1)
        self.setPen(pen)

    # ── Filter application ────────────────────────────────────────────────────

    def apply_filter(
        self,
        filter_name: str,
        zone_color: QColor | None = None,
        region_color: QColor | None = None,
    ) -> None:
        """Recolours this node and rebuilds satellite badges for *filter_name*.

        Args:
            filter_name:  ``"terrain"``, ``"items"``, ``"actors"``,
                          ``"zone"``, or ``"region"``.
            zone_color:   Fill colour when *filter_name* is ``"zone"``.
            region_color: Fill colour when *filter_name* is ``"region"``.
        """
        self._clear_satellites()
        scene = self.scene()

        if filter_name == "terrain":
            fill: QColor = _terrain_color_from_tags(self._tags)
        elif filter_name == "items":
            fill = (resolve_semantic_color("map_items_has") if self._item_ids
                    else resolve_semantic_color("map_items_empty"))
            if scene and self._item_ids:
                self._build_item_satellites(scene)
        elif filter_name == "actors":
            fill = (resolve_semantic_color("map_actors_has") if self._actor_ids
                    else resolve_semantic_color("map_actors_empty"))
            if scene and self._actor_ids:
                self._build_actor_satellites(scene)
        elif filter_name == "zone":
            fill = zone_color if zone_color is not None else resolve_semantic_color("panel")
        elif filter_name == "region":
            fill = region_color if region_color is not None else resolve_semantic_color("panel")
        else:
            fill = resolve_semantic_color("panel")

        self.setBrush(QBrush(fill))
        self._sync_border()

    def set_items(self, item_ids: list[str]) -> None:
        """Updates cached item list (call apply_filter to refresh visuals)."""
        self._item_ids = list(item_ids)

    def set_actors(self, actor_ids: list[str]) -> None:
        """Updates cached actor list (call apply_filter to refresh visuals)."""
        self._actor_ids = list(actor_ids)

    def center(self) -> tuple[float, float]:
        """Returns the (x, y) centre of this node in scene coordinates."""
        return (self._cx, self._cy)

    # ── Satellite helpers ─────────────────────────────────────────────────────

    def _clear_satellites(self) -> None:
        scene = self.scene()
        for sat in self._satellites:
            if scene is not None:
                scene.removeItem(sat)
        self._satellites.clear()

    def _build_item_satellites(self, scene: QGraphicsScene) -> None:
        positions = _satellite_positions(
            len(self._item_ids), self._cx, self._cy, _SAT_ORBIT)
        for i, (sx, sy) in enumerate(positions):
            label = chr(ord("A") + i) if i < 26 else str(i - 25)
            self._add_satellite(
                scene, sx, sy, label,
                resolve_semantic_color("map_sat_item_bg"),
                resolve_semantic_color("map_sat_item_fg"),
            )

    def _build_actor_satellites(self, scene: QGraphicsScene) -> None:
        labels = _actor_labels(self._actor_ids)
        positions = _satellite_positions(
            len(labels), self._cx, self._cy, _SAT_ORBIT)
        for (sx, sy), label in zip(positions, labels):
            self._add_satellite(
                scene, sx, sy, label,
                resolve_semantic_color("map_sat_actor_bg"),
                resolve_semantic_color("map_sat_actor_fg"),
            )

    def _add_satellite(
        self,
        scene: QGraphicsScene,
        sx: float,
        sy: float,
        label: str,
        bg: QColor,
        fg: QColor,
    ) -> None:
        d = float(_SAT_DIAMETER)
        sat = QGraphicsEllipseItem(sx, sy, d, d)
        sat.setBrush(QBrush(bg))
        sat.setPen(QPen(fg, 1))
        sat.setZValue(3.0)
        scene.addItem(sat)
        font = QFont()
        font.setPointSize(6)
        font.setBold(True)
        txt = QGraphicsSimpleTextItem(label, sat)
        txt.setBrush(QBrush(fg))
        txt.setFont(font)
        br = txt.boundingRect()
        txt.setPos(sx + (d - br.width()) / 2.0, sy + (d - br.height()) / 2.0)
        self._satellites.append(sat)


# ── ActorMarker ───────────────────────────────────────────────────────────────

class ActorMarker(QGraphicsEllipseItem):
    """Small coloured circle with actor initial, placed above a LocationNode.

    Faction colours:
    - ``heroes``  → blue
    - ``enemies`` → red
    - ``neutral`` → purple
    - (other)     → slate
    """

    _FACTION_TOKENS: dict[str, str] = {
        "heroes": "accent",
        "enemies": "state_error",
        "neutral": "state_warning",
    }
    _DEFAULT_TOKEN: str = "border"

    def __init__(
        self,
        actor_id: str,
        cx: float,
        cy: float,
        faction: str = "",
        parent: QGraphicsItem | None = None,
    ) -> None:
        d = float(_ACTOR_DIAMETER)
        # Place marker slightly above-right of node centre.
        mx = cx + float(_NODE_DIAMETER) / 4.0 - d / 2.0
        my = cy - float(_NODE_DIAMETER) / 2.0 - d / 2.0
        super().__init__(mx, my, d, d, parent)
        color_token = self._FACTION_TOKENS.get(faction, self._DEFAULT_TOKEN)
        color = resolve_semantic_color(color_token)
        self.setBrush(QBrush(color))
        self.setPen(QPen(resolve_semantic_color("text"), 1))
        self.setZValue(2.0)

        initial = actor_id[0].upper() if actor_id else "?"
        lbl = QGraphicsSimpleTextItem(initial, self)
        lbl.setBrush(QBrush(resolve_semantic_color("text")))
        br = lbl.boundingRect()
        lbl.setPos(mx + (d - br.width()) / 2.0, my + (d - br.height()) / 2.0)


# ── MapScene ──────────────────────────────────────────────────────────────────

class MapScene(QGraphicsScene):
    """Location graph scene with filter-driven node rendering.

    Methods
    -------
    load_map(locations, edges)
        Rebuilds the scene from a snapshot and applies the current filter.
    set_filter(filter_name)
        Re-applies a named filter to every node.
    move_actor(actor_id, new_location_id)
        Repositions an actor marker and refreshes actors-filter satellites.
    update_location(loc_id, metadata)
        Updates a node's tags/items and refreshes its filter rendering.
    node_count() / edge_count()
        Query helpers used by tests.
    marker_location(actor_id)
        Returns the current location_id of an actor marker.
    """

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self._nodes: dict[int, LocationNode] = {}
        self._edges: list[QGraphicsLineItem] = []
        self._markers: dict[str, ActorMarker] = {}
        self._marker_locations: dict[str, int] = {}
        self._current_filter: str = "terrain"
        self._location_actors: dict[int, list[str]] = {}
        self._location_items: dict[int, list[str]] = {}
        self._location_zone: dict[int, str] = {}
        self._location_region: dict[int, str] = {}
        self._zone_palette: dict[str, QColor] = {}
        self._region_palette: dict[str, QColor] = {}

    # ── Public API ────────────────────────────────────────────────────────────

    def set_filter(self, filter_name: str) -> None:
        """Applies *filter_name* to every location node.

        Args:
            filter_name: One of ``"terrain"``, ``"items"``, ``"actors"``,
                         ``"zone"``, ``"region"``.
        """
        self._current_filter = filter_name
        for loc_id, node in self._nodes.items():
            zone_c   = self._zone_palette.get(self._location_zone.get(loc_id, ""))
            region_c = self._region_palette.get(self._location_region.get(loc_id, ""))
            node.apply_filter(filter_name, zone_c, region_c)

    def load_map(
        self,
        locations: list[dict],
        edges: list[tuple[int, int]],
    ) -> None:
        """Clears the scene and rebuilds it from a snapshot.

        Accepts both the new-style ``tags`` list and the legacy
        ``metadata.terrain`` / ``metadata.items`` fields.

        Args:
            locations: List of dicts with ``"location_id"`` (int), optional
                       ``"x"``/``"y"`` (float), ``"metadata"`` (dict),
                       ``"tags"`` (list[str]), ``"zone_id"`` (str),
                       ``"region_id"`` (str).
            edges:     List of ``(loc_id_a, loc_id_b)`` pairs.
        """
        self.clear()
        self._nodes = {}
        self._edges = []
        self._markers = {}
        self._marker_locations = {}
        self._location_actors = {}
        self._location_items = {}
        self._location_zone = {}
        self._location_region = {}
        self._zone_palette = {}
        self._region_palette = {}

        self.setBackgroundBrush(QBrush(resolve_semantic_color("panel")))

        positions = _circle_positions(len(locations))
        for i, loc in enumerate(locations):
            loc_id = int(loc.get("location_id", i))
            cx = float(loc.get("x", positions[i][0]))
            cy = float(loc.get("y", positions[i][1]))
            meta: dict = loc.get("metadata", {})
            if not isinstance(meta, dict):
                meta = {}
            # Merge explicit tags list with legacy metadata.terrain.
            tags: list[str] = list(loc.get("tags", []))
            terrain = str(meta.get("terrain", ""))
            if terrain and not any(t.startswith("terrain:") for t in tags):
                tags.append(f"terrain:{terrain}")
            items: list[str] = [str(x) for x in meta.get("items", [])]
            self._location_items[loc_id] = items
            zone = str(loc.get("zone_id") or meta.get("zone_id") or "")
            region = str(loc.get("region_id") or meta.get("region_id") or "")
            if zone:
                self._location_zone[loc_id] = zone
            if region:
                self._location_region[loc_id] = region
            node = LocationNode(loc_id, cx, cy, tags, items)
            self.addItem(node)
            self._nodes[loc_id] = node

        # Build deterministic colour palettes.
        self._zone_palette   = _build_palette(set(self._location_zone.values()),   _ZONE_PALETTE_TOKENS)
        self._region_palette = _build_palette(set(self._location_region.values()), _ZONE_PALETTE_TOKENS)

        for pair in edges:
            a, b = int(pair[0]), int(pair[1])
            if a in self._nodes and b in self._nodes:
                cx_a, cy_a = self._nodes[a].center()
                cx_b, cy_b = self._nodes[b].center()
                line = self.addLine(
                    cx_a, cy_a, cx_b, cy_b,
                    QPen(resolve_semantic_color("border"), 1),
                )
                line.setZValue(-1.0)
                self._edges.append(line)

        # Apply the active filter now that all nodes are in the scene.
        self.set_filter(self._current_filter)

    def move_actor(self, actor_id: str, new_location_id: int) -> None:
        """Places (or moves) an actor marker at the given location.

        Also updates per-location actor lists used by the actors filter.

        Args:
            actor_id:        Actor identifier.
            new_location_id: Destination location ID (must exist in scene).
        """
        if new_location_id not in self._nodes:
            return
        old_loc = self._marker_locations.get(actor_id)
        # Update per-location actor tracking.
        if old_loc is not None:
            old_list = self._location_actors.get(old_loc, [])
            self._location_actors[old_loc] = [a for a in old_list if a != actor_id]
            if old_loc in self._nodes:
                self._nodes[old_loc].set_actors(self._location_actors[old_loc])
        self._location_actors.setdefault(new_location_id, [])
        if actor_id not in self._location_actors[new_location_id]:
            self._location_actors[new_location_id].append(actor_id)
        self._nodes[new_location_id].set_actors(self._location_actors[new_location_id])
        # Reposition the ActorMarker.
        if actor_id in self._markers:
            self.removeItem(self._markers[actor_id])
            del self._markers[actor_id]
        cx, cy = self._nodes[new_location_id].center()
        marker = ActorMarker(actor_id, cx, cy)
        self.addItem(marker)
        self._markers[actor_id] = marker
        self._marker_locations[actor_id] = new_location_id
        # Refresh actors-filter satellites on affected nodes.
        if self._current_filter == "actors":
            if old_loc is not None and old_loc in self._nodes:
                self._nodes[old_loc].apply_filter("actors")
            self._nodes[new_location_id].apply_filter("actors")

    def update_location(self, loc_id: int, metadata: dict) -> None:
        """Applies new metadata to an existing node and refreshes its rendering.

        Args:
            loc_id:   Target location ID.
            metadata: Dict with optional keys ``"terrain"`` (str) and
                      ``"items"`` (list).
        """
        if loc_id not in self._nodes:
            return
        node = self._nodes[loc_id]
        if "items" in metadata:
            items = [str(x) for x in metadata["items"]]
            self._location_items[loc_id] = items
            node.set_items(items)
        if "terrain" in metadata:
            terrain = str(metadata["terrain"])
            tags = [t for t in node._tags if not t.startswith("terrain:")]
            if terrain:
                tags.append(f"terrain:{terrain}")
            node._tags = tags
        zone_c   = self._zone_palette.get(self._location_zone.get(loc_id, ""))
        region_c = self._region_palette.get(self._location_region.get(loc_id, ""))
        node.apply_filter(self._current_filter, zone_c, region_c)

    def node_count(self) -> int:
        """Returns the number of location nodes in the scene."""
        return len(self._nodes)

    def edge_count(self) -> int:
        """Returns the number of adjacency edges in the scene."""
        return len(self._edges)

    def marker_location(self, actor_id: str) -> int | None:
        """Returns the location_id where the actor marker currently sits, or None."""
        return self._marker_locations.get(actor_id)
