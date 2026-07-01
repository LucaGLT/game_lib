"""MapScene — QGraphicsScene for gmMap location-graph visualisation.

Renders gmMap LocationId nodes as ellipses and adjacency edges as lines.
Actor markers reposition in response to movement events.
Each node supports five visual filter modes: terrain, items, actors, zone, region.
"""
from __future__ import annotations

import math
from collections import deque

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QColor, QFont, QPen
from PySide6.QtWidgets import (
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
    QStyle,
    QStyleOptionGraphicsItem,
)

from ..theme_manager import resolve_semantic_color

_NODE_DIAMETER: int = 40
_NODE_RADIUS: float = _NODE_DIAMETER / 2.0
# Item satellites — small red badges
_SAT_D_ITEM: int = 14
_SAT_R_ITEM: float = _SAT_D_ITEM / 2.0
_SAT_ORBIT_ITEM: float = _NODE_RADIUS + _SAT_R_ITEM + 2.0
# Actor satellites — larger blue badges (same diameter as old ActorMarker)
_SAT_D_ACTOR: int = 18
_SAT_R_ACTOR: float = _SAT_D_ACTOR / 2.0
_SAT_ORBIT_ACTOR: float = _NODE_RADIUS + _SAT_R_ACTOR + 2.0

# Angular pools (degrees, 0°=east, clockwise).
# Items: odd multiples of 45° — never a multiple of 90°.
_ITEM_ANGLES_DEG: tuple[float, ...] = (45.0, 135.0, 225.0, 315.0)
# Actors: multiples of 30° that are NOT multiples of 90°, starting near 210°
# (≈ opposite of 45°) so actors and items are placed on opposite sides of the node.
_ACTOR_ANGLES_DEG: tuple[float, ...] = (210.0, 240.0, 300.0, 330.0, 30.0, 60.0, 120.0, 150.0)
# The two pools are disjoint, so items and actors never overlap visually.

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


def _stable_idx(name: str, n: int) -> int:
    """Deterministic palette index: sum of character codes modulo *n*."""
    return sum(ord(c) for c in name) % n if n else 0


