"""Le Pergamene di Eldhôm — hand widget.

HandWidget displays the current hand of a single hero as a row of
clickable card buttons.  Emits a ``card_selected`` signal when a button
is pressed.

All visual styling is applied exclusively through QSS — no hardcoded
color values are present in this module.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt, Signal


_CARD_TYPE_ICONS: dict[str, str] = {
    "SINGLE":       "\u25c6",
    "SEQ_START":    "\u25b6",
    "SEQ_CONTINUE": "\u25b7",
    "SEQ_END":      "\u25c4",
    "INSTANT":      "\u26a1",
}


class HandWidget(QFrame):
    """Row of card buttons representing the active hero's current hand.

    Signals:
        card_selected (str): Emitted with the card_id when a card button is clicked.
    """

    card_selected = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        main_layout = QVBoxLayout(self)
        main_layout.setSpacing(4)
        main_layout.setContentsMargins(4, 4, 4, 4)

        self._title = QLabel("Mano: \u2014", self)
        self._title.setProperty("text_role", "secondary")
        main_layout.addWidget(self._title)

        card_row = QWidget(self)
        self._card_layout = QHBoxLayout(card_row)
        self._card_layout.setSpacing(8)
        self._card_layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        self._card_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(card_row)

        self._buttons: list[QPushButton] = []
        self._enabled = False

        self.setFrameShape(QFrame.Shape.StyledPanel)

    def set_hand(
        self,
        hero_name: str,
        cards: list[dict],
        enabled: bool = True,
    ) -> None:
        """Replaces the displayed hand.

        Args:
            hero_name: Name of the active hero (for the title label).
            cards:     List of dicts with at least ``card_id`` and ``name``;
                       optionally ``card_type`` for the type icon.
            enabled:   Whether the card buttons should be clickable.
        """
        self._enabled = enabled

        # Clear old buttons
        for btn in self._buttons:
            btn.deleteLater()
        self._buttons.clear()

        deck_hint = f"  ({len(cards)} carte)" if cards else "  (mano vuota)"
        self._title.setText(f"Mano \u2014 {hero_name}{deck_hint}")

        for card in cards:
            card_id   = card.get("card_id", card.get("id", ""))
            name      = card.get("name", card_id)
            card_type = card.get("card_type", card.get("type", "SINGLE"))
            cost      = card.get("timeline_cost", card.get("cost", ""))
            icon      = _CARD_TYPE_ICONS.get(card_type, "\u25c6")

            label = f"{icon} {name}"
            if cost:
                label += f"\n\u231b{cost}"
            btn = QPushButton(label, self)
            btn.setProperty("role", "card_hand")
            btn.setEnabled(enabled)
            btn.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
            btn.clicked.connect(
                lambda _checked=False, cid=card_id: self.card_selected.emit(cid)
            )
            self._card_layout.addWidget(btn)
            self._buttons.append(btn)

    def set_enabled(self, enabled: bool) -> None:
        """Enables or disables all card buttons without clearing the hand."""
        self._enabled = enabled
        for btn in self._buttons:
            btn.setEnabled(enabled)

    def clear(self) -> None:
        """Clears the hand display."""
        self.set_hand("—", [], enabled=False)
