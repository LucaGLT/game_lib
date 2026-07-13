"""TimelineWidget — horizontal timeline panel with actor tokens and milestone markers.

Shows actors as labelled blocks on a horizontal time axis, ordered by their
``position`` value.  Milestone thresholds are drawn as vertical markers.
The ``active_id`` actor is highlighted with the theme accent border.

Data format (from ``gmflow.timeline.actors_updated``)::

    {
        "actors": [
            {"id": "pg_1", "label": "Eran",     "position": 5,  "kind": "hero"},
            {"id": "mob_a","label": "Goblin A",  "position": 7,  "kind": "monster_group"}
        ],
        "milestones": [12, 24, 60],
        "active_id": "pg_1"
    }

``kind`` values map to semantic QSS properties (no hardcoded colours).
Supported kinds: ``hero``, ``ally``, ``monster_group``, ``boss``, ``system``.
Unknown kinds fall back to the ``neutral`` tone.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QPen
from PySide6.QtWidgets import (
    QGraphicsLineItem,
    QGraphicsRectItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
    QGraphicsView,
    QLabel,
    QVBoxLayout,
    QWidget,
)

from ..theme_manager import resolve_semantic_color

# ── Kind → semantic-colour role mapping ───────────────────────────────────────
# Maps a ``kind`` string to a semantic colour name understood by
# ``resolve_semantic_color()``.  Game adapters should use exactly these keys.

_KIND_TONE: dict[str, str] = {
    "hero":          "accent",
    "ally":          "state_success_dark",
    "monster_group": "state_error",
    "boss":          "state_warning",
    "system":        "border",
}

# Pixels per timeline unit (used to convert ``position`` to x-coordinate)
_PX_PER_UNIT: int = 64
_BLOCK_W: int = 56
_BLOCK_H: int = 40
_ROW_Y:   int = 16
_LABEL_OFFSET_Y: int = 4
_MILESTONE_LABEL_Y: int = 4


class TimelineWidget(QWidget):
    """Composite widget that renders a horizontal game timeline.

    Call :meth:`update_state` to refresh the display from a data dict.
    The widget is read-only; it emits no signals.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._build_layout()

    # ── Layout construction ────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        layout = QVBoxLayout()
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(4)

        title = QLabel("Linea Temporale")
        title.setProperty("text_role", "subtitle")
        layout.addWidget(title)

        self._scene: QGraphicsScene = QGraphicsScene(self)
        self._view: QGraphicsView = QGraphicsView(self._scene)
        self._view.setFixedHeight(80)
        self._view.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAsNeeded
        )
        self._view.setVerticalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )
        self._view.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop)
        layout.addWidget(self._view)

        self.setLayout(layout)

    # ── Public API ─────────────────────────────────────────────────────────────

    def update_state(self, data: dict) -> None:
        """Redraws the timeline from a state snapshot.

        Args:
            data: Dict with keys ``actors`` (list), ``milestones`` (list[int]),
                  ``active_id`` (str).  Unknown keys are ignored.
        """
        self._scene.clear()
        self._scene.setBackgroundBrush(QBrush(resolve_semantic_color("panel")))

        actors: list[dict]   = data.get("actors", [])
        milestones: list[int] = data.get("milestones", [])
        active_id: str        = data.get("active_id", "")

        self._draw_milestones(milestones)
        self._draw_actors(actors, active_id)

    # ── Internal drawing helpers ───────────────────────────────────────────────

    def _x_for(self, position: int) -> float:
        """Converts a timeline position value to a scene x-coordinate."""
        return float(8 + position * _PX_PER_UNIT)

    def _draw_milestones(self, milestones: list[int]) -> None:
        """Draws thin vertical marker lines at each milestone threshold."""
        milestone_color = resolve_semantic_color("state_warning")
        pen = QPen(milestone_color, 1, Qt.PenStyle.DashLine)
        height = float(_ROW_Y + _BLOCK_H + 8)

        for threshold in milestones:
            x = self._x_for(threshold)
            line: QGraphicsLineItem = self._scene.addLine(
                x, 0.0, x, height, pen
            )
            line.setZValue(-1)

            # Milestone value label above the line
            lbl: QGraphicsSimpleTextItem = self._scene.addSimpleText(
                str(threshold)
            )
            lbl.setPos(x + 2.0, float(_MILESTONE_LABEL_Y))
            lbl.setBrush(QBrush(milestone_color))
            lbl.setZValue(1)

    def _draw_actors(self, actors: list[dict], active_id: str) -> None:
        """Draws one labelled block per actor, highlighting the active one."""
        border_color  = resolve_semantic_color("border")
        accent_color  = resolve_semantic_color("accent")
        text_color    = resolve_semantic_color("text")
        panel_color   = resolve_semantic_color("panel")

        for actor in actors:
            actor_id: str = str(actor.get("id", "?"))
            label:    str = str(actor.get("label", actor_id))
            position: int = int(actor.get("position", 0))
            kind:     str = str(actor.get("kind", ""))

            x = self._x_for(position) - _BLOCK_W / 2.0
            y = float(_ROW_Y)

            # Choose fill colour from kind mapping; fall back to panel.
            tone_key = _KIND_TONE.get(kind, "")
            if tone_key:
                fill_color = resolve_semantic_color(tone_key).lighter(160)
            else:
                fill_color = panel_color

            # Border: accent for active actor, normal border otherwise.
            is_active = (actor_id == active_id)
            if is_active:
                pen = QPen(accent_color, 2)
            else:
                pen = QPen(border_color, 1)

            rect: QGraphicsRectItem = self._scene.addRect(
                x, y, float(_BLOCK_W), float(_BLOCK_H),
                pen, QBrush(fill_color)
            )
            rect.setZValue(0 if not is_active else 1)

            # Actor label centred in the block.
            text_item: QGraphicsSimpleTextItem = self._scene.addSimpleText(label)
            text_item.setBrush(QBrush(text_color))
            tw = text_item.boundingRect().width()
            text_item.setPos(
                x + (_BLOCK_W - tw) / 2.0,
                y + float(_LABEL_OFFSET_Y)
            )
            text_item.setZValue(2)

            # Position value below the label.
            pos_item: QGraphicsSimpleTextItem = self._scene.addSimpleText(
                str(position)
            )
            pos_item.setBrush(QBrush(text_color))
            pw = pos_item.boundingRect().width()
            pos_item.setPos(
                x + (_BLOCK_W - pw) / 2.0,
                y + float(_LABEL_OFFSET_Y + 16)
            )
            pos_item.setZValue(2)
