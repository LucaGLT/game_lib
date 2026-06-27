"""Dungeon Crawler Basic — action panel widget.

Shows ALL actions of the **selected** actor (Move, Heal/Pozione, Equipaggia, …).
Actions are enabled only when:
  - the selected actor is the one whose turn it is, AND
  - the action is available (has_potion, etc.), AND
  - fewer than MAX_ACTIONS actions have been used this turn.

After MAX_ACTIONS actions the action buttons are disabled; only "Fine Turno"
remains enabled. "Fine Turno" is always available during a hero's turn.

The **selected** actor auto-follows the turn actor on TURN_STARTED; the hero
panel can override it via :meth:`set_selected_actor`.

All visual styling is applied exclusively through QSS.
"""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QWidget

_MAX_ACTIONS: int = 2


class ActionPanelWidget(QWidget):
    """Action palette driven by the selected actor's state.

    Signals:
        move_requested(hero_id):         Player pressed Muovi.
        heal_requested(hero_id, target): Player pressed Pozione.
        equip_requested(hero_id, item):  Player pressed Equipaggia.
        end_turn_requested(hero_id):     Player pressed Fine Turno.
        attack_requested(attacker_id):   Player pressed Attacca (target chosen next).
        defend_requested(defender_id, mode, block):
                                         Active defense choice (reduce / cancel).
        defend_pass_requested(defender_id):
                                         Player declined defense (passive stat only).
    """

    move_requested:            Signal = Signal(str)
    heal_requested:            Signal = Signal(str, str)
    equip_requested:           Signal = Signal(str, str)
    end_turn_requested:        Signal = Signal(str)
    attack_requested:          Signal = Signal(str)
    defend_requested:          Signal = Signal(str, str, int)
    defend_pass_requested:     Signal = Signal(str)
    actions_remaining_changed: Signal = Signal(int)  # emitted after every action consumed

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(6)

        self._lbl_actor    = QLabel("—")
        self._btn_move     = QPushButton("Muovi")
        self._btn_attack   = QPushButton("Attacca")
        self._btn_heal     = QPushButton("Pozione")
        self._btn_equip    = QPushButton("Equipaggia")
        self._lbl_actions  = QLabel("")
        self._btn_end_turn = QPushButton("Fine Turno")

        # Reactive-defense buttons (hidden outside a defense window).
        self._btn_defend   = QPushButton("Difendi")
        self._btn_cancel   = QPushButton("Annulla colpo")
        self._btn_pass     = QPushButton("Passa")

        self._lbl_actor.setProperty("text_role", "secondary")
        self._lbl_actions.setProperty("text_role", "secondary")

        layout.addWidget(self._lbl_actor)
        layout.addWidget(self._btn_move)
        layout.addWidget(self._btn_attack)
        layout.addWidget(self._btn_heal)
        layout.addWidget(self._btn_equip)
        layout.addWidget(self._btn_defend)
        layout.addWidget(self._btn_cancel)
        layout.addWidget(self._btn_pass)
        layout.addWidget(self._lbl_actions)
        layout.addStretch()
        layout.addWidget(self._btn_end_turn)

        for btn in (self._btn_move, self._btn_attack, self._btn_heal,
                    self._btn_equip, self._btn_end_turn):
            btn.setEnabled(False)
        for btn in (self._btn_defend, self._btn_cancel, self._btn_pass):
            btn.setVisible(False)

        self._btn_move.clicked.connect(self._on_move_clicked)
        self._btn_attack.clicked.connect(self._on_attack_clicked)
        self._btn_heal.clicked.connect(self._on_heal_clicked)
        self._btn_equip.clicked.connect(self._on_equip_clicked)
        self._btn_end_turn.clicked.connect(self._on_end_turn_clicked)
        self._btn_defend.clicked.connect(self._on_defend_clicked)
        self._btn_cancel.clicked.connect(self._on_cancel_clicked)
        self._btn_pass.clicked.connect(self._on_pass_clicked)

        # ── State ──────────────────────────────────────────────────────────────
        # Actor whose turn it is (server-authoritative).
        self._active_actor_id:   str = ""
        # Actor whose actions are shown (user-selectable; defaults to active).
        self._selected_actor_id: str = ""
        # Actions the active actor may execute this turn (from C++ payload).
        self._available_actions: list[str] = []
        # Local counter (mirrors C++ MAX_ACTIONS_PER_TURN, decrements on click).
        self._actions_remaining: int = 0
        # Per-actor data cache: actor_id → {has_potion, has_item,
        #                                    weapon_equipped, kind}
        self._actors_state: dict[str, dict] = {}
        # True while the player must select a destination for the Move action.
        self._awaiting_move: bool = False
        # True while the player must select an enemy target for the Attack action.
        self._awaiting_attack: bool = False
        # Defender id while a reactive-defense window is open ("" = no window).
        self._defending_id: str = ""

    # ── Public API ────────────────────────────────────────────────────────────

    def set_selected_actor(self, actor_id: str) -> None:
        """Switch display to *actor_id* without changing whose turn it is."""
        self._selected_actor_id = actor_id
        self._lbl_actor.setText(f"Selezionato: {actor_id}" if actor_id else "—")
        self._update_buttons()
    def set_awaiting_move(self, active: bool) -> None:
        """Enters or exits move-targeting mode (changes Muovi button text)."""
        self._awaiting_move = active
        self._btn_move.setText("Annulla Muovi" if active else "Muovi")

    def set_awaiting_attack(self, active: bool) -> None:
        """Enters or exits attack-targeting mode (changes Attacca button text)."""
        self._awaiting_attack = active
        self._btn_attack.setText("Annulla Attacco" if active else "Attacca")

    def is_defending(self) -> bool:
        """True while a reactive-defense window is open for the local player."""
        return bool(self._defending_id)

    def enter_defense_mode(self, defender_id: str, incoming_damage: int,
                           can_pass: bool, can_cancel: bool) -> None:
        """Switches the panel into reactive-defense mode for *defender_id*.

        Normal action buttons are hidden; the Difendi / Annulla colpo / Passa
        buttons are shown according to the options advertised by the engine.
        """
        self._defending_id = defender_id
        self._awaiting_move = False
        self._awaiting_attack = False
        self._btn_move.setText("Muovi")
        self._btn_attack.setText("Attacca")
        self._lbl_actor.setText(f"Difesa: {defender_id}")
        self._lbl_actions.setText(f"Danno in arrivo: {incoming_damage}")

        for btn in (self._btn_move, self._btn_attack, self._btn_heal,
                    self._btn_equip, self._btn_end_turn):
            btn.setVisible(False)

        self._btn_defend.setVisible(True)
        self._btn_defend.setEnabled(True)
        self._btn_pass.setVisible(bool(can_pass))
        self._btn_pass.setEnabled(bool(can_pass))
        self._btn_cancel.setVisible(bool(can_cancel))
        self._btn_cancel.setEnabled(bool(can_cancel))

    def exit_defense_mode(self) -> None:
        """Leaves reactive-defense mode and restores the normal action buttons."""
        self._defending_id = ""
        for btn in (self._btn_defend, self._btn_cancel, self._btn_pass):
            btn.setVisible(False)
            btn.setEnabled(False)
        self._btn_move.setVisible(True)
        self._btn_attack.setVisible(True)
        self._btn_end_turn.setVisible(True)
        self._update_buttons()

    # ── Envelope handler ─────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {}) or {}

        if tid == "dungeon.session.started":
            self.reset()

        elif tid == "dungeon.actor.snapshot":
            for actor in data.get("actors", []):
                actor_id = str(actor.get("id", ""))
                if not actor_id:
                    continue
                tags: list = actor.get("tags", [])
                self._actors_state[actor_id] = {
                    "kind": str(actor.get("kind", "")),
                    "has_potion":       "has_potion"         in tags,
                    "has_item":         "bigword_available"  in tags,
                    "weapon_equipped":  "equipped_weapon"    in tags,
                }
            self._update_buttons()

        elif tid == "dungeon.turn.started":
            actor_id = str(data.get("actor_id", ""))
            self._active_actor_id   = actor_id
            self._selected_actor_id = actor_id          # auto-follow
            self._available_actions = [str(a) for a in data.get("available_actions", [])]
            self._actions_remaining = int(data.get("actions_remaining", _MAX_ACTIONS))
            self._lbl_actor.setText(f"Selezionato: {actor_id}" if actor_id else "—")
            self._update_buttons()

        elif tid in ("dungeon.turn.ended", "dungeon.game.over"):
            if self._defending_id:
                # A defense window overrides turn lifecycle; keep it open.
                return
            self._active_actor_id   = ""
            self._available_actions = []
            self._actions_remaining = 0
            self._lbl_actions.setText("")
            for btn in (self._btn_move, self._btn_attack, self._btn_heal,
                        self._btn_equip, self._btn_end_turn):
                btn.setEnabled(False)

    # ── Internal helpers ─────────────────────────────────────────────────────

    def _is_selected_turn(self) -> bool:
        """True when the selected actor is the one currently having their turn."""
        return bool(self._active_actor_id) and \
               self._selected_actor_id == self._active_actor_id

    def _update_buttons(self) -> None:
        """Re-evaluates every button state from current cache.

        Rules:
        - HIDE action buttons the selected actor does not have available
          (no potion → Heal hidden; no item / already equipped → Equip hidden).
        - SHOW but DISABLE actions when it is not the selected actor's turn
          or actions_remaining == 0.
        - Fine Turno is always visible; enabled only during a hero's active turn.
        """
        if self._defending_id:
            # While a defense window is open the panel is driven by
            # enter_defense_mode()/exit_defense_mode(); skip normal evaluation.
            return

        sel_id    = self._selected_actor_id
        state     = self._actors_state.get(sel_id, {})
        remaining = self._actions_remaining

        is_hero        = state.get("kind", "") == "HERO"
        is_active_turn = self._is_selected_turn()
        can_act        = is_active_turn and remaining > 0

        # ── Visibility: based on what the selected actor HAS ──────────────────
        move_visible  = is_hero
        heal_visible  = is_hero and state.get("has_potion", False)
        equip_visible = (is_hero
                         and state.get("has_item", False)
                         and not state.get("weapon_equipped", False))

        self._btn_move.setVisible(move_visible)
        self._btn_attack.setVisible(move_visible)
        self._btn_heal.setVisible(heal_visible)
        self._btn_equip.setVisible(equip_visible)

        # ── Enable: only when it is the selected actor's turn and can act ─────
        self._btn_move.setEnabled(can_act and move_visible)
        self._btn_attack.setEnabled(can_act and move_visible)
        self._btn_heal.setEnabled(can_act and heal_visible)
        self._btn_equip.setEnabled(can_act and equip_visible)

        # ── Fine Turno: always visible; ON during any hero's active turn ──────
        active_state   = self._actors_state.get(self._active_actor_id, {})
        active_is_hero = active_state.get("kind", "") == "HERO"
        self._btn_end_turn.setVisible(True)
        self._btn_end_turn.setEnabled(
            bool(self._active_actor_id) and active_is_hero)

        # ── Counter label ─────────────────────────────────────────────────────
        if self._active_actor_id and active_is_hero:
            if remaining > 0:
                self._lbl_actions.setText(f"Azioni: {remaining}/{_MAX_ACTIONS}")
            else:
                self._lbl_actions.setText("Azioni esaurite")
        else:
            self._lbl_actions.setText("")

    def _consume_action(self) -> None:
        if self._actions_remaining > 0:
            self._actions_remaining -= 1
        self._update_buttons()
        self.actions_remaining_changed.emit(self._actions_remaining)

    def consume_actions(self, cost: int) -> None:
        """Decrements the action counter by cost (called when a card is played)."""
        self._actions_remaining = max(0, self._actions_remaining - cost)
        self._update_buttons()
        self.actions_remaining_changed.emit(self._actions_remaining)

    # ── Button callbacks ──────────────────────────────────────────────────────

    def _on_move_clicked(self) -> None:
        if self._active_actor_id:
            self.move_requested.emit(self._active_actor_id)
            self._consume_action()

    def _on_attack_clicked(self) -> None:
        if self._active_actor_id:
            self.attack_requested.emit(self._active_actor_id)

    def mark_action_consumed(self) -> None:
        """Decrements the local action counter (called once an attack is sent)."""
        self._consume_action()

    def _on_defend_clicked(self) -> None:
        if self._defending_id:
            self.defend_requested.emit(self._defending_id, "reduce", 0)
            self._disable_defense_buttons()

    def _on_cancel_clicked(self) -> None:
        if self._defending_id:
            self.defend_requested.emit(self._defending_id, "cancel", 0)
            self._disable_defense_buttons()

    def _on_pass_clicked(self) -> None:
        if self._defending_id:
            self.defend_pass_requested.emit(self._defending_id)
            self._disable_defense_buttons()

    def _disable_defense_buttons(self) -> None:
        """Disables the defense buttons after a choice to prevent double submit."""
        for btn in (self._btn_defend, self._btn_cancel, self._btn_pass):
            btn.setEnabled(False)

    def _on_heal_clicked(self) -> None:
        if self._active_actor_id:
            self.heal_requested.emit(self._active_actor_id, self._active_actor_id)
            self._consume_action()

    def _on_equip_clicked(self) -> None:
        if self._active_actor_id:
            self.equip_requested.emit(self._active_actor_id, "bigword_available")
            self._consume_action()

    def _on_end_turn_clicked(self) -> None:
        if self._active_actor_id:
            self.end_turn_requested.emit(self._active_actor_id)

    # ── Reset ─────────────────────────────────────────────────────────────────

    def reset(self) -> None:
        self._active_actor_id   = ""
        self._selected_actor_id = ""
        self._available_actions = []
        self._actions_remaining = 0
        self._actors_state.clear()
        self._awaiting_move = False
        self._awaiting_attack = False
        self._defending_id = ""
        self._btn_move.setText("Muovi")
        self._btn_attack.setText("Attacca")
        for btn in (self._btn_defend, self._btn_cancel, self._btn_pass):
            btn.setVisible(False)
            btn.setEnabled(False)
        self._lbl_actor.setText("\u2014")
        self._lbl_actions.setText("")
        self._update_buttons()


