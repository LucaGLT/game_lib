"""MapScene — QGraphicsScene for gmMap location-graph visualisation.

Renders gmMap LocationId nodes as ellipses and adjacency edges as lines.
Actor markers reposition in response to movement events.
"""
from __future__ import annotations

import math

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QPen
from PySide6.QtWidgets import (
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
)

from ..theme_manager import resolve_semantic_color

_NODE_DIAMETER: int = 32
_ACTOR_DIAMETER: int = 16


def _terrain_token(terrain: str) -> str:
    """Maps terrain metadata to semantic color token names."""
    if terrain in ("grass", "forest"):
        return "state_success"
    if terrain in ("water",):
        return "accent"
    if terrain in ("desert",):
        return "state_warning"
    if terrain in ("rock", "road"):
        return "state_disabled"
    return "panel"


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

    The fill colour reflects the ``terrain`` metadata field.
    The node is selectable so ``MapScene.selectionChanged`` can report details.
    """

    def __init__(
        self,
        loc_id: int,
        cx: float,
        cy: float,
        terrain: str = "",
        parent: QGraphicsItem | None = None,
    ) -> None:
        d = float(_NODE_DIAMETER)
        super().__init__(cx - d / 2.0, cy - d / 2.0, d, d, parent)
        self.loc_id: int = loc_id
        self._cx: float = cx
        self._cy: float = cy
        self._terrain: str = terrain
        self._items: list = []
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsSelectable, True)
        self._apply_terrain(terrain)

        # Centred label
        lbl = QGraphicsSimpleTextItem(str(loc_id), self)
        br = lbl.boundingRect()
        lbl.setPos(
            cx - d / 2.0 + (d - br.width()) / 2.0,
            cy - d / 2.0 + (d - br.height()) / 2.0,
        )
        self._update_tooltip()

    def _apply_terrain(self, terrain: str) -> None:
        color = resolve_semantic_color(_terrain_token(terrain))
        self.setBrush(QBrush(color))
        self.setPen(QPen(resolve_semantic_color("border"), 1))

    def _update_tooltip(self) -> None:
        parts = [f"Location #{self.loc_id}"]
        if self._terrain:
            parts.append(f"Terrain: {self._terrain}")
        if self._items:
            parts.append(f"Items: {', '.join(str(i) for i in self._items)}")
        self.setToolTip("\n".join(parts))

    def update_metadata(self, metadata: dict) -> None:
        """Applies new terrain colour and/or item list from *metadata*.

        Only fields present in *metadata* are updated; absent fields keep
        their previous value.

        Args:
            metadata: Dict with optional keys ``"terrain"`` (str) and
                      ``"items"`` (list).
        """
        if "terrain" in metadata:
            self._terrain = str(metadata["terrain"])
            self._apply_terrain(self._terrain)
        if "items" in metadata:
            self._items = list(metadata["items"])
        self._update_tooltip()

    def center(self) -> tuple[float, float]:
        """Returns the (x, y) centre of this node in scene coordinates."""
        return (self._cx, self._cy)


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
    """Location graph scene: nodes, adjacency edges, and actor markers.

    Methods
    -------
    load_map(locations, edges)
        Rebuilds the scene from a snapshot.
    move_actor(actor_id, new_location_id)
        Repositions an actor marker.
    update_location(loc_id, metadata)
        Updates a node's colour and tooltip.
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

    # ── Public API ────────────────────────────────────────────────────────────

    def load_map(
        self,
        locations: list[dict],
        edges: list[tuple[int, int]],
    ) -> None:
        """Clears the scene and rebuilds it from a snapshot.

        Args:
            locations: List of dicts with ``"location_id"`` (int), optional
                       ``"x"``/``"y"`` (float) and ``"metadata"`` (dict).
            edges:     List of ``(loc_id_a, loc_id_b)`` pairs or two-element
                       lists.
        """
        self.clear()
        self._nodes = {}
        self._edges = []
        self._markers = {}
        self._marker_locations = {}
        
        # Set scene background to theme panel color.
        self.setBackgroundBrush(QBrush(resolve_semantic_color("panel")))

        positions = _circle_positions(len(locations))
        for i, loc in enumerate(locations):
            loc_id = int(loc.get("location_id", i))
            cx = float(loc.get("x", positions[i][0]))
            cy = float(loc.get("y", positions[i][1]))
            meta = loc.get("metadata", {})
            terrain = str(meta.get("terrain", "")) if isinstance(meta, dict) else ""
            node = LocationNode(loc_id, cx, cy, terrain)
            self.addItem(node)
            self._nodes[loc_id] = node

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

    def move_actor(self, actor_id: str, new_location_id: int) -> None:
        """Places (or moves) an actor marker at the given location.

        Args:
            actor_id:        Actor identifier.
            new_location_id: Destination location ID (must exist in scene).
        """
        if new_location_id not in self._nodes:
            return
        if actor_id in self._markers:
            self.removeItem(self._markers[actor_id])
            del self._markers[actor_id]
        cx, cy = self._nodes[new_location_id].center()
        marker = ActorMarker(actor_id, cx, cy)
        self.addItem(marker)
        self._markers[actor_id] = marker
        self._marker_locations[actor_id] = new_location_id

    def update_location(self, loc_id: int, metadata: dict) -> None:
        """Applies new metadata to an existing node.

        Args:
            loc_id:   Target location ID.
            metadata: Dict with optional keys ``"terrain"`` and ``"items"``.
        """
        if loc_id in self._nodes:
            self._nodes[loc_id].update_metadata(metadata)

    def node_count(self) -> int:
        """Returns the number of location nodes in the scene."""
        return len(self._nodes)

    def edge_count(self) -> int:
        """Returns the number of adjacency edges in the scene."""
        return len(self._edges)

    def marker_location(self, actor_id: str) -> int | None:
        """Returns the location_id where the actor marker currently sits, or None."""
        return self._marker_locations.get(actor_id)



