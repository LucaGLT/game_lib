"""SequenceStateWidget — compact sequence-state indicator for one actor.

Shows whether an actor is currently in a card sequence, how many cards have
been played, and which card types are valid as the next play.

Data format (from ``gmalea.sequence.state_changed``)::

    {
        "actor_id":    "pg_1",
        "active":      true,
        "last_type":   "SEQ_START",
        "cards_played": 1,
        "valid_next":  ["SEQ_CONTINUE", "SEQ_END", "INSTANT"]
    }

Visual states (all via QSS ``sequence_state`` property — no hardcoded colours):

- ``inactive`` — grey / muted when ``active == false``
- ``active``   — accent-coloured when a sequence is in progress
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QVBoxLayout,
    QWidget,
)

# Human-readable labels for each CardType value.
_CARD_TYPE_LABELS: dict[str, str] = {
    "SINGLE":       "Singola",
    "SEQ_START":    "Inizio Seq.",
    "SEQ_CONTINUE": "Continua Seq.",
    "SEQ_END":      "Fine Seq.",
    "INSTANT":      "Istantanea",
}


class SequenceStateWidget(QWidget):
    """Displays the current card-sequence state for one actor.

    Call :meth:`update_state` to refresh the display.
    The widget is read-only; it emits no signals.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._build_layout()

    # ── Layout construction ────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        root = QVBoxLayout()
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(4)

        title = QLabel("Sequenza")
        title.setProperty("text_role", "subtitle")
        root.addWidget(title)

        # State summary line.
        self._state_label = QLabel("Nessuna sequenza")
        self._state_label.setProperty("text_role", "body")
        self._state_label.setProperty("sequence_state", "inactive")
        root.addWidget(self._state_label)

        # Row of valid-next badge chips.
        self._badges_container = QFrame()
        badges_layout = QHBoxLayout(self._badges_container)
        badges_layout.setContentsMargins(0, 0, 0, 0)
        badges_layout.setSpacing(4)
        badges_layout.setAlignment(Qt.AlignmentFlag.AlignLeft)
        self._badges_container.setLayout(badges_layout)
        root.addWidget(self._badges_container)

        root.addStretch()
        self.setLayout(root)

    # ── Public API ─────────────────────────────────────────────────────────────

    def update_state(self, data: dict) -> None:
        """Refreshes the widget from a sequence state snapshot.

        Args:
            data: Dict with keys ``actor_id`` (str), ``active`` (bool),
                  ``last_type`` (str), ``cards_played`` (int),
                  ``valid_next`` (list[str]).
        """
        active:       bool      = bool(data.get("active", False))
        cards_played: int       = int(data.get("cards_played", 0))
        last_type:    str       = str(data.get("last_type", ""))
        valid_next:   list[str] = list(data.get("valid_next", []))

        # Update summary label text and visual state property.
        if active:
            type_label = _CARD_TYPE_LABELS.get(last_type, last_type)
            self._state_label.setText(
                f"Sequenza aperta (carta {cards_played}) — {type_label}"
            )
            self._state_label.setProperty("sequence_state", "active")
            self._state_label.setProperty("tone", "accent")
        else:
            self._state_label.setText("Nessuna sequenza")
            self._state_label.setProperty("sequence_state", "inactive")
            self._state_label.setProperty("tone", "muted")

        # Force QSS re-evaluation after property change.
        self._state_label.style().unpolish(self._state_label)
        self._state_label.style().polish(self._state_label)
        self._state_label.update()

        # Rebuild valid-next badges.
        self._clear_badges()
        for card_type in valid_next:
            badge = _CardTypeBadge(card_type, active=active)
            self._badges_container.layout().addWidget(badge)

    # ── Internal helpers ───────────────────────────────────────────────────────

    def _clear_badges(self) -> None:
        """Removes all badge widgets from the badges container."""
        layout = self._badges_container.layout()
        while layout.count():
            item = layout.takeAt(0)
            if item and item.widget():
                item.widget().deleteLater()


class _CardTypeBadge(QLabel):
    """Chip badge showing one valid CardType name."""

    def __init__(self, card_type: str, active: bool = True,
                 parent: QWidget | None = None) -> None:
        label = _CARD_TYPE_LABELS.get(card_type, card_type)
        super().__init__(label, parent)
        self.setProperty("chip", "true")
        # Use accent tone when sequence is active, muted when inactive.
        self.setProperty("tone", "accent" if active else "muted")
        self.setToolTip(card_type)
