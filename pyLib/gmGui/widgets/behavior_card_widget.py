"""BehaviorCardWidget — displays the active behavior card of a monster group.

Shows the card label, a step-by-step list with per-step state indicators
(done / active / pending), and an optional reaction badge.

The widget empties itself (shows "Nessuna carta attiva") when called with
an empty ``card_id``, which signals that the card was discarded.

Data format (from ``gmactor.behavior.card_changed``)::

    {
        "group_id":         "goblin_a",
        "card_id":          "bc_goblin_charge",
        "card_label":       "Carica",
        "steps": [
            {"index": 0, "label": "Muovi 2",     "cost": 1, "state": "done"},
            {"index": 1, "label": "Attacca 2",   "cost": 2, "state": "active"},
            {"index": 2, "label": "Indietreggia","cost": 1, "state": "pending"}
        ],
        "has_reaction":      true,
        "reaction_trigger":  "gmflow.hero.played_card"
    }

Step ``state`` values: ``"done"`` | ``"active"`` | ``"pending"``.

Visual state is driven entirely through QSS ``step_state`` property on each
row and ``tone`` property on badge labels — no hardcoded colours.
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

# Step state → QSS tone mapping (drives QSS rules, not inline colour).
_STEP_TONE: dict[str, str] = {
    "done":    "muted",
    "active":  "accent",
    "pending": "neutral",
}

# Step state → leading indicator symbol.
_STEP_SYMBOL: dict[str, str] = {
    "done":    "✓",
    "active":  "▶",
    "pending": "○",
}

# Reaction badge symbol.
_REACTION_SYMBOL: str = "⚡"


class BehaviorCardWidget(QWidget):
    """Shows the active behavior card for a monster group.

    Call :meth:`update_state` to refresh the display.
    Pass a data dict with an empty ``card_id`` to clear the widget.
    The widget is read-only; it emits no signals.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._build_layout()

    # ── Layout construction ────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        root = QVBoxLayout()
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(8)

        section_title = QLabel("Carta Comportamento")
        section_title.setProperty("text_role", "subtitle")
        root.addWidget(section_title)

        # Card header: label + group id.
        header_row = QHBoxLayout()
        header_row.setContentsMargins(0, 0, 0, 0)
        header_row.setSpacing(8)

        self._card_label = QLabel("Nessuna carta attiva")
        self._card_label.setProperty("text_role", "body")
        self._card_label.setProperty("weight", "bold")
        header_row.addWidget(self._card_label)

        self._group_label = QLabel("")
        self._group_label.setProperty("text_role", "secondary")
        self._group_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        header_row.addWidget(self._group_label, stretch=1)

        root.addLayout(header_row)

        # Step list container.
        self._steps_frame = QFrame()
        steps_layout = QVBoxLayout(self._steps_frame)
        steps_layout.setContentsMargins(0, 0, 0, 0)
        steps_layout.setSpacing(4)
        self._steps_frame.setLayout(steps_layout)
        root.addWidget(self._steps_frame)

        # Reaction row (hidden when no reaction).
        self._reaction_row = QFrame()
        reaction_layout = QHBoxLayout(self._reaction_row)
        reaction_layout.setContentsMargins(0, 0, 0, 0)
        reaction_layout.setSpacing(4)

        self._reaction_badge = QLabel(_REACTION_SYMBOL + " Reazione")
        self._reaction_badge.setProperty("chip", "true")
        self._reaction_badge.setProperty("tone", "warning")
        reaction_layout.addWidget(self._reaction_badge)

        self._reaction_trigger_label = QLabel("")
        self._reaction_trigger_label.setProperty("text_role", "secondary")
        reaction_layout.addWidget(self._reaction_trigger_label, stretch=1)

        self._reaction_row.setLayout(reaction_layout)
        self._reaction_row.setVisible(False)
        root.addWidget(self._reaction_row)

        root.addStretch()
        self.setLayout(root)

    # ── Public API ─────────────────────────────────────────────────────────────

    def update_state(self, data: dict) -> None:
        """Refreshes the widget from a behavior card state snapshot.

        Args:
            data: Dict with keys ``group_id``, ``card_id``, ``card_label``,
                  ``steps``, ``has_reaction``, ``reaction_trigger``.
                  Pass ``card_id = ""`` to clear the widget.
        """
        card_id:          str       = str(data.get("card_id", ""))
        card_label:       str       = str(data.get("card_label", ""))
        group_id:         str       = str(data.get("group_id", ""))
        steps:            list[dict] = list(data.get("steps", []))
        has_reaction:     bool      = bool(data.get("has_reaction", False))
        reaction_trigger: str       = str(data.get("reaction_trigger", ""))

        # Update header.
        if card_id:
            self._card_label.setText(card_label or card_id)
        else:
            self._card_label.setText("Nessuna carta attiva")

        self._group_label.setText(group_id)

        # Rebuild step rows.
        self._clear_steps()
        for step_data in steps:
            row = _StepRow(step_data)
            self._steps_frame.layout().addWidget(row)

        # Show/hide reaction row.
        self._reaction_row.setVisible(has_reaction and bool(card_id))
        if has_reaction and reaction_trigger:
            self._reaction_trigger_label.setText(reaction_trigger)
        else:
            self._reaction_trigger_label.setText("")

    # ── Internal helpers ───────────────────────────────────────────────────────

    def _clear_steps(self) -> None:
        """Removes all step rows from the steps frame."""
        layout = self._steps_frame.layout()
        while layout.count():
            item = layout.takeAt(0)
            if item and item.widget():
                item.widget().deleteLater()


class _StepRow(QFrame):
    """One step row: state indicator + label + cost chip."""

    def __init__(self, step_data: dict, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        index: int = int(step_data.get("index", 0))
        label: str = str(step_data.get("label", f"Step {index}"))
        cost:  int = int(step_data.get("cost", 1))
        state: str = str(step_data.get("state", "pending"))

        tone   = _STEP_TONE.get(state, "neutral")
        symbol = _STEP_SYMBOL.get(state, "○")

        self.setProperty("step_state", state)

        row_layout = QHBoxLayout()
        row_layout.setContentsMargins(4, 4, 4, 4)
        row_layout.setSpacing(8)

        # State indicator symbol.
        indicator = QLabel(symbol)
        indicator.setProperty("text_role", "body")
        indicator.setProperty("tone", tone)
        indicator.setFixedWidth(16)
        row_layout.addWidget(indicator)

        # Step description label.
        desc = QLabel(label)
        desc.setProperty("text_role", "body")
        desc.setProperty("tone", tone)
        desc.setSizePolicy(
            desc.sizePolicy().horizontalPolicy(),
            desc.sizePolicy().verticalPolicy(),
        )
        row_layout.addWidget(desc, stretch=1)

        # Timeline-cost chip.
        cost_chip = QLabel(f"⌛{cost}")
        cost_chip.setProperty("chip", "true")
        cost_chip.setProperty("tone", tone)
        row_layout.addWidget(cost_chip)

        self.setLayout(row_layout)
