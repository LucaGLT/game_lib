"""Le Pergamene di Eldhôm — action panel widget.

ActionPanelWidget provides buttons for all four Azioni Semplici:
  Movimento (1⌛), Attacco (2⌛), Interazione (3⌛), Recupero (3⌛).
A stop-sequence button becomes visible when a sequence is active.

All visual styling is applied exclusively through QSS — no hardcoded
color values are present in this module.
"""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QWidget,
)


class ActionPanelWidget(QFrame):
    """Row of simple-action buttons for the active hero.

    Signals:
        action_move(str):    Emitted with destination location_id.
        action_attack():     Emitted when Attack is pressed.
        action_recover():    Emitted when Recover is pressed.
        stop_sequence():     Emitted when Stop Sequence is pressed.
    """

    action_move     = Signal(str)
    action_attack   = Signal()
    action_interact = Signal()
    action_recover  = Signal()
    stop_sequence   = Signal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        layout = QHBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(8, 4, 8, 4)

        self._turn_label = QLabel("In attesa\u2026", self)
        self._turn_label.setProperty("text_role", "secondary")
        layout.addWidget(self._turn_label)

        layout.addStretch()

        self._move_combo = QComboBox(self)
        self._move_combo.setFixedWidth(120)
        layout.addWidget(self._move_combo)

        self._move_btn = QPushButton("\u25b6 Muovi 1\u23f3", self)
        self._move_btn.setProperty("role", "secondary")
        self._move_btn.clicked.connect(self._on_move_clicked)
        layout.addWidget(self._move_btn)

        self._attack_btn = QPushButton("\u2694 Attacca 2\u23f3", self)
        self._attack_btn.setProperty("role", "secondary")
        self._attack_btn.clicked.connect(self.action_attack)
        layout.addWidget(self._attack_btn)

        self._interact_btn = QPushButton("\u23fa Interagisci 3\u23f3", self)
        self._interact_btn.setProperty("role", "secondary")
        self._interact_btn.clicked.connect(self.action_interact)
        layout.addWidget(self._interact_btn)

        self._recover_btn = QPushButton("\u267b Recupera 3\u23f3", self)
        self._recover_btn.setProperty("role", "secondary")
        self._recover_btn.clicked.connect(self.action_recover)
        layout.addWidget(self._recover_btn)

        self._stop_btn = QPushButton("\u25a0 Stop seq.", self)
        self._stop_btn.setProperty("role", "danger")
        self._stop_btn.clicked.connect(self.stop_sequence)
        self._stop_btn.setVisible(False)
        layout.addWidget(self._stop_btn)

        self._all_action_buttons = [
            self._move_btn,
            self._attack_btn,
            self._interact_btn,
            self._recover_btn,
        ]
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.set_enabled(False)

    def set_turn(
        self,
        hero_name: str,
        adjacent_locations: list[str],
        sequence_active: bool = False,
    ) -> None:
        """Updates the panel for the given hero's turn.

        Args:
            hero_name:          Name of the active hero.
            adjacent_locations: Location IDs reachable from current location.
            sequence_active:    Show/hide the Stop Sequence button.
        """
        self._turn_label.setText(f"TURNO: {hero_name}")
        self._move_combo.clear()
        for loc in adjacent_locations:
            self._move_combo.addItem(loc, loc)
        self._stop_btn.setVisible(sequence_active)
        self.set_enabled(True)

    def set_sequence_active(self, active: bool) -> None:
        """Shows or hides the Stop Sequence button.

        Args:
            active: True when a card sequence is currently open.
        """
        self._stop_btn.setVisible(active)

    def set_enabled(self, enabled: bool) -> None:
        """Enables or disables all action buttons uniformly.

        Args:
            enabled: True to enable all action buttons.
        """
        for btn in self._all_action_buttons:
            btn.setEnabled(enabled)
        self._move_combo.setEnabled(enabled)
        self._stop_btn.setEnabled(enabled)
        if not enabled:
            self._turn_label.setText("In attesa\u2026")

    def _on_move_clicked(self) -> None:
        """Emits action_move with the selected destination."""
        destination = self._move_combo.currentData()
        if destination:
            self.action_move.emit(destination)
