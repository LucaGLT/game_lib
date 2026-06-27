"""Le Pergamene di Eldhom — action panel widget.

ActionPanelWidget provides buttons for Azioni Semplici (Move, Attack,
Recover) plus a stop-sequence button when a sequence is active.

Emits signals that the main window connects to the bridge.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QWidget,
)
from PySide6.QtCore import Signal

_BTN_STYLE = (
    "QPushButton { background:#203020; border:1px solid #406040;"
    " border-radius:4px; color:#80d080; padding:6px 12px; font-size:11px; }"
    "QPushButton:hover { background:#304030; border-color:#60d060; }"
    "QPushButton:disabled { color:#555; border-color:#333; background:#1a1a1a; }"
)
_BTN_STOP_STYLE = (
    "QPushButton { background:#302010; border:1px solid #806030;"
    " border-radius:4px; color:#d09050; padding:6px 12px; font-size:11px; }"
    "QPushButton:hover { background:#403010; }"
    "QPushButton:disabled { color:#555; border-color:#333; background:#1a1a1a; }"
)
_COMBO_STYLE = (
    "QComboBox { background:#1e1e1e; border:1px solid #555; border-radius:3px;"
    " color:#ccc; padding:4px 8px; font-size:11px; }"
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
    action_recover  = Signal()
    stop_sequence   = Signal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        layout = QHBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(6, 4, 6, 4)

        self._turn_label = QLabel("In attesa…", self)
        self._turn_label.setStyleSheet("color:#aaa; font-size:12px; font-weight:bold;")
        layout.addWidget(self._turn_label)

        layout.addStretch()

        self._move_combo = QComboBox(self)
        self._move_combo.setStyleSheet(_COMBO_STYLE)
        self._move_combo.setFixedWidth(120)
        layout.addWidget(self._move_combo)

        self._move_btn = QPushButton("🚶 Muovi", self)
        self._move_btn.setStyleSheet(_BTN_STYLE)
        self._move_btn.clicked.connect(self._on_move_clicked)
        layout.addWidget(self._move_btn)

        self._attack_btn = QPushButton("⚔ Attacca", self)
        self._attack_btn.setStyleSheet(_BTN_STYLE)
        self._attack_btn.clicked.connect(self.action_attack)
        layout.addWidget(self._attack_btn)

        self._recover_btn = QPushButton("💊 Recupera", self)
        self._recover_btn.setStyleSheet(_BTN_STYLE)
        self._recover_btn.clicked.connect(self.action_recover)
        layout.addWidget(self._recover_btn)

        self._stop_btn = QPushButton("■ Stop seq.", self)
        self._stop_btn.setStyleSheet(_BTN_STOP_STYLE)
        self._stop_btn.clicked.connect(self.stop_sequence)
        self._stop_btn.setVisible(False)
        layout.addWidget(self._stop_btn)

        self._all_buttons = [
            self._move_btn, self._attack_btn,
            self._recover_btn, self._stop_btn,
        ]
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setStyleSheet("QFrame { background:#1c1c1c; border:none; }")
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

    def set_enabled(self, enabled: bool) -> None:
        """Enables or disables all action buttons."""
        for btn in self._all_buttons:
            btn.setEnabled(enabled)
        self._move_combo.setEnabled(enabled)
        if not enabled:
            self._turn_label.setText("In attesa…")

    def _on_move_clicked(self) -> None:
        """Emits action_move with the selected destination."""
        destination = self._move_combo.currentData()
        if destination:
            self.action_move.emit(destination)
