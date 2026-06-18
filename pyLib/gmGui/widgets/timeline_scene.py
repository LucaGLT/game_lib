"""TimelineScene — QGraphicsScene for gmFlow timeline visualisation.

Renders gmFlow actors as labelled blocks on a horizontal time axis.
Each actor is a ``QGraphicsRectItem`` labelled with its ``actor_id``.
The active actor is highlighted with a yellow border and raised z-order.
A vertical cyan line marks the current minimum timeline position.
"""
from __future__ import annotations

from PySide6.QtGui import QBrush, QPen
from PySide6.QtWidgets import (
    QGraphicsLineItem,
    QGraphicsRectItem,
    QGraphicsScene,
)

from ..theme_manager import build_typography_font, resolve_semantic_color


class TimelineScene(QGraphicsScene):
    """Renders gmFlow actors on a horizontal time axis.

    The X coordinate maps from ``TimelineValue`` (int) using
    ``_pixels_per_unit`` (default 8 px per unit).

    A vertical cyan line marks the current minimum timeline position and
    moves when :meth:`advance_time` is called.
    """

    _BLOCK_WIDTH: int = 70
    _BLOCK_HEIGHT: int = 42
    _BLOCK_GAP: int = 8
    _LEFT_PADDING: int = 16
    _TOP_PADDING: int = 8
    _pixels_per_unit: int = _BLOCK_WIDTH + _BLOCK_GAP

    def __init__(self, parent: object = None) -> None:
        super().__init__(parent)
        self._actor_rects: dict[str, QGraphicsRectItem] = {}
        self._selected_id: str | None = None
        self._current_time: int = 0
        self._time_line: QGraphicsLineItem | None = None
        self._default_pens: dict[str, QPen] = {}

    def set_actors(self, actors: list[dict]) -> None:
        """Populates the scene with actor blocks from a snapshot.

        Clears all existing items first.  Any previous :meth:`select_actor`
        selection is re-applied if the actor is still present.

        Args:
            actors: List of dicts with keys:
                    - ``"actor_id"`` (str, required): unique identifier for dict key
                    - ``"timeline_position"`` (int, required): x position
                    - ``"label"`` (str, optional): display label (defaults to actor_id)
        """
        self.clear()
        self._actor_rects = {}
        self._time_line = None
        self._default_pens = {}
        
        # Set scene background to theme panel color.
        self.setBackgroundBrush(QBrush(resolve_semantic_color("panel")))

        base_font = build_typography_font("subtitle")

        for actor in actors:
            actor_id: str = str(actor.get("actor_id", "?"))
            pos: int = int(actor.get("timeline_position", 0))
            label_text: str = str(actor.get("label", actor_id))
            x: float = float(self._LEFT_PADDING + pos * self._pixels_per_unit)
            y: float = float(self._TOP_PADDING)

            if label_text == "X":
                pen: QPen = QPen(resolve_semantic_color("accent"), 2)
                brush: QBrush = QBrush(resolve_semantic_color("panel"))
                text_color = resolve_semantic_color("text")
            elif label_text == "O":
                pen = QPen(resolve_semantic_color("border"), 2)
                brush = QBrush(resolve_semantic_color("panel"))
                text_color = resolve_semantic_color("text")
            else:
                pen = QPen(resolve_semantic_color("border"), 1)
                brush = QBrush(resolve_semantic_color("panel"))
                text_color = resolve_semantic_color("text")

            rect: QGraphicsRectItem = self.addRect(
                x, y,
                float(self._BLOCK_WIDTH), float(self._BLOCK_HEIGHT),
                pen,
                brush,
            )
            self._default_pens[actor_id] = QPen(pen)

            label = self.addText(label_text)
            label.setDefaultTextColor(text_color)
            label.setFont(base_font)
            text_rect = label.boundingRect()
            label_x = x + (self._BLOCK_WIDTH - text_rect.width()) / 2.0
            label_y = y + (self._BLOCK_HEIGHT - text_rect.height()) / 2.0
            label.setPos(label_x, label_y)
            label.setParentItem(rect)
            self._actor_rects[actor_id] = rect

        scene_width = self._LEFT_PADDING + max(1, len(actors)) * self._pixels_per_unit + 20
        scene_height = self._TOP_PADDING + self._BLOCK_HEIGHT + 20
        self.setSceneRect(0.0, 0.0, float(scene_width), float(scene_height))

        # Vertical time cursor
        x_cur: float = float(
            self._LEFT_PADDING
            + self._current_time * self._pixels_per_unit
            + self._BLOCK_WIDTH / 2
        )
        self._time_line = self.addLine(
            x_cur,
            float(self._TOP_PADDING - 8),
            x_cur,
            float(self._TOP_PADDING + self._BLOCK_HEIGHT + 8),
            QPen(resolve_semantic_color("accent"), 2),
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
            old_pen: QPen | None = self._default_pens.get(self._selected_id)
            if old_pen is not None:
                self._actor_rects[self._selected_id].setPen(old_pen)
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
            x: float = float(
                self._LEFT_PADDING
                + new_time * self._pixels_per_unit
                + self._BLOCK_WIDTH / 2
            )
            self._time_line.setLine(
                x,
                float(self._TOP_PADDING - 8),
                x,
                float(self._TOP_PADDING + self._BLOCK_HEIGHT + 8),
            )

    # ── Internal ──────────────────────────────────────────────────────────────

    def _apply_highlight(self, actor_id: str) -> None:
        """Applies active-actor styling to *actor_id*'s rect."""
        rect: QGraphicsRectItem = self._actor_rects[actor_id]
        rect.setPen(QPen(resolve_semantic_color("state_active"), 3))
        rect.setZValue(1.0)
