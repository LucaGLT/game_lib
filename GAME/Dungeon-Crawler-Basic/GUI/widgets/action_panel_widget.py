"""Dungeon Crawler Basic — action panel widget (v1: Move, Heal, Equip).

ActionPanelWidget presents the three v1 gameplay actions available to the
current hero: Move (handled via board click), Heal and Equip. It reads action
availability from the actor state and disables unavailable actions accordingly.
When the player confirms an action the widget emits the appropriate signal
which is forwarded to DungeonBridge.send_command().

All visual styling is applied exclusively through QSS.
Attack and Defend actions are intentionally absent from this widget (v1 scope).
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Signal


class ActionPanelWidget(QWidget):
    """Action palette for v1 hero actions: Heal and Equip.

    Move is triggered from DungeonBoardWidget rather than here.

    Signals:
        heal_requested(hero_id, target_id): Player pressed the Heal button.
        equip_requested(hero_id, item_tag): Player pressed the Equip button.
    """

    heal_requested:  Signal = Signal(str, str)
    equip_requested: Signal = Signal(str, str)

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the panel with all action buttons disabled."""
        super().__init__(parent)
        from PySide6.QtWidgets import QHBoxLayout, QPushButton
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        self._btn_heal  = QPushButton("Heal (potion)")
        self._btn_equip = QPushButton("Equip weapon")
        layout.addWidget(self._btn_heal)
        layout.addWidget(self._btn_equip)
        layout.addStretch()
        self._btn_heal.setEnabled(False)
        self._btn_equip.setEnabled(False)
        self._btn_heal.clicked.connect(self._on_heal_clicked)
        self._btn_equip.clicked.connect(self._on_equip_clicked)
        self._hero_id: str = ""
        self._has_potion: bool = False
        self._has_item: bool = False
        self._weapon_equipped: bool = False

    def on_envelope(self, msg: dict) -> None:
        """Receives a decoded engine event and updates button availability."""
        type_id = msg.get("typeId", "")
        data = msg.get("data", {})
        if type_id == "dungeon.actor.snapshot":
            for actor in data.get("actors", []):
                if actor.get("kind") == "HERO":
                    self._hero_id = actor.get("id", "")
                    tags = actor.get("tags", [])
                    self._has_potion = "has_potion" in tags
                    self._has_item = "bigword_available" in tags
                    self._weapon_equipped = "equipped_weapon" in tags
                    self._update_buttons()
        elif type_id == "dungeon.turn.started":
            actor_id = data.get("actor_id", "")
            self._set_actions_enabled(actor_id == self._hero_id)
        elif type_id in ("dungeon.turn.ended", "dungeon.game.over"):
            self._set_actions_enabled(False)
        elif type_id == "dungeon.session.started":
            self.reset()

    def _update_buttons(self) -> None:
        self._btn_heal.setEnabled(self._has_potion)
        self._btn_equip.setEnabled(self._has_item and not self._weapon_equipped)

    def _set_actions_enabled(self, enabled: bool) -> None:
        """Enables or disables all action buttons at once."""
        if enabled:
            self._update_buttons()
        else:
            self._btn_heal.setEnabled(False)
            self._btn_equip.setEnabled(False)

    def _on_heal_clicked(self) -> None:
        """Internal handler for the Heal button click."""
        if self._hero_id:
            self.heal_requested.emit(self._hero_id, self._hero_id)

    def _on_equip_clicked(self) -> None:
        """Internal handler for the Equip button click."""
        if self._hero_id:
            self.equip_requested.emit(self._hero_id, "bigword_available")

    def reset(self) -> None:
        """Disables all action buttons and clears selection state."""
        self._hero_id = ""
        self._has_potion = False
        self._has_item = False
        self._weapon_equipped = False
        self._btn_heal.setEnabled(False)
        self._btn_equip.setEnabled(False)