def _item_letter(index: int) -> str:
    """Converts a 0-based index to a globally unique item letter (A…Z, AA…)."""
    if index < 26:
        return chr(ord("A") + index)
    return _item_letter((index // 26) - 1) + chr(ord("A") + (index % 26))


def _sat_positions_pooled(
    n: int,
    cx: float,
    cy: float,
    orbit_r: float,
    sat_r: float,
    angle_pool: tuple[float, ...],
) -> list[tuple[float, float]]:
    """Top-left corners of *n* satellite circles using angles from *angle_pool*.

    Angles are taken in order from *angle_pool*, cycling when *n* exceeds the
    pool size.  Each entry in *angle_pool* is in degrees (0°=east, clockwise).
    Returns ``(x, y)`` suitable for ``QGraphicsEllipseItem(x, y, d, d)``.
    """
    m = len(angle_pool)
    positions: list[tuple[float, float]] = []
    for i in range(n):
        angle = math.radians(angle_pool[i % m])
        sx = cx + orbit_r * math.cos(angle) - sat_r
        sy = cy + orbit_r * math.sin(angle) - sat_r
        positions.append((sx, sy))
    return positions


def _satellite_positions(
    n: int, cx: float, cy: float, orbit_r: float, sat_r: float
) -> list[tuple[float, float]]:
    """Top-left corners of *n* satellite circles evenly placed on *orbit_r*.

    Starts from top (angle = -π/2), clockwise.  Returns (x, y) for
    ``QGraphicsEllipseItem(x, y, d, d)``.
    """
    positions: list[tuple[float, float]] = []
    for i in range(n):
        angle = 2.0 * math.pi * i / n - math.pi / 2.0
        sx = cx + orbit_r * math.cos(angle) - sat_r
        sy = cy + orbit_r * math.sin(angle) - sat_r
        positions.append((sx, sy))
    return positions


def _build_palette(names: set[str], palette_tokens: list[str]) -> dict[str, QColor]:
    """Assigns a deterministic colour from *palette_tokens* to each name."""
    n = len(palette_tokens)
    return {
        name: resolve_semantic_color(palette_tokens[_stable_idx(name, n)])
        for name in names
    }


# ── Edge type colours ─────────────────────────────────────────────────────────
#
# Edge types passed as the optional third element of each edge tuple:
#   "FREE"          — open passage within the same zone
#   "CLOSED_DOOR"   — door between zones, traversable by PG (costs 1 extra ⌛)
#   "LOCKED_DOOR"   — door locked by a mechanic; neither side can pass
#
_EDGE_FREE_COLOR: QColor        = QColor("#707070")   # neutral grey
_EDGE_CLOSED_DOOR_COLOR: QColor = QColor("#c89030")   # amber
_EDGE_LOCKED_DOOR_COLOR: QColor = QColor("#c03030")   # red


def _edge_pen(edge_type: str) -> QPen:
    """Returns the QPen appropriate for an edge of the given type string."""
    if edge_type == "CLOSED_DOOR":
        return QPen(_EDGE_CLOSED_DOOR_COLOR, 2)
    if edge_type == "LOCKED_DOOR":
        pen = QPen(_EDGE_LOCKED_DOOR_COLOR, 2)
        pen.setStyle(Qt.PenStyle.DashLine)
        return pen
    # FREE (default)
    return QPen(_EDGE_FREE_COLOR, 1)


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


def _bfs_radial_layout(
    adj: dict[int, list[int]],
    all_ids: list[int],
    step: float = 90.0,
) -> dict[int, tuple[float, float]]:
    """BFS radial tree layout starting from the first node in *all_ids*.

    Places the root at the origin, then spreads children outward at priority
    angles (Qt screen coordinates, y increases downward):

    * Cardinal: E (0°), N (270°), W (180°), S (90°)
    * Diagonal: NE (315°), NW (225°), SW (135°), SE (45°)
    * Fine:     330°, 300°, 240°, 210°, 150°, 120°, 60°, 30°

    Each node avoids placing children in the direction it was approached from
    (±75° around the back-direction).  Nodes not reachable from the root are
    placed in a row below the main layout.

    Args:
        adj:     Adjacency dict ``{node_id: [neighbor_ids]}``.  Undirected.
        all_ids: Ordered list of all node IDs (disconnected nodes included).
        step:    Pixel distance between adjacent nodes.

    Returns:
        ``{node_id: (x, y)}`` with root at ``(0, 0)``.
    """
    if not all_ids:
        return {}

    # Priority angles in Qt screen space (y-down):
    # 0°=east, 270°=north(up), 180°=west, 90°=south(down)
    _PRIORITY: tuple[float, ...] = (
        0.0, 270.0, 180.0, 90.0,
        315.0, 225.0, 135.0, 45.0,
        330.0, 300.0, 240.0, 210.0, 150.0, 120.0, 60.0, 30.0,
    )

    positions: dict[int, tuple[float, float]] = {}
    entry_angle: dict[int, float] = {}

    root = all_ids[0]
    positions[root] = (0.0, 0.0)
    visited: set[int] = {root}
    queue: deque[int] = deque([root])

    while queue:
        node = queue.popleft()
        cx_n, cy_n = positions[node]
        unvisited = [nb for nb in adj.get(node, []) if nb not in visited]
        if not unvisited:
            continue

        if node == root:
            available = list(_PRIORITY)
        else:
            ea = entry_angle[node]
            back = (ea + 180.0) % 360.0
            # Keep angles at least 75° away from the back-direction
            available = [
                a for a in _PRIORITY
                if abs((a - back + 180.0) % 360.0 - 180.0) > 75.0
            ]
            if not available:
                available = list(_PRIORITY)

        for i, neighbor in enumerate(unvisited):
            angle_deg = available[i % len(available)]
            angle_rad = math.radians(angle_deg)
            nx = cx_n + step * math.cos(angle_rad)
            ny = cy_n + step * math.sin(angle_rad)
            positions[neighbor] = (nx, ny)
            entry_angle[neighbor] = angle_deg
            visited.add(neighbor)
            queue.append(neighbor)

    # Disconnected nodes: place in a row below the main graph
    unpositioned = [n for n in all_ids if n not in positions]
    if unpositioned:
        if positions:
            min_x = min(p[0] for p in positions.values())
            max_y = max(p[1] for p in positions.values())
        else:
            min_x, max_y = 0.0, 0.0
        for k, n in enumerate(unpositioned):
            positions[n] = (min_x + k * step, max_y + step * 1.5)

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
        label: str | None = None,
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
        # Default fill; overridden by apply_layers once inserted into scene.
        self._border_color: QColor = resolve_semantic_color("border")
        self.setBrush(QBrush(_terrain_color_from_tags(self._tags)))
        self.setPen(QPen(self._border_color, 1))
        # Centred location label (supplied string label or integer id as fallback).
        _lbl_text = label if label else str(loc_id)
        _lbl_font = QFont()
        _lbl_font.setPointSize(7)
        _lbl_font.setBold(True)
        lbl = QGraphicsSimpleTextItem(_lbl_text, self)
        lbl.setFont(_lbl_font)
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

    def paint(
        self,
        painter: object,
        option: QStyleOptionGraphicsItem,
        widget: object = None,
    ) -> None:
        """Paints the node without the built-in dashed selection rectangle."""
        opt = QStyleOptionGraphicsItem(option)
        opt.state = opt.state & ~QStyle.State(QStyle.StateFlag.State_Selected)
        super().paint(painter, opt, widget)

    def _sync_border(self) -> None:
        """Border colour = region colour; width 3 px when selected, 1 px otherwise."""
        pen = QPen(self._border_color)
        pen.setWidth(3 if self.isSelected() else 1)
        self.setPen(pen)

    # ── Filter application ────────────────────────────────────────────────────

    def apply_layers(
        self,
        active_layers: set,
        zone_color: QColor | None = None,
        region_color: QColor | None = None,
        item_labels: dict | None = None,
        actor_labels: dict | None = None,
    ) -> None:
        """Recolours this node and rebuilds satellite badges for *active_layers*.

        Fill priority (highest wins): region → zone → terrain → panel.
        Items and actors satellite rings are independent and additive.

        Args:
            active_layers: Active layer names (any subset of
                           ``{"terrain","items","actors","zone","region"}``).
            zone_color:    Fill colour when ``"zone"`` is active.
            region_color:  Fill colour when ``"region"`` is active.
            item_labels:   item_id → global letter for item satellite badges.
            actor_labels:  actor_id → game-unique label from the C++ engine.
        """
        self._clear_satellites()
        scene = self.scene()

        # Border colour always tracks the region, regardless of active layers.
        self._border_color = region_color if region_color is not None else resolve_semantic_color("border")

        # Fill: highest-priority active layer wins.
        if "region" in active_layers and region_color is not None:
            fill: QColor = region_color
        elif "zone" in active_layers and zone_color is not None:
            fill = zone_color
        elif "terrain" in active_layers:
            fill = _terrain_color_from_tags(self._tags)
        else:
            fill = resolve_semantic_color("panel")

        # Satellite rings: items (inner) and actors (outer) are additive.
        if scene and "items" in active_layers and self._item_ids:
            self._build_item_satellites(scene, item_labels or {})
        if scene and "actors" in active_layers and self._actor_ids:
            self._build_actor_satellites(scene, actor_labels or {})

        self.setBrush(QBrush(fill))
        self._sync_border()

    def apply_filter(
        self,
        filter_name: str,
        zone_color: QColor | None = None,
        region_color: QColor | None = None,
    ) -> None:
        """Compatibility wrapper: activates a single named layer."""
        self.apply_layers({filter_name}, zone_color, region_color)

    def set_items(self, item_ids: list[str]) -> None:
        """Updates cached item list (call apply_layers to refresh visuals)."""
        self._item_ids = list(item_ids)

    def set_actors(self, actor_ids: list[str]) -> None:
        """Updates cached actor list (call apply_layers to refresh visuals)."""
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

    def _build_item_satellites(self, scene: QGraphicsScene, item_labels: dict) -> None:
        positions = _sat_positions_pooled(
            len(self._item_ids), self._cx, self._cy, _SAT_ORBIT_ITEM, _SAT_R_ITEM,
            _ITEM_ANGLES_DEG)
        for i, (sx, sy) in enumerate(positions):
            iid = self._item_ids[i] if i < len(self._item_ids) else ""
            label = item_labels.get(iid, chr(ord("A") + i) if i < 26 else "?")
            self._add_satellite(
                scene, sx, sy, label, _SAT_D_ITEM,
                resolve_semantic_color("map_sat_item_bg"),
                resolve_semantic_color("map_sat_item_fg"),
            )

    def _build_actor_satellites(self, scene: QGraphicsScene, actor_labels: dict) -> None:
        # Use engine-assigned labels; fall back to first-letter initial if absent.
        labels = [
            actor_labels.get(aid, aid[0].upper() if aid else "?")
            for aid in self._actor_ids
        ]
        positions = _sat_positions_pooled(
            len(labels), self._cx, self._cy, _SAT_ORBIT_ACTOR, _SAT_R_ACTOR,
            _ACTOR_ANGLES_DEG)
        for (sx, sy), label in zip(positions, labels):
            self._add_satellite(
                scene, sx, sy, label, _SAT_D_ACTOR,
                resolve_semantic_color("map_sat_actor_bg"),
                resolve_semantic_color("map_sat_actor_fg"),
            )

    def _add_satellite(
        self,
        scene: QGraphicsScene,
        sx: float,
        sy: float,
        label: str,
        sat_d: int,
        bg: QColor,
        fg: QColor,
    ) -> None:
        d = float(sat_d)
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


# ── MapScene ──────────────────────────────────────────────────────────────────

class MapScene(QGraphicsScene):
    """Location graph scene with multi-layer overlay rendering.

    Methods
    -------
    load_map(locations, edges)
        Rebuilds the scene and applies the active layer set.
    set_active_layers(layers)
        Applies an arbitrary set of active layer names to every node.
    set_filter(filter_name)
        Compatibility wrapper: activates a single named layer.
    move_actor(actor_id, new_location_id)
        Updates actor tracking and refreshes actors-layer satellites.
    update_location(loc_id, metadata)
        Updates a node's tags/items and re-applies all active layers.
    node_count() / edge_count()
        Query helpers used by tests.
    marker_location(actor_id)
        Returns the current location_id of an actor.
    """

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self._nodes: dict[int, LocationNode] = {}
        self._edges: list[QGraphicsLineItem] = []
        self._marker_locations: dict[str, int] = {}
        self._active_layers: set[str] = {"terrain", "items", "actors"}
        self._location_actors: dict[int, list[str]] = {}
        self._location_items: dict[int, list[str]] = {}
        self._location_zone: dict[int, str] = {}
        self._location_region: dict[int, str] = {}
        self._zone_palette: dict[str, QColor] = {}
        self._region_palette: dict[str, QColor] = {}
        self._zone_explicit_colors: dict[str, QColor] = {}
        self._region_explicit_colors: dict[str, QColor] = {}
        self._global_item_letters: dict[str, str] = {}
        self._next_item_letter_idx: int = 0
        self._actor_label_map: dict[str, str] = {}

    # ── Public API ────────────────────────────────────────────────────────────

    def register_actor_labels(self, mapping: dict) -> None:
        """Stores the game-wide actor_id → display-label map from the C++ engine.

        Args:
            mapping: dict of ``{actor_id: label}`` as received in the actor
                     snapshot event.  Merged into any previously registered labels.
        """
        self._actor_label_map.update(mapping)

    def set_active_layers(self, layers: set) -> None:
        """Applies the given set of active layers to every location node.

        Args:
            layers: Any subset of ``{"terrain","items","actors","zone","region"}``.
        """
        self._active_layers = set(layers)
        for loc_id in self._nodes:
            self._apply_node_layers(loc_id)

    def set_filter(self, filter_name: str) -> None:
        """Compatibility wrapper: activates a single named layer (clears others)."""
        self.set_active_layers({filter_name})

    def load_map(
        self,
        locations: list[dict],
        edges: list[tuple[int, int]],
    ) -> None:
        """Clears the scene and rebuilds it from a snapshot.

        Accepts both the new-style ``tags`` list and the legacy
        ``metadata.terrain`` / ``metadata.items`` fields.
        Supports explicit zone/region colour tokens via ``zone_color_token``
        and ``region_color_token`` fields in each location dict.

        Args:
            locations: List of dicts with keys ``"location_id"``, ``"x"``,
                       ``"y"``, ``"metadata"``, ``"tags"``, ``"zone_id"``,
                       ``"region_id"``, ``"zone_color_token"``,
                       ``"region_color_token"``.
            edges:     List of ``(loc_id_a, loc_id_b)`` pairs.
        """
        self.clear()
        self._nodes = {}
        self._edges = []
        self._marker_locations = {}
        self._location_actors = {}
        self._location_items = {}
        self._location_zone = {}
        self._location_region = {}
        self._zone_palette = {}
        self._region_palette = {}
        self._zone_explicit_colors = {}
        self._region_explicit_colors = {}
        self._global_item_letters = {}
        self._next_item_letter_idx = 0

        self.setBackgroundBrush(QBrush(resolve_semantic_color("panel")))

        # ── Pre-compute node positions ─────────────────────────────────────────
        # Use BFS radial layout when no explicit x/y coordinates are provided.
        _any_explicit = any("x" in loc and "y" in loc for loc in locations)
        _id_order = [
            int(loc.get("location_id", i)) for i, loc in enumerate(locations)
        ]
        if not _any_explicit and _id_order:
            _adj_layout: dict[int, list[int]] = {}
            for _pair in edges:
                _pa, _pb = int(_pair[0]), int(_pair[1])
                _adj_layout.setdefault(_pa, []).append(_pb)
                _adj_layout.setdefault(_pb, []).append(_pa)
            _bfs = _bfs_radial_layout(_adj_layout, _id_order, step=90.0)
            if _bfs:
                _mx = min(p[0] for p in _bfs.values())
                _my = min(p[1] for p in _bfs.values())
                _bfs = {
                    k: (v[0] - _mx + 60.0, v[1] - _my + 60.0)
                    for k, v in _bfs.items()
                }
        else:
            _bfs = {}
        _circle_fb = _circle_positions(len(locations))

        for i, loc in enumerate(locations):
            loc_id = int(loc.get("location_id", i))
            if "x" in loc and "y" in loc:
                cx, cy = float(loc["x"]), float(loc["y"])
            elif _bfs:
                cx, cy = _bfs.get(loc_id, (60.0 + i * 90.0, 60.0))
            else:
                cx, cy = _circle_fb[i] if i < len(_circle_fb) else (60.0, 60.0)
            meta: dict = loc.get("metadata", {})
            if not isinstance(meta, dict):
                meta = {}
            # Merge explicit tags list with legacy metadata.terrain.
            tags: list[str] = list(loc.get("tags", []))
            terrain = str(meta.get("terrain", ""))
            if terrain and not any(t.startswith("terrain:") for t in tags):
                tags.append(f"terrain:{terrain}")
            items: list[str] = [str(x) for x in meta.get("items", [])]
            # Assign globally unique letters to items first encountered.
            for iid in items:
                if iid not in self._global_item_letters:
                    self._global_item_letters[iid] = _item_letter(
                        self._next_item_letter_idx)
                    self._next_item_letter_idx += 1
            self._location_items[loc_id] = items
            zone = str(loc.get("zone_id") or meta.get("zone_id") or "")
            region = str(loc.get("region_id") or meta.get("region_id") or "")
            if zone:
                self._location_zone[loc_id] = zone
                zone_token = loc.get("zone_color_token", "")
                if zone_token and zone not in self._zone_explicit_colors:
                    self._zone_explicit_colors[zone] = resolve_semantic_color(zone_token)
            if region:
                self._location_region[loc_id] = region
                region_token = loc.get("region_color_token", "")
                if region_token and region not in self._region_explicit_colors:
                    self._region_explicit_colors[region] = resolve_semantic_color(region_token)
            node_label = str(loc.get("label", "")) or str(loc_id)
            node = LocationNode(loc_id, cx, cy, tags, items, label=node_label)
            self.addItem(node)
            self._nodes[loc_id] = node

        # Fallback palette for zones/regions without explicit colours.
        n = len(_ZONE_PALETTE_TOKENS)
        for zone in set(self._location_zone.values()):
            if zone not in self._zone_explicit_colors:
                self._zone_palette[zone] = resolve_semantic_color(
                    _ZONE_PALETTE_TOKENS[_stable_idx(zone, n)])
        for region in set(self._location_region.values()):
            if region not in self._region_explicit_colors:
                self._region_palette[region] = resolve_semantic_color(
                    _ZONE_PALETTE_TOKENS[_stable_idx(region, n)])

        for pair in edges:
            a, b = int(pair[0]), int(pair[1])
            edge_type = str(pair[2]) if len(pair) > 2 else "FREE"
            if a in self._nodes and b in self._nodes:
                cx_a, cy_a = self._nodes[a].center()
                cx_b, cy_b = self._nodes[b].center()
                line = self.addLine(
                    cx_a, cy_a, cx_b, cy_b,
                    _edge_pen(edge_type),
                )
                line.setZValue(-1.0)
                self._edges.append(line)

        # Apply active layers now that all nodes are in the scene.
        for loc_id in self._nodes:
            self._apply_node_layers(loc_id)

    def move_actor(self, actor_id: str, new_location_id: int) -> None:
        """Updates actor location tracking and refreshes actors-layer satellites.

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
        self._marker_locations[actor_id] = new_location_id
        # Refresh actors-layer satellites on affected nodes.
        if "actors" in self._active_layers:
            if old_loc is not None and old_loc in self._nodes:
                self._apply_node_layers(old_loc)
            self._apply_node_layers(new_location_id)

    def update_location(self, loc_id: int, metadata: dict) -> None:
        """Applies new metadata to an existing node and re-applies active layers.

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
            for iid in items:
                if iid not in self._global_item_letters:
                    self._global_item_letters[iid] = _item_letter(
                        self._next_item_letter_idx)
                    self._next_item_letter_idx += 1
            self._location_items[loc_id] = items
            node.set_items(items)
        if "terrain" in metadata:
            terrain = str(metadata["terrain"])
            tags = [t for t in node._tags if not t.startswith("terrain:")]
            if terrain:
                tags.append(f"terrain:{terrain}")
            node._tags = tags
        self._apply_node_layers(loc_id)

    def node_count(self) -> int:
        """Returns the number of location nodes in the scene."""
        return len(self._nodes)

    def edge_count(self) -> int:
        """Returns the number of adjacency edges in the scene."""
        return len(self._edges)

    def marker_location(self, actor_id: str) -> int | None:
        """Returns the location_id where the actor currently is, or None."""
        return self._marker_locations.get(actor_id)

    # ── Internal helpers ──────────────────────────────────────────────────────

    def _apply_node_layers(self, loc_id: int) -> None:
        """Re-applies all active layers to a single node."""
        if loc_id not in self._nodes:
            return
        node = self._nodes[loc_id]
        zone = self._location_zone.get(loc_id, "")
        region = self._location_region.get(loc_id, "")
        zone_c = self._zone_explicit_colors.get(zone) or self._zone_palette.get(zone)
        region_c = (self._region_explicit_colors.get(region)
                    or self._region_palette.get(region))
        item_ids = self._location_items.get(loc_id, [])
        item_labels = {
            iid: self._global_item_letters[iid]
            for iid in item_ids if iid in self._global_item_letters
        }
        actor_ids = self._location_actors.get(loc_id, [])
        actor_labels = {
            aid: self._actor_label_map.get(aid, aid[0].upper() if aid else "?")
            for aid in actor_ids
        }
        node.apply_layers(self._active_layers, zone_c, region_c, item_labels, actor_labels)
