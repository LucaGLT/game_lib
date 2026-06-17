"""MapScene — QGraphicsScene for gmMap location-graph visualisation.

Renders gmMap LocationId nodes as ellipses and adjacency edges as lines.
Actor markers reposition in response to movement events.
Full implementation: Phase 8.
Phase 1 stub: class skeleton with no-op methods.
"""
from __future__ import annotations

from PySide6.QtWidgets import QGraphicsScene


class MapScene(QGraphicsScene):
    """Renders gmMap LocationId nodes, adjacency edges, and actor markers.

    Node colours are determined by the ``terrain`` metadata field via a
    configurable ``terrain_colours`` mapping (Phase 8).

    Full implementation: Phase 8.
    """

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)

    def load_map(self, locations: list[dict], edges: list[tuple]) -> None:
        """Builds the scene from a map snapshot.

        Args:
            locations: List of dicts with keys ``"id"`` (int) and
                       ``"metadata"`` (dict, must include ``"terrain"``).
            edges:     List of ``(src_id, dst_id)`` int tuples.

        Phase 1 stub: no-op.
        Full implementation: Phase 8.
        """
        # TODO: Phase 8 — clear scene, add LocationNode and AdjacencyEdge items
        pass

    def move_actor(self, actor_id: str, new_location_id: int) -> None:
        """Repositions the actor marker for *actor_id* onto *new_location_id*.

        Phase 1 stub: no-op.
        Full implementation: Phase 8.
        """
        # TODO: Phase 8 — look up ActorMarker by actor_id, move to node centre
        pass

    def update_location(self, loc_id: int, metadata: dict) -> None:
        """Updates the colour and tooltip of the node for *loc_id*.

        Phase 1 stub: no-op.
        Full implementation: Phase 8.
        """
        # TODO: Phase 8 — look up LocationNode, apply new terrain colour + tooltip
        pass
