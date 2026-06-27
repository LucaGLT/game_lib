"""Le Pergamene di Eldhom — hand widget.

HandWidget displays the current hand of a single hero as a row of
clickable card buttons. Emits a ``card_selected`` signal when a button
is pressed.
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


_CARD_STYLE = (
    "QPushButton {"
    "  background:#2d2510; border:1px solid #7a5a20; border-radius:5px;"
    "  color:#d4b07a; padding:6px 8px; font-size:11px; min-width:90px; max-width:120px;"
    "}"
    "QPushButton:hover { background:#3d3010; border-color:#c8a060; }"
    "QPushButton:pressed { background:#1a1508; }"
    "QPushButton:disabled { color:#555; border-color:#333; background:#1a1a1a; }"
)

_CARD_TYPE_ICONS: dict[str, str] = {
    "SINGLE":       "◆",
    "SEQ_START":    "▶",
    "SEQ_CONTINUE": "▷",
    "SEQ_END":      "◀",
    "INSTANT":      "⚡",
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
        main_layout.setSpacing(2)
        main_layout.setContentsMargins(4, 4, 4, 4)

        self._title = QLabel("Mano: —", self)
        self._title.setStyleSheet("color:#888; font-size:11px;")
        main_layout.addWidget(self._title)

        card_row = QWidget(self)
        self._card_layout = QHBoxLayout(card_row)
        self._card_layout.setSpacing(6)
        self._card_layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        self._card_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(card_row)

        self._buttons: list[QPushButton] = []
        self._enabled = False

        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setStyleSheet("QFrame { background:#181818; border:none; }")

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
        self._title.setText(f"Mano — {hero_name}{deck_hint}")

        for card in cards:
            card_id   = card.get("card_id", "")
            name      = card.get("name", card_id)
            card_type = card.get("card_type", "SINGLE")
            cost      = card.get("timeline_cost", "?")
            icon      = _CARD_TYPE_ICONS.get(card_type, "◆")

            btn = QPushButton(f"{icon} {name}\n⌛{cost}", self)
            btn.setStyleSheet(_CARD_STYLE)
            btn.setEnabled(enabled)
            btn.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)

            btn.clicked.connect(lambda _checked=False, cid=card_id: self.card_selected.emit(cid))

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
