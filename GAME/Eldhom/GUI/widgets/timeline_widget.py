"""Le Pergamene di Eldhom — timeline widget.

TimelineWidget shows all actors sorted by timeline position as a
horizontal strip of colored chips.  The actor that acts next is
highlighted with a border.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QWidget,
)
from PySide6.QtCore import Qt


class _TimelineChip(QLabel):
    """Small label chip representing one actor on the timeline."""

    HERO_STYLE = (
        "QLabel { background:#2d2510; border:1px solid #7a5a20; border-radius:12px;"
        " color:#d4b07a; padding:2px 8px; font-size:11px; }"
    )
    MONSTER_STYLE = (
        "QLabel { background:#2d1010; border:1px solid #7a2020; border-radius:12px;"
        " color:#d47a7a; padding:2px 8px; font-size:11px; }"
    )
    ACTIVE_HERO_STYLE = (
        "QLabel { background:#4a3c10; border:2px solid #c8a060; border-radius:12px;"
        " color:#ffe080; padding:2px 8px; font-size:11px; font-weight:bold; }"
    )
    ACTIVE_MONSTER_STYLE = (
        "QLabel { background:#4a1010; border:2px solid #e05050; border-radius:12px;"
        " color:#ff8080; padding:2px 8px; font-size:11px; font-weight:bold; }"
    )

    def __init__(self, actor_id: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._actor_id = actor_id
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def update_chip(
        self, name: str, timeline: int, is_hero: bool, is_active: bool
    ) -> None:
        """Refreshes chip text and style."""
        self.setText(f"{name}\n⌛{timeline}")
        if is_active:
            self.setStyleSheet(
                self.ACTIVE_HERO_STYLE if is_hero else self.ACTIVE_MONSTER_STYLE
            )
        else:
            self.setStyleSheet(self.HERO_STYLE if is_hero else self.MONSTER_STYLE)


class TimelineWidget(QFrame):
    """Horizontal strip showing all actors sorted by timeline_position."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._chips: dict[str, _TimelineChip] = {}
        self._active_actor: str = ""

        layout = QHBoxLayout(self)
        layout.setSpacing(6)
        layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        layout.setContentsMargins(6, 4, 6, 4)
        self._layout = layout

        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFixedHeight(70)
        self.setStyleSheet("QFrame { background:#141414; border:none; }")

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

        next_actor_id = data.get("next_actor", {}).get("actor_id", "")
        self._rebuild(actors, next_actor_id)

    def on_next_actor(self, msg: dict) -> None:
        """Updates the active chip from a turn.next_actor event."""
        data = _extract_data(msg)
        actor_id = data.get("actor_id", "")
        self._active_actor = actor_id
        for aid, chip in self._chips.items():
            # Read current text to extract timeline — simpler than re-storing
            chip_text = chip.text()
            is_hero = "hero" in chip.styleSheet().lower() or "c8a060" in chip.styleSheet()
            # Just toggle the active style
            chip.setStyleSheet(
                (_TimelineChip.ACTIVE_HERO_STYLE if is_hero else _TimelineChip.ACTIVE_MONSTER_STYLE)
                if aid == actor_id
                else (_TimelineChip.HERO_STYLE if is_hero else _TimelineChip.MONSTER_STYLE)
            )

    def update_actor_timeline(self, actor_id: str, new_timeline: int) -> None:
        """Updates a single chip's timeline value."""
        if actor_id in self._chips:
            chip = self._chips[actor_id]
            old_text = chip.text().split("\n")[0]
            chip.setText(f"{old_text}\n⌛{new_timeline}")
            self._sort_chips()

    # ── Internal ──────────────────────────────────────────────────────────────

    def _rebuild(self, actors: list[dict], active_id: str) -> None:
        """Clears and rebuilds all chips from a sorted actor list."""
        for chip in self._chips.values():
            chip.deleteLater()
        self._chips.clear()

        actors_sorted = sorted(actors, key=lambda a: a["timeline"])
        for actor in actors_sorted:
            aid = actor["id"]
            chip = _TimelineChip(aid, self)
            chip.update_chip(
                actor["name"],
                actor["timeline"],
                actor["is_hero"],
                aid == active_id,
            )
            self._chips[aid] = chip
            self._layout.addWidget(chip)

        self._active_actor = active_id

    def _sort_chips(self) -> None:
        """Re-sorts the chip order in the layout by timeline value.

        Reads the current timeline from the chip text (last line after \\n).
        """
        chip_items = list(self._chips.items())
        chip_items.sort(key=lambda kv: _read_timeline_from_chip(kv[1]))
        for _aid, chip in chip_items:
            self._layout.removeWidget(chip)
            self._layout.addWidget(chip)


def _read_timeline_from_chip(chip: _TimelineChip) -> int:
    text = chip.text()
    try:
        return int(text.split("⌛")[-1].strip())
    except ValueError:
        return 0


def _extract_data(msg: dict) -> dict:
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    import json
    try:
        return json.loads(raw)
    except Exception:
        return {}
