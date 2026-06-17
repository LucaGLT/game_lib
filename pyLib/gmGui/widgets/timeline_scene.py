"""TimelineScene — QGraphicsScene for gmFlow timeline visualisation.

Renders gmFlow actors as labelled blocks on a horizontal time axis.
Each actor is a ``QGraphicsRectItem`` labelled with its ``actor_id``.
The active actor is highlighted with a yellow border and raised z-order.
A vertical cyan line marks the current minimum timeline position.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QPen
from PySide6.QtWidgets import (
    QGraphicsLineItem,
    QGraphicsRectItem,
    QGraphicsScene,
)


class TimelineScene(QGraphicsScene):
    """Renders gmFlow actors on a horizontal time axis.

    The X coordinate maps from ``TimelineValue`` (int) using
    ``_pixels_per_unit`` (default 8 px per unit).

    A vertical cyan line marks the current minimum timeline position and
    moves when :meth:`advance_time` is called.
    """

    _BLOCK_WIDTH: int = 60
    _BLOCK_HEIGHT: int = 40
    _pixels_per_unit: int = 8

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self._actor_rects: dict[str, QGraphicsRectItem] = {}
        self._selected_id: str | None = None
        self._current_time: int = 0
        self._time_line: QGraphicsLineItem | None = None

    def set_actors(self, actors: list[dict]) -> None:
        """Populates the scene with actor blocks from a snapshot.

        Clears all existing items first.  Any previous :meth:`select_actor`
        selection is re-applied if the actor is still present.

        Args:
            actors: List of dicts with keys ``"actor_id"`` (str) and
                    ``"timeline_position"`` (int).
        """
        self.clear()
        self._actor_rects = {}
        self._time_line = None

        inactive_pen: QPen = QPen(Qt.GlobalColor.darkGray, 1)

        for actor in actors:
            actor_id: str = str(actor.get("actor_id", "?"))
            pos: int = int(actor.get("timeline_position", 0))
            x: float = float(pos * self._pixels_per_unit)

            rect: QGraphicsRectItem = self.addRect(
                x, 0.0,
                float(self._BLOCK_WIDTH), float(self._BLOCK_HEIGHT),
                inactive_pen,
            )
            label = self.addText(actor_id)
            label.setPos(x + 2.0, 2.0)
            label.setParentItem(rect)
            self._actor_rects[actor_id] = rect

        # Vertical time cursor
        x_cur: float = float(self._current_time * self._pixels_per_unit)
        self._time_line = self.addLine(
            x_cur, -10.0,
            x_cur, float(self._BLOCK_HEIGHT + 10),
            QPen(Qt.GlobalColor.cyan, 2),
        )

        # Re-apply existing selection if the actor is still in the scene.
        if self._selected_id is not None and self._selected_id in self._actor_rects:
            self._apply_highlight(self._selected_id)

    def select_actor(self, actor_id: str) -> None:
        """Highlights the block for *actor_id* as the currently active actor.

        Clears the highlight on the previously selected actor.
        Safe to call when the scene has no actors yet (stores the id for later).
        """
        if self._selected_id is not None and self._selected_id in self._actor_rects:
            self._actor_rects[self._selected_id].setPen(
                QPen(Qt.GlobalColor.darkGray, 1)
            )
            self._actor_rects[self._selected_id].setZValue(0.0)

        self._selected_id = actor_id
        if actor_id in self._actor_rects:
            self._apply_highlight(actor_id)

    def advance_time(self, new_time: int) -> None:
        """Moves the current-time vertical line to *new_time*.

        Updates ``_current_time`` even if the scene has no time line yet.
        """
        self._current_time = new_time
        if self._time_line is not None:
            x: float = float(new_time * self._pixels_per_unit)
            self._time_line.setLine(x, -10.0, x, float(self._BLOCK_HEIGHT + 10))

    # ── Internal ──────────────────────────────────────────────────────────────

    def _apply_highlight(self, actor_id: str) -> None:
        """Applies active-actor styling to *actor_id*'s rect."""
        rect: QGraphicsRectItem = self._actor_rects[actor_id]
        rect.setPen(QPen(Qt.GlobalColor.yellow, 3))
        rect.setZValue(1.0)
