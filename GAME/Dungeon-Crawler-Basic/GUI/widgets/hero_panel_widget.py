"""Dungeon Crawler Basic — hero/actor panel widget.

HeroPanelWidget displays the current state of all actors in the dungeon:
hero HP, monster HP, active statuses and equipped items. It updates on
``dungeon.actor.snapshot`` and ``dungeon.actor.hp_changed`` / ``status_changed``
events from the CoreEngine.

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class HeroPanelWidget(QWidget):
    """Scrollable panel listing all dungeon actors with their current state.

    Displays for each actor: id, kind, current HP / max HP, active statuses
    and relevant tags (has_potion, equipped_weapon, wounded, etc.).

    The panel is read-only; user interaction for actions is handled by
    :class:`ActionPanelWidget`.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the panel with an empty actor list."""
        super().__init__(parent)
        from PySide6.QtWidgets import QVBoxLayout, QScrollArea, QWidget as _W
        outer = QVBoxLayout(self)
        outer.setContentsMargins(2, 2, 2, 2)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        self._container = _W()
        self._layout = QVBoxLayout(self._container)
        self._layout.setSpacing(4)
        self._layout.addStretch()
        scroll.setWidget(self._container)
        outer.addWidget(scroll)
        self._actors: dict[str, "QLabel"] = {}

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and refreshes the actor display."""
        type_id = msg.get("typeId", "")
        data = msg.get("data", {})
        if type_id == "dungeon.actor.snapshot":
            for actor in data.get("actors", []):
                self._refresh_actor(actor)
        elif type_id in ("dungeon.actor.hp_changed", "dungeon.actor.status_changed"):
            actor_id = data.get("actor_id", "")
            if actor_id in self._actors:
                lbl = self._actors[actor_id]
                current_text = lbl.text()
                # Update HP inline when hp_after present
                if "hp_after" in data:
                    parts = current_text.split(" | ")
                    if parts:
                        # rebuild minimal text keeping first part (id/kind)
                        lbl.setText(parts[0] + f" | HP: {data['hp_after']}")
        elif type_id == "dungeon.session.started":
            self.reset()

    def _refresh_actor(self, actor_data: dict) -> None:
        """Updates the display row for a single actor."""
        from PySide6.QtWidgets import QLabel
        actor_id = actor_data.get("id", "?")
        kind = actor_data.get("kind", "?")
        hp = actor_data.get("hp", 0)
        max_hp = actor_data.get("max_hp", 0)
        tags = actor_data.get("tags", [])
        statuses = actor_data.get("statuses", [])
        text = f"{actor_id} [{kind}] | HP: {hp}/{max_hp}"
        if tags:
            text += f" | {', '.join(tags)}"
        if statuses:
            text += f" | status: {', '.join(statuses)}"
        if actor_id in self._actors:
            self._actors[actor_id].setText(text)
        else:
            lbl = QLabel(text)
            lbl.setWordWrap(True)
            self._actors[actor_id] = lbl
            self._layout.insertWidget(self._layout.count() - 1, lbl)

    def reset(self) -> None:
        """Removes all actor rows from the panel."""
        for lbl in self._actors.values():
            self._layout.removeWidget(lbl)
            lbl.deleteLater()
        self._actors.clear()
