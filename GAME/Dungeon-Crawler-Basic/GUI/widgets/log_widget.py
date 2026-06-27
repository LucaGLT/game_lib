"""Dungeon Crawler Basic — game log widget.

LogWidget displays a scrollable list of game log messages emitted by the
CoreEngine. Each entry shows a timestamp, an actor identifier and a
human-readable description of the event (action performed, rejection reason,
session start/end, etc.).

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class LogWidget(QWidget):
    """Scrollable read-only log of game events.

    Appends one entry per relevant event received from the CoreEngine.
    Older entries are kept visible for the duration of the session.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates an empty log widget."""
        super().__init__(parent)
        from PySide6.QtWidgets import QVBoxLayout, QListWidget
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        self._list = QListWidget()
        self._list.setWordWrap(True)
        layout.addWidget(self._list)

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and appends a log entry if relevant."""
        type_id = msg.get("typeId", "")
        data = msg.get("data", {})
        if type_id == "dungeon.session.started":
            self.append_entry(f"[Session] Started — round {data.get('round', 1)}")
        elif type_id == "dungeon.actor.moved":
            self.append_entry(f"[Move] {data.get('actor_id')} → {data.get('to')}")
        elif type_id == "dungeon.actor.healed":
            self.append_entry(
                f"[Heal] {data.get('actor_id')} healed {data.get('target_id')} "
                f"(+{data.get('amount', 0)} HP, now {data.get('hp_after')})"
            )
        elif type_id == "dungeon.actor.equipped":
            self.append_entry(f"[Equip] {data.get('actor_id')} equipped {data.get('item_tag')}")
        elif type_id == "dungeon.attack.declared":
            self.append_entry(
                f"[Attack] {data.get('attacker_id')} → {data.get('defender_id')} "
                f"(danno base {data.get('base_damage', 0)}, fonte {data.get('source', 'base')})"
            )
        elif type_id == "dungeon.defense.window.opened":
            self.append_entry(
                f"[Defense] Finestra aperta per {data.get('defender_id')} "
                f"(danno in arrivo {data.get('incoming_damage', 0)})"
            )
        elif type_id == "dungeon.attack.resolved":
            if data.get("cancelled"):
                self.append_entry(
                    f"[Attack] Annullato da {data.get('defender_id')} (0 danni)"
                )
            else:
                self.append_entry(
                    f"[Attack] {data.get('defender_id')} subisce "
                    f"{data.get('final_damage', 0)} danni (HP {data.get('hp_after')})"
                )
        elif type_id == "dungeon.action.rejected":
            self.append_entry(f"[Rejected] {data.get('command')}: {data.get('reason')}")
        elif type_id == "dungeon.game.over":
            self.append_entry(f"[Game Over] Outcome: {data.get('outcome')}")

    def append_entry(self, text: str) -> None:
        """Appends a formatted text entry to the log."""
        self._list.addItem(text)
        self._list.scrollToBottom()

    def clear(self) -> None:
        """Removes all log entries."""
        self._list.clear()
