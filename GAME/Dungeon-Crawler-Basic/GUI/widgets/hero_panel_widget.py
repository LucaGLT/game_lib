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
        # ToBeImplemented //

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and refreshes the actor display.

        Handles: ``dungeon.actor.snapshot``, ``dungeon.actor.hp_changed``,
        ``dungeon.actor.status_changed``, ``dungeon.session.started``.

        Args:
            msg: Decoded event dict with ``typeId`` and ``data`` keys.
        """
        # ToBeImplemented //

    def _refresh_actor(self, actor_data: dict) -> None:
        """Updates the display row for a single actor.

        Args:
            actor_data: Dict with keys id, kind, hp, max_hp, tags, statuses.
        """
        # ToBeImplemented //

    def reset(self) -> None:
        """Removes all actor rows from the panel."""
        # ToBeImplemented //
