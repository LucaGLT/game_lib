"""Le Pergamene di Eldhôm — timeline widget.

TimelineWidget shows all actors sorted by timeline position as a
horizontal strip of chips.  The actor that acts next is highlighted.

All visual styling is applied exclusively through QSS via the dynamic
properties ``chip_type`` (``"hero"`` | ``"enemy"``) and ``chip_active``
(``"true"`` | ``"false"``).  No hardcoded color values are present.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QWidget,
)
from PySide6.QtCore import Qt


class _TimelineChip(QLabel):
    """Small label chip representing one actor on the timeline."""

    def __init__(self, actor_id: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._actor_id = actor_id
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setProperty("chip_type", "hero")
        self.setProperty("chip_active", "false")

    def update_chip(
        self, name: str, timeline: int, is_hero: bool, is_active: bool
    ) -> None:
        """Refreshes chip text and QSS dynamic properties.

        Args:
            name:      Actor display name.
            timeline:  Current timeline position value.
            is_hero:   True for PG/allies; False for monsters/bosses.
            is_active: True when this actor acts next.
        """
        self.setText(f"{name}\n\u231b{timeline}")
        new_type   = "hero" if is_hero else "enemy"
        new_active = "true" if is_active else "false"
        if (
            self.property("chip_type") != new_type
            or self.property("chip_active") != new_active
        ):
            self.setProperty("chip_type", new_type)
            self.setProperty("chip_active", new_active)
            self.style().polish(self)


class TimelineWidget(QFrame):
    """Horizontal strip showing all actors sorted by timeline position."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._chips: dict[str, _TimelineChip] = {}
        self._actor_meta: dict[str, dict] = {}   # actor_id → {is_hero, name, timeline}
        self._active_actor: str = ""

        scroll = QScrollArea(self)
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        scroll.setFrameShape(QFrame.Shape.NoFrame)

        inner = QWidget()
        self._inner_layout = QHBoxLayout(inner)
        self._inner_layout.setSpacing(8)
        self._inner_layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        self._inner_layout.setContentsMargins(8, 4, 8, 4)
        scroll.setWidget(inner)

        outer = QHBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(scroll)

        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFixedHeight(72)

    def on_state_full(self, msg: dict) -> None:
        """Rebuilds the timeline from a full state snapshot."""
        data = _extract_data(msg)
        actors: list[dict] = []
        for hero in data.get("heroes", []):
            actors.append({
                "id":       hero["id"],
                "name":     hero.get("name", hero["id"]),
                "timeline": hero.get("timeline", 0),
                "is_hero":  True,
            })
        for grp in data.get("groups", []):
            actors.append({
                "id":       grp["id"],
                "name":     grp.get("name", grp["id"]),
                "timeline": grp.get("timeline", 0),
                "is_hero":  False,
            })
        next_id = data.get("next_actor", {}).get("actor_id", "")
        self._rebuild(actors, next_id)

    def on_next_actor(self, msg: dict) -> None:
        """Updates the active chip from a turn.next_actor event."""
        data = _extract_data(msg)
        self._set_active(data.get("actor_id", ""))

    def update_actor_timeline(self, actor_id: str, new_timeline: int) -> None:
        """Updates a single chip's timeline value.

        Args:
            actor_id:     ID of the actor to update.
            new_timeline: New timeline position.
        """
        if actor_id not in self._chips:
            return
        meta = self._actor_meta.get(actor_id, {})
        meta["timeline"] = new_timeline
        self._chips[actor_id].update_chip(
            name=meta.get("name", actor_id),
            timeline=new_timeline,
            is_hero=meta.get("is_hero", True),
            is_active=(actor_id == self._active_actor),
        )

    # ── Internal ──────────────────────────────────────────────────────────────

    def _rebuild(self, actors: list[dict], active_id: str) -> None:
        """Clears and rebuilds all chips from a sorted actor list."""
        for chip in self._chips.values():
            self._inner_layout.removeWidget(chip)
            chip.setParent(None)
            chip.deleteLater()
        self._chips.clear()
        self._actor_meta.clear()

        actors_sorted = sorted(actors, key=lambda a: a.get("timeline", 0))
        for actor in actors_sorted:
            aid = str(actor["id"])
            self._actor_meta[aid] = {
                "name":     actor.get("name", aid),
                "timeline": actor.get("timeline", 0),
                "is_hero":  actor.get("is_hero", True),
            }
            chip = _TimelineChip(aid)
            chip.update_chip(
                name=actor.get("name", aid),
                timeline=actor.get("timeline", 0),
                is_hero=actor.get("is_hero", True),
                is_active=(aid == active_id),
            )
            self._chips[aid] = chip
            self._inner_layout.addWidget(chip)

        self._active_actor = active_id

    def _set_active(self, active_id: str) -> None:
        old_id = self._active_actor
        self._active_actor = active_id
        if old_id in self._chips:
            chip = self._chips[old_id]
            chip.setProperty("chip_active", "false")
            chip.style().polish(chip)
        if active_id in self._chips:
            chip = self._chips[active_id]
            chip.setProperty("chip_active", "true")
            chip.style().polish(chip)


def _extract_data(msg: dict) -> dict:
    return msg.get("data", msg)


def _extract_data(msg: dict) -> dict:
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    import json
    try:
        return json.loads(raw)
    except Exception:
        return {}
