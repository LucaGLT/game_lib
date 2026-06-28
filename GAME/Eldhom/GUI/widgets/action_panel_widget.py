"""Le Pergamene di Eldhôm — action panel widget.

ActionPanelWidget provides buttons for all four Azioni Semplici:
  Movimento (1⌛), Attacco (2⌛), Interazione (3⌛), Recupero (3⌛).
A stop-sequence button becomes visible when a sequence is active.

Movement and attack both use a *targeting mode* (Dungeon-Crawler style):
pressing "Muovi" / "Attacca" arms the panel and the player then clicks the
target (a location for move, an enemy actor for attack).  While armed the
button label changes to its "Annulla" variant.

When the engine opens a reaction window the panel switches to *defense mode*:
the action buttons are hidden and one button per allowed reaction
(Subisci / Para / Schiva) is shown.  The chosen reaction is only a selection —
the engine decides and applies every effect.

All visual styling is applied exclusively through QSS — no hardcoded
color values are present in this module.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QWidget,
)


class ActionPanelWidget(QFrame):
    """Row of simple-action buttons for the active hero.

    Signals:
        move_armed(bool):    Emitted when move targeting is armed/disarmed.
        attack_armed(bool):  Emitted when attack targeting is armed/disarmed.
        action_interact():   Emitted when Interact is pressed.
        action_recover():    Emitted when Recover is pressed.
        stop_sequence():     Emitted when Stop Sequence is pressed.
        react_chosen(str):   Emitted with a reaction code (TAKE/BLOCK/DODGE).
    """

    move_armed      = Signal(bool)
    attack_armed    = Signal(bool)
    action_interact = Signal()
    action_recover  = Signal()
    stop_sequence   = Signal()
    react_chosen    = Signal(str)

    _LABEL_MOVE: str          = "\u25b6 Muovi 1\u23f3"
    _LABEL_MOVE_CANCEL: str   = "\u2715 Annulla Muovi"
    _LABEL_ATTACK: str        = "\u2694 Attacca 2\u23f3"
    _LABEL_ATTACK_CANCEL: str = "\u2715 Annulla Attacco"

    _REACTION_LABELS: dict[str, str] = {
        "TAKE":  "\u2620 Subisci",
        "BLOCK": "\u26e8 Para",
        "DODGE": "\u21ba Schiva",
    }

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        self._move_active: bool   = False
        self._attack_active: bool = False
        self._defending_id: str   = ""

        layout = QHBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(8, 4, 8, 4)

        self._turn_label = QLabel("In attesa\u2026", self)
        self._turn_label.setProperty("text_role", "secondary")
        layout.addWidget(self._turn_label)

        layout.addStretch(1)

        # Centre area: instruction shown during targeting or defense mode.
        self._hint_label = QLabel("", self)
        self._hint_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._hint_label.setProperty("text_role", "primary")
        self._hint_label.setVisible(False)
        layout.addWidget(self._hint_label)

        layout.addStretch(1)

        self._move_btn = QPushButton(self._LABEL_MOVE, self)
        self._move_btn.setProperty("role", "secondary")
        self._move_btn.clicked.connect(self._on_move_clicked)
        layout.addWidget(self._move_btn)

        self._attack_btn = QPushButton(self._LABEL_ATTACK, self)
        self._attack_btn.setProperty("role", "secondary")
        self._attack_btn.clicked.connect(self._on_attack_clicked)
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

        # Defense-mode buttons (one per reaction kind, hidden by default).
        self._react_btns: dict[str, QPushButton] = {}
        for code, label in self._REACTION_LABELS.items():
            btn = QPushButton(label, self)
            btn.setProperty("role", "primary")
            btn.clicked.connect(lambda _=False, c=code: self._on_react_clicked(c))
            btn.setVisible(False)
            layout.addWidget(btn)
            self._react_btns[code] = btn

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
        sequence_active: bool = False,
    ) -> None:
        """Updates the panel for the given hero's turn.

        Args:
            hero_name:       Name of the active hero.
            sequence_active: Show/hide the Stop Sequence button.
        """
        self._turn_label.setText(f"TURNO: {hero_name}")
        self._stop_btn.setVisible(sequence_active)
        self.disarm_move()
        self.disarm_attack()
        self.set_hint("")
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
        self._stop_btn.setEnabled(enabled)
        if not enabled:
            self._turn_label.setText("In attesa\u2026")
            self.disarm_move()
            self.disarm_attack()
            self.set_hint("")

    def set_hint(self, text: str) -> None:
        """Shows an instruction in the panel centre; clears when text is empty.

        Args:
            text: Instruction string (e.g. "\u25b6 Clicca la locazione…").
                  Pass an empty string to hide the label.
        """
        self._hint_label.setText(text)
        self._hint_label.setVisible(bool(text))

    def disarm_move(self) -> None:
        """Cancels move targeting mode and restores the button label."""
        if self._move_active:
            self._move_active = False
            self._move_btn.setText(self._LABEL_MOVE)
            self.move_armed.emit(False)

    def disarm_attack(self) -> None:
        """Cancels attack targeting mode and restores the button label."""
        if self._attack_active:
            self._attack_active = False
            self._attack_btn.setText(self._LABEL_ATTACK)
            self.attack_armed.emit(False)

    def _on_move_clicked(self) -> None:
        """Toggles move targeting mode and notifies listeners."""
        self.disarm_attack()
        self._move_active = not self._move_active
        if self._move_active:
            self._move_btn.setText(self._LABEL_MOVE_CANCEL)
        else:
            self._move_btn.setText(self._LABEL_MOVE)
        self.move_armed.emit(self._move_active)

    def _on_attack_clicked(self) -> None:
        """Toggles attack targeting mode and notifies listeners."""
        self.disarm_move()
        self._attack_active = not self._attack_active
        if self._attack_active:
            self._attack_btn.setText(self._LABEL_ATTACK_CANCEL)
        else:
            self._attack_btn.setText(self._LABEL_ATTACK)
        self.attack_armed.emit(self._attack_active)

    # ── Defense mode (reaction window) ──────────────────────────────────────────

    def enter_defense_mode(
        self,
        defender_name: str,
        incoming_damage: int,
        reactions: list[str],
    ) -> None:
        """Switches the panel to defense mode for the reaction window.

        Args:
            defender_name:   Display name of the reacting defender.
            incoming_damage: Declared (pre-reaction) damage.
            reactions:       Allowed reaction codes (subset of TAKE/BLOCK/DODGE).
        """
        self.disarm_move()
        self.disarm_attack()
        self._turn_label.setText(
            f"DIFESA: {defender_name}  \u2022  danno in arrivo {incoming_damage}\u274c"
        )
        for btn in self._all_action_buttons:
            btn.setVisible(False)
        self._stop_btn.setVisible(False)
        allowed = set(reactions)
        for code, btn in self._react_btns.items():
            visible = code in allowed
            btn.setVisible(visible)
            btn.setEnabled(visible)

    def exit_defense_mode(self) -> None:
        """Leaves defense mode and restores the normal action buttons."""
        for btn in self._react_btns.values():
            btn.setVisible(False)
            btn.setEnabled(False)
        for btn in self._all_action_buttons:
            btn.setVisible(True)
        self.set_hint("")

    def _on_react_clicked(self, code: str) -> None:
        """Emits the chosen reaction code and disables the defense buttons."""
        for btn in self._react_btns.values():
            btn.setEnabled(False)
        self.react_chosen.emit(code)
