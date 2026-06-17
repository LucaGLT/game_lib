"""TimelineScene — QGraphicsScene for gmFlow timeline visualisation.

Renders gmFlow actors as labelled blocks on a horizontal time axis.
Full implementation: Phase 4.
Phase 1 stub: class skeleton with no-op methods.
"""
from __future__ import annotations

from PySide6.QtWidgets import QGraphicsScene


class TimelineScene(QGraphicsScene):
    """Renders gmFlow actors on a horizontal time axis.

    The X coordinate maps from ``TimelineValue`` (int) using
    ``pixels_per_unit`` (default 8 px per unit).

    A vertical line marks the current minimum timeline position and
    advances when ``advance_time()`` is called.

    Full implementation: Phase 4.
    """

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self._pixels_per_unit: int = 8

    def set_actors(self, actors: list[dict]) -> None:
        """Populates the scene with actor blocks from a snapshot.

        Args:
            actors: List of dicts with keys ``"actor_id"`` (str) and
                    ``"timeline_position"`` (int).

        Phase 1 stub: no-op.
        Full implementation: Phase 4.
        """
        # TODO: Phase 4 — clear scene, add QGraphicsRectItem per actor
        pass

    def select_actor(self, actor_id: str) -> None:
        """Highlights the block for *actor_id* as the currently active actor.

        Phase 1 stub: no-op.
        Full implementation: Phase 4.
        """
        # TODO: Phase 4 — set border colour and raise z-order on selected block
        pass

    def advance_time(self, new_time: int) -> None:
        """Moves the current-time vertical line to *new_time*.

        Phase 1 stub: no-op.
        Full implementation: Phase 4.
        """
        # TODO: Phase 4 — reposition the QGraphicsLineItem time cursor
        pass
