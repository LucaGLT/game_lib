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
    """

    move_requested:     Signal = Signal(str)
    heal_requested:     Signal = Signal(str, str)
    equip_requested:    Signal = Signal(str, str)
    end_turn_requested: Signal = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(6)

        self._lbl_actor    = QLabel("—")
        self._btn_move     = QPushButton("Muovi")
        self._btn_heal     = QPushButton("Pozione")
        self._btn_equip    = QPushButton("Equipaggia")
        self._lbl_actions  = QLabel("")
        self._btn_end_turn = QPushButton("Fine Turno")

        self._lbl_actor.setProperty("text_role", "secondary")
        self._lbl_actions.setProperty("text_role", "secondary")

        layout.addWidget(self._lbl_actor)
        layout.addWidget(self._btn_move)
        layout.addWidget(self._btn_heal)
        layout.addWidget(self._btn_equip)
        layout.addWidget(self._lbl_actions)
        layout.addStretch()
        layout.addWidget(self._btn_end_turn)

        for btn in (self._btn_move, self._btn_heal,
                    self._btn_equip, self._btn_end_turn):
            btn.setEnabled(False)

        self._btn_move.clicked.connect(self._on_move_clicked)
        self._btn_heal.clicked.connect(self._on_heal_clicked)
        self._btn_equip.clicked.connect(self._on_equip_clicked)
        self._btn_end_turn.clicked.connect(self._on_end_turn_clicked)

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

    # ── Public API ────────────────────────────────────────────────────────────

    def set_selected_actor(self, actor_id: str) -> None:
        """Switch display to *actor_id* without changing whose turn it is."""
        self._selected_actor_id = actor_id
        self._lbl_actor.setText(f"Selezionato: {actor_id}" if actor_id else "—")
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
            self._active_actor_id   = ""
            self._available_actions = []
            self._actions_remaining = 0
            self._lbl_actions.setText("")
            for btn in (self._btn_move, self._btn_heal,
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
        - ALWAYS show the selected actor's actions (Move, Heal, Equip).
        - Enable actions only when:  it is the selected actor's turn,
          the action is available, and actions_remaining > 0.
        - When actions_remaining reaches 0: all action buttons OFF,
          Fine Turno stays ON (so the player can end manually; C++ also
          auto-ends, but the button provides immediate feedback).
        - Fine Turno is ON only during a hero's active turn.
        """
        sel_id = self._selected_actor_id
        state  = self._actors_state.get(sel_id, {})
        avail  = self._available_actions   # valid only for _active_actor_id
        remaining = self._actions_remaining

        is_hero_sel    = state.get("kind", "") == "HERO"
        is_active_turn = self._is_selected_turn()
        can_act        = is_active_turn and remaining > 0

        # ── Action buttons: shown for any selected actor, enabled per rules ──
        self._btn_move.setEnabled(
            can_act and "move" in avail)
        self._btn_heal.setEnabled(
            can_act and "heal" in avail and state.get("has_potion", False))
        self._btn_equip.setEnabled(
            can_act and "equip" in avail
            and state.get("has_item", False)
            and not state.get("weapon_equipped", False))

        # ── Fine Turno: ON whenever a hero has the turn (any selection) ──────
        active_state = self._actors_state.get(self._active_actor_id, {})
        active_is_hero = active_state.get("kind", "") == "HERO"
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

    # ── Button callbacks ──────────────────────────────────────────────────────

    def _on_move_clicked(self) -> None:
        if self._active_actor_id:
            self.move_requested.emit(self._active_actor_id)
            self._consume_action()

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
        self._lbl_actor.setText("—")
        self._lbl_actions.setText("")
        for btn in (self._btn_move, self._btn_heal,
                    self._btn_equip, self._btn_end_turn):
            btn.setEnabled(False)


