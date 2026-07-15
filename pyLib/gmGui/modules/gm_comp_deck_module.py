"""GmCompDeckModule — Five-zone deck manager (GmCompDeck).

Subscribes to deck card-move, zone-change, shuffle, and draw events and
renders the GmCompDeck state as five side-by-side ``ZoneList`` columns.

Zone names (must match the C++ ``GmCompDeck`` zone identifiers):
    MainDeck, CardHand, PlayArea, DiscardPile, BanishZone

Drop policy
-----------
- All zones except BanishZone accept drag-and-drop moves.
- BanishZone has ``NoDragDrop`` — cards arrive there only via engine events.
- A drop emits ``card_dropped`` on the ZoneList, which calls
  ``send_command("gmAlea.deck.move_card", ...)`` and waits for the engine's
  ``gmAlea.deck.card_moved`` response before updating the UI.
"""
from __future__ import annotations

from PySide6.QtCore import QPropertyAnimation, Qt
from PySide6.QtWidgets import (
    QAbstractItemView,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidgetItem,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..widgets.zone_list import ZoneList
from .base_module import BaseModule

# Ordered zone names — order determines display order (left → right).
_ZONE_NAMES: list[str] = [
    "MainDeck",
    "CardHand",
    "PlayArea",
    "Memory",
    "DiscardPile",
    "BanishZone",
]

_BANISH_ZONE: str = "BanishZone"
_ROLE_CARD_NAME: int = int(Qt.ItemDataRole.UserRole) + 1
_ROLE_CARD_META: int = int(Qt.ItemDataRole.UserRole) + 2  # dict with full card metadata


class GmCompDeckModule(BaseModule):
    """Visualises GmCompDeck zones: MainDeck, CardHand, PlayArea, DiscardPile, BanishZone.

    TypeIds: ``gmAlea.deck.card_moved``, ``gmAlea.deck.zone_changed``,
    ``gmAlea.deck.shuffled``, ``gmAlea.deck.drawn``.
    """

    @property
    def module_id(self) -> str:
        return "gm_comp_deck"

    @property
    def title(self) -> str:
        return "Deck Manager"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.RightDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmAlea.deck.card_moved",
            "gmAlea.deck.zone_changed",
            "gmAlea.deck.shuffled",
            "gmAlea.deck.drawn",
            "gmActor.snapshot",
            "gmActor.actor.resource_changed",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        # Internal lookups built during construction.
        self._zone_lists: dict[str, ZoneList] = {}
        self._counters: dict[str, QLabel] = {}
        self._flash_anims: dict[str, QPropertyAnimation] = {}
        self._zone_groups: dict[str, QGroupBox] = {}
        self._selected_zone_name: str | None = None
        self._selected_card_id: str | None = None
        self._non_usable_count: int = 0
        self._zone_revealed: dict[str, bool] = {
            "DiscardPile": False,
            "MainDeck": False,
        }
        # Resources of the active actor (Player_X), updated via events.
        self._active_actor_actions: int = 1
        # When True, warn before playing a card that would exceed available
        # actions. Games where a backend engine validates action cost should
        # disable this via set_enforce_action_cost(False). Preserve any value
        # set before the (lazy) widget build so the opt-out is not overwritten.
        self._enforce_action_cost: bool = getattr(self, "_enforce_action_cost", True)

        container = QWidget()
        container.setObjectName("gm_comp_deck_module")
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(8, 8, 8, 8)
        vbox.setSpacing(8)

        # ── Deck selector ─────────────────────────────────────────────────────
        top_row = QHBoxLayout()
        top_row.setSpacing(8)
        deck_lbl = QLabel("Mazzo:")
        deck_lbl.setProperty("text_role", "subtitle")
        self._deck_combo = QComboBox()
        self._deck_combo.addItem("Deck 1")
        top_row.addWidget(deck_lbl)
        top_row.addWidget(self._deck_combo)
        top_row.addStretch()
        vbox.addLayout(top_row)

        # ── Main dashboard grid ───────────────────────────────────────────────
        grid = QGridLayout()
        grid.setHorizontalSpacing(8)
        grid.setVerticalSpacing(8)

        self._detail_label = QLabel(
            "Info carta selezionata.\n\n"
            "Seleziona una carta da una zona per vedere i dettagli."
        )
        self._detail_label.setWordWrap(True)
        self._detail_label.setTextFormat(Qt.TextFormat.RichText)
        self._detail_label.setProperty("text_role", "body")
        detail_box = QGroupBox("Dettaglio Carta")
        detail_layout = QVBoxLayout(detail_box)
        detail_layout.setContentsMargins(8, 24, 8, 8)
        detail_layout.setSpacing(8)
        detail_layout.addWidget(self._detail_label)

        play_box = self._create_zone_group("Giocate", "PlayArea")
        memory_box = self._create_memory_group()

        discard_box = self._create_zone_group("Scarti", "DiscardPile")
        main_box = self._create_zone_group("Mazzo", "MainDeck")
        hand_box = self._create_zone_group("Mano", "CardHand")
        banish_box = self._create_zone_group("Eliminate", "BanishZone")
        not_used_box = self._create_not_used_group()

        right_stack = QWidget()
        right_stack_layout = QVBoxLayout(right_stack)
        right_stack_layout.setContentsMargins(0, 0, 0, 0)
        right_stack_layout.setSpacing(8)
        right_stack_layout.addWidget(banish_box)
        right_stack_layout.addWidget(not_used_box)

        grid.addWidget(detail_box, 0, 0, 1, 2)
        grid.addWidget(play_box, 0, 2)
        grid.addWidget(memory_box, 0, 3)

        grid.addWidget(discard_box, 1, 0)
        grid.addWidget(main_box, 1, 1)
        grid.addWidget(hand_box, 1, 2)
        grid.addWidget(right_stack, 1, 3)

        grid.setColumnStretch(0, 2)
        grid.setColumnStretch(1, 2)
        grid.setColumnStretch(2, 4)
        grid.setColumnStretch(3, 2)
        vbox.addLayout(grid)

        # ── Zone action buttons (inside matching panels) ─────────────────────

        self._btn_observe_discard: QPushButton = QPushButton("Osserva (Nascondi)")
        self._btn_observe_main: QPushButton = QPushButton("Osserva (Nascondi)")
        self._btn_discard_pick: QPushButton = QPushButton("Prendi la Prima")
        self._btn_discard_shuffle: QPushButton = QPushButton("Rimescola")

        self._btn_hand_discard: QPushButton = QPushButton("<- Scarta")
        self._btn_hand_play: QPushButton = QPushButton("^ Gioca")
        self._btn_hand_banish: QPushButton = QPushButton("Elimina ->")
        self._btn_hand_memory: QPushButton = QPushButton("^ In Memoria")

        self._btn_play_discard: QPushButton = QPushButton("Scarta")
        self._btn_play_retake: QPushButton = QPushButton("Riprendi")
        self._btn_memory_retake: QPushButton = QPushButton("Riprendi")

        self._btn_draw = QPushButton("Pesca la Prima")
        self._btn_shuffle = QPushButton("Gestisci Mazzo")

        self._btn_observe_discard.clicked.connect(
            lambda: self._toggle_zone_reveal("DiscardPile")
        )
        self._btn_observe_main.clicked.connect(
            lambda: self._toggle_zone_reveal("MainDeck")
        )
        self._btn_draw.clicked.connect(self._on_main_deck_pick)

        self._btn_discard_pick.clicked.connect(self._on_discard_pick)
        self._btn_discard_shuffle.clicked.connect(
            lambda: self.send_command("gmAlea.deck.recycle_discard", {})
        )

        self._btn_hand_discard.clicked.connect(
            lambda: self._move_selected_from_to("CardHand", "DiscardPile")
        )
        self._btn_hand_play.clicked.connect(
            lambda: self._move_selected_from_to("CardHand", "PlayArea")
        )
        self._btn_hand_banish.clicked.connect(
            lambda: self._move_selected_from_to("CardHand", "BanishZone")
        )
        self._btn_hand_memory.clicked.connect(
            lambda: self._move_selected_from_to("CardHand", "Memory")
        )

        self._btn_play_discard.clicked.connect(
            lambda: self._move_selected_from_to("PlayArea", "DiscardPile")
        )
        self._btn_play_retake.clicked.connect(
            lambda: self._move_selected_from_to("PlayArea", "CardHand")
        )
        self._btn_memory_retake.clicked.connect(
            lambda: self._move_selected_from_to("Memory", "CardHand")
        )

        self._btn_shuffle.clicked.connect(
            lambda: self.send_command("gmAlea.deck.recycle_discard", {})
        )

        discard_layout = self._zone_groups["DiscardPile"].layout()
        if isinstance(discard_layout, QVBoxLayout):
            discard_layout.addWidget(self._btn_observe_discard)
            discard_layout.addWidget(self._btn_discard_pick)
            discard_layout.addWidget(self._btn_discard_shuffle)

        main_layout = self._zone_groups["MainDeck"].layout()
        if isinstance(main_layout, QVBoxLayout):
            main_layout.addWidget(self._btn_observe_main)
            main_layout.addWidget(self._btn_draw)

        hand_layout = self._zone_groups["CardHand"].layout()
        if isinstance(hand_layout, QVBoxLayout):
            hand_btn_grid = QGridLayout()
            hand_btn_grid.setHorizontalSpacing(8)
            hand_btn_grid.setVerticalSpacing(4)
            hand_btn_grid.addWidget(self._btn_hand_discard, 0, 0)
            hand_btn_grid.addWidget(self._btn_hand_play, 0, 1)
            hand_btn_grid.addWidget(self._btn_hand_banish, 0, 2)
            hand_btn_grid.addWidget(self._btn_hand_memory, 1, 1)
            hand_layout.addLayout(hand_btn_grid)

        play_layout = self._zone_groups["PlayArea"].layout()
        if isinstance(play_layout, QVBoxLayout):
            play_btn_grid = QGridLayout()
            play_btn_grid.setHorizontalSpacing(8)
            play_btn_grid.setVerticalSpacing(4)
            play_btn_grid.addWidget(self._btn_play_discard, 0, 0)
            play_btn_grid.addWidget(self._btn_play_retake, 0, 1)
            play_layout.addLayout(play_btn_grid)

        memory_layout = self._zone_groups["Memory"].layout()
        if isinstance(memory_layout, QVBoxLayout):
            memory_layout.addWidget(self._btn_memory_retake)

        not_used_layout = self._zone_groups["NonInUso"].layout()
        if isinstance(not_used_layout, QVBoxLayout):
            not_used_layout.addWidget(self._btn_shuffle)

        self._refresh_zone_texts("DiscardPile")
        self._refresh_zone_texts("MainDeck")
        self._refresh_not_used_label()

        return container

    def _create_zone_group(self, title: str, zone_name: str) -> QGroupBox:
        group = QGroupBox(title)
        layout = QVBoxLayout(group)
        layout.setContentsMargins(8, 24, 8, 8)
        layout.setSpacing(4)

        zone_list = ZoneList(zone_name)
        if zone_name == _BANISH_ZONE:
            zone_list.setDragDropMode(QAbstractItemView.DragDropMode.NoDragDrop)
        zone_list.card_dropped.connect(self._on_card_dropped)
        zone_list.currentItemChanged.connect(
            lambda curr, _prev, current_zone=zone_name: self._on_zone_selection_changed(
                current_zone, curr
            )
        )
        layout.addWidget(zone_list)

        counter = QLabel("0 carte")
        counter.setAlignment(Qt.AlignmentFlag.AlignCenter)
        counter.setProperty("text_role", "caption")
        layout.addWidget(counter)

        from PySide6.QtWidgets import QGraphicsOpacityEffect

        effect = QGraphicsOpacityEffect(counter)
        effect.setOpacity(1.0)
        counter.setGraphicsEffect(effect)
        anim = QPropertyAnimation(effect, b"opacity")
        anim.setDuration(300)

        self._zone_lists[zone_name] = zone_list
        self._counters[zone_name] = counter
        self._flash_anims[zone_name] = anim
        self._zone_groups[zone_name] = group
        return group

    def _create_memory_group(self) -> QGroupBox:
        group = QGroupBox("Memoria")
        layout = QVBoxLayout(group)
        layout.setContentsMargins(8, 24, 8, 8)
        layout.setSpacing(4)

        zone_list = ZoneList("Memory")
        zone_list.card_dropped.connect(self._on_card_dropped)
        zone_list.currentItemChanged.connect(
            lambda curr, _prev: self._on_zone_selection_changed("Memory", curr)
        )
        layout.addWidget(zone_list)

        counter = QLabel("0 carte")
        counter.setAlignment(Qt.AlignmentFlag.AlignCenter)
        counter.setProperty("text_role", "caption")
        layout.addWidget(counter)

        from PySide6.QtWidgets import QGraphicsOpacityEffect

        effect = QGraphicsOpacityEffect(counter)
        effect.setOpacity(1.0)
        counter.setGraphicsEffect(effect)
        anim = QPropertyAnimation(effect, b"opacity")
        anim.setDuration(300)

        self._zone_lists["Memory"] = zone_list
        self._counters["Memory"] = counter
        self._flash_anims["Memory"] = anim
        self._zone_groups["Memory"] = group

        return group

    def _create_not_used_group(self) -> QGroupBox:
        group = QGroupBox("Non in Uso")
        layout = QVBoxLayout(group)
        layout.setContentsMargins(8, 24, 8, 8)
        layout.setSpacing(4)

        self._lbl_not_used = QLabel("0 carte")
        self._lbl_not_used.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._lbl_not_used.setWordWrap(True)
        self._lbl_not_used.setProperty("text_role", "subtitle")
        layout.addWidget(self._lbl_not_used)

        self._zone_groups["NonInUso"] = group
        return group

    # ── Public API ────────────────────────────────────────────────────────────

    def set_enforce_action_cost(self, enabled: bool) -> None:
        """Enables or disables the client-side action-cost warning.

        When disabled, the module never blocks a card play based on its own
        action accounting; a backend engine is expected to validate instead.

        Args:
            enabled: True to keep the local warning; False to delegate to the
                backend engine.
        """
        self._enforce_action_cost = enabled

    # ── Envelope handler ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {})

        if tid == "gmAlea.deck.zone_changed":
            self._handle_zone_changed(data)
        elif tid == "gmAlea.deck.card_moved":
            self._handle_card_moved(data)
        elif tid == "gmAlea.deck.shuffled":
            self._handle_shuffled(data)
        elif tid == "gmAlea.deck.drawn":
            self._handle_drawn(data)
        elif tid == "gmActor.snapshot":
            self._handle_actor_snapshot(data)
        elif tid == "gmActor.actor.resource_changed":
            self._handle_resource_changed(data)

    # ── Private handlers ──────────────────────────────────────────────────────

    def _handle_actor_snapshot(self, data: dict) -> None:
        for actor in data.get("actors", []):
            if str(actor.get("actor_id", "")) == "Player_X":
                resources: dict = actor.get("resources", {})
                self._active_actor_actions = int(resources.get("actions", 1))
                break

    def _handle_resource_changed(self, data: dict) -> None:
        if str(data.get("actor_id", "")) == "Player_X" and str(data.get("resource_id", "")) == "actions":
            self._active_actor_actions = int(data.get("new_value", self._active_actor_actions))

    def _handle_zone_changed(self, data: dict) -> None:
        zone_name: str = str(data.get("zone_name", ""))
        zone_list: ZoneList | None = self._zone_lists.get(zone_name)
        if zone_list is None:
            if zone_name in {"NonUsable", "NonUsableZone", "NON_USABLE"}:
                self._non_usable_count = len(data.get("cards", []))
                self._refresh_not_used_label()
            return
        zone_list.clear()
        for card in data.get("cards", []):
            card_id: str = str(card.get("card_id", ""))
            name: str = str(card.get("name", card_id))
            meta: dict = {
                "value":         card.get("value", ""),
                "action_cost":   int(card.get("action_cost", 1)),
                "rule_group_id": str(card.get("rule_group_id", "")),
                "description":   str(card.get("description", "")),
                "rules":         list(card.get("rules", [])),
            }
            self._add_card_item(zone_list, card_id, name, meta)
        self._refresh_zone_texts(zone_name)
        self._update_counter(zone_name)

    def _handle_card_moved(self, data: dict) -> None:
        card_id: str = str(data.get("card_id", ""))
        from_zone: str = str(data.get("from_zone", ""))
        to_zone: str = str(data.get("to_zone", ""))
        src: ZoneList | None = self._zone_lists.get(from_zone)
        dst: ZoneList | None = self._zone_lists.get(to_zone)
        if src is None or dst is None:
            return
        item: QListWidgetItem | None = self._find_item(src, card_id)
        if item is None:
            return
        row: int = src.row(item)
        taken: QListWidgetItem = src.takeItem(row)
        dst.insertItem(0, taken)
        self._refresh_zone_texts(from_zone)
        self._refresh_zone_texts(to_zone)
        self._update_counter(from_zone)
        self._update_counter(to_zone)

    def _handle_shuffled(self, data: dict) -> None:
        zone_name: str = str(data.get("zone_name", "MainDeck"))
        self._update_counter(zone_name)
        self._flash_zone(zone_name)

    def _handle_drawn(self, data: dict) -> None:
        card_id: str = str(data.get("card_id", ""))
        # "drawn" is equivalent to card_moved from MainDeck to CardHand.
        self._handle_card_moved({
            "card_id": card_id,
            "from_zone": "MainDeck",
            "to_zone": "CardHand",
        })

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns the selected deck name for QSettings persistence."""
        if self._widget is None:
            return {}
        return {"deck": self._deck_combo.currentText()}

    def restore_state(self, state: dict) -> None:
        """Restores the selected deck combo entry from a previously saved state dict."""
        if self._widget is None:
            return
        deck = state.get("deck", "")
        idx = self._deck_combo.findText(deck)
        if idx >= 0:
            self._deck_combo.setCurrentIndex(idx)

    def _on_card_dropped(self, card_id: str, from_zone: str, to_zone: str) -> None:
        """Slot connected to ZoneList.card_dropped — forwards to engine."""
        if not self._is_drag_move_allowed(card_id, from_zone, to_zone):
            return
        self.send_command(
            "gmAlea.deck.move_card",
            {"card_id": card_id, "from": from_zone, "to": to_zone},
        )

    # ── UI helpers ────────────────────────────────────────────────────────────

    @staticmethod
    def _add_card_item(
        zone_list: ZoneList,
        card_id: str,
        name: str,
        meta: dict | None = None,
    ) -> None:
        item = QListWidgetItem(name)
        item.setData(Qt.ItemDataRole.UserRole, card_id)
        item.setData(_ROLE_CARD_NAME, name)
        item.setData(_ROLE_CARD_META, meta or {})
        zone_list.addItem(item)

    def _update_counter(self, zone_name: str) -> None:
        zone_list: ZoneList | None = self._zone_lists.get(zone_name)
        counter: QLabel | None = self._counters.get(zone_name)
        if zone_list is not None and counter is not None:
            n: int = zone_list.count()
            counter.setText(f"{n} carte" if n != 1 else "1 carta")
            self._refresh_not_used_label()

    def _flash_zone(self, zone_name: str) -> None:
        """Triggers a brief opacity flash on the counter label of *zone_name*."""
        anim: QPropertyAnimation | None = self._flash_anims.get(zone_name)
        if anim is not None:
            anim.stop()
            anim.setStartValue(0.3)
            anim.setEndValue(1.0)
            anim.start()

    @staticmethod
    def _find_item(
        zone_list: ZoneList, card_id: str
    ) -> QListWidgetItem | None:
        """Returns the first item in *zone_list* whose UserRole equals *card_id*."""
        for i in range(zone_list.count()):
            item: QListWidgetItem = zone_list.item(i)
            if item.data(Qt.ItemDataRole.UserRole) == card_id:
                return item
        return None

    def _toggle_zone_reveal(self, zone_name: str) -> None:
        current = self._zone_revealed.get(zone_name, False)
        self._zone_revealed[zone_name] = not current
        self._refresh_zone_texts(zone_name)
        if zone_name == "MainDeck":
            self._btn_draw.setText("Scegli" if not current else "Pesca la Prima")
        elif zone_name == "DiscardPile":
            self._btn_discard_pick.setText("Scegli" if not current else "Prendi la Prima")

    def _refresh_zone_texts(self, zone_name: str) -> None:
        zone_list: ZoneList | None = self._zone_lists.get(zone_name)
        if zone_list is None:
            return
        reveal = self._zone_revealed.get(zone_name, True)
        if reveal:
            for idx in range(zone_list.count()):
                item = zone_list.item(idx)
                full_name = str(item.data(_ROLE_CARD_NAME) or item.text())
                item.setText(full_name)
        else:
            for idx in range(zone_list.count()):
                item = zone_list.item(idx)
                if zone_name == "DiscardPile" and idx == 0:
                    full_name = str(item.data(_ROLE_CARD_NAME) or item.text())
                    item.setText(full_name)
                else:
                    item.setText(f"Carta_{idx + 1}")

    def _on_zone_selection_changed(
        self,
        zone_name: str,
        item: QListWidgetItem | None,
    ) -> None:
        if item is None:
            self._selected_zone_name = None
            self._selected_card_id = None
            self._detail_label.setText(
                "Info carta selezionata.\n\n"
                "Seleziona una carta da una zona per vedere i dettagli."
            )
            return
        card_id = str(item.data(Qt.ItemDataRole.UserRole) or "")
        card_name = str(item.data(_ROLE_CARD_NAME) or item.text())
        meta: dict = item.data(_ROLE_CARD_META) or {}
        self._selected_zone_name = zone_name
        self._selected_card_id = card_id

        lines: list[str] = []
        lines.append(card_name)
        lines.append("")

        description = str(meta.get("description", "")).strip()
        if description:
            lines.append(description)
            lines.append("")

        rules: list = meta.get("rules", [])
        if rules:
            lines.append("Effetti:")
            for r in rules:
                lines.append(f"  • {r}")
            lines.append("")

        value = meta.get("value", "")
        if value != "":
            lines.append(f"Costo: {value} monete")

        rg = str(meta.get("rule_group_id", "")).strip()
        if rg:
            lines.append(f"Rule Group: {rg}")
        else:
            lines.append("Rule Group: (nessuno)")

        lines.append(f"Zona: {zone_name}")
        lines.append(f"ID: {card_id}")

        self._detail_label.setText("\n".join(lines))

    def _refresh_not_used_label(self) -> None:
        count = self._non_usable_count
        if count == 0:
            main_deck = self._zone_lists.get("MainDeck")
            count = main_deck.count() if main_deck is not None else 0
        self._lbl_not_used.setText(f"{count} carte")

    def _move_selected_card(self, to_zone: str) -> None:
        if self._selected_card_id is None or self._selected_zone_name is None:
            return
        if self._selected_zone_name == to_zone:
            return
        self.send_command(
            "gmAlea.deck.move_card",
            {
                "card_id": self._selected_card_id,
                "from": self._selected_zone_name,
                "to": to_zone,
            },
        )

    def _send_move_card(self, card_id: str, from_zone: str, to_zone: str) -> None:
        self.send_command(
            "gmAlea.deck.move_card",
            {
                "card_id": card_id,
                "from": from_zone,
                "to": to_zone,
            },
        )

    def _move_selected_from_to(self, from_zone: str, to_zone: str) -> None:
        if self._selected_zone_name != from_zone or self._selected_card_id is None:
            return
        # When playing from hand to an active zone, check action cost.
        _PLAY_ZONES: frozenset[str] = frozenset({"PlayArea", "Memory"})
        if self._enforce_action_cost and from_zone == "CardHand" and to_zone in _PLAY_ZONES:
            action_cost: int = 1  # default
            zone_list = self._zone_lists.get(from_zone)
            if zone_list is not None:
                item = self._find_item(zone_list, self._selected_card_id)
                if item is not None:
                    meta = item.data(_ROLE_CARD_META) or {}
                    action_cost = int(meta.get("action_cost", 1))
            # Show warning only if playing would make actions go negative.
            if (self._active_actor_actions - action_cost) < 0:
                answer = QMessageBox.question(
                    self._widget,
                    "Azioni esaurite",
                    "Hai finito le Azioni.\nContinua comunque?",
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                    QMessageBox.StandardButton.No,
                )
                if answer != QMessageBox.StandardButton.Yes:
                    return
        self._send_move_card(self._selected_card_id, from_zone, to_zone)

    def _move_first_from_to(self, from_zone: str, to_zone: str) -> None:
        zone_list = self._zone_lists.get(from_zone)
        if zone_list is None or zone_list.count() == 0:
            return
        item = zone_list.item(0)
        card_id = str(item.data(Qt.ItemDataRole.UserRole) or "")
        if not card_id:
            return
        self._send_move_card(card_id, from_zone, to_zone)

    def _on_main_deck_pick(self) -> None:
        # Hidden mode: blind draw (first card). Revealed mode: selected card.
        if self._zone_revealed.get("MainDeck", False):
            self._move_selected_from_to("MainDeck", "CardHand")
        else:
            self._move_first_from_to("MainDeck", "CardHand")

    def _on_discard_pick(self) -> None:
        # Hidden mode: take first discard card. Revealed mode: selected card.
        if self._zone_revealed.get("DiscardPile", False):
            self._move_selected_from_to("DiscardPile", "CardHand")
        else:
            self._move_first_from_to("DiscardPile", "CardHand")

    def _first_card_id(self, zone_name: str) -> str | None:
        zone_list = self._zone_lists.get(zone_name)
        if zone_list is None or zone_list.count() == 0:
            return None
        first_item = zone_list.item(0)
        card_id = str(first_item.data(Qt.ItemDataRole.UserRole) or "")
        if card_id == "":
            return None
        return card_id

    def _is_drag_move_allowed(self, card_id: str, from_zone: str, to_zone: str) -> bool:
        # Allowed drag matrix by rule:
        # - MainDeck -> CardHand
        # - DiscardPile -> CardHand or MainDeck
        # - CardHand -> DiscardPile / PlayArea / Memory / BanishZone
        if from_zone == "MainDeck":
            if to_zone != "CardHand":
                return False
            if not self._zone_revealed.get("MainDeck", False):
                first_id = self._first_card_id("MainDeck")
                return first_id is not None and card_id == first_id
            return True

        if from_zone == "DiscardPile":
            if to_zone not in {"CardHand", "MainDeck"}:
                return False
            if not self._zone_revealed.get("DiscardPile", False):
                first_id = self._first_card_id("DiscardPile")
                return first_id is not None and card_id == first_id
            return True

        if from_zone == "CardHand":
            return to_zone in {"DiscardPile", "PlayArea", "Memory", "BanishZone"}

        return False
