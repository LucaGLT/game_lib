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
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidgetItem,
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
    "DiscardPile",
    "BanishZone",
]

_BANISH_ZONE: str = "BanishZone"


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
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        # Internal lookups built during construction.
        self._zone_lists: dict[str, ZoneList] = {}
        self._counters: dict[str, QLabel] = {}
        self._flash_anims: dict[str, QPropertyAnimation] = {}

        container = QWidget()
        container.setObjectName("gm_comp_deck_module")
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(4, 4, 4, 4)
        vbox.setSpacing(4)

        # ── Deck selector ─────────────────────────────────────────────────────
        self._deck_combo: QComboBox = QComboBox()
        self._deck_combo.addItem("Deck 1")
        vbox.addWidget(self._deck_combo)

        # ── Zone columns ──────────────────────────────────────────────────────
        row = QHBoxLayout()
        row.setSpacing(4)

        for zone_name in _ZONE_NAMES:
            col = QVBoxLayout()
            col.setSpacing(4)

            header = QLabel(zone_name)
            header.setAlignment(Qt.AlignmentFlag.AlignCenter)
            col.addWidget(header)

            zone_list = ZoneList(zone_name)
            if zone_name == _BANISH_ZONE:
                zone_list.setDragDropMode(
                    QAbstractItemView.DragDropMode.NoDragDrop
                )
            # Connect inter-zone drops to send_command.
            zone_list.card_dropped.connect(self._on_card_dropped)
            col.addWidget(zone_list)

            counter = QLabel("0 carte")
            counter.setAlignment(Qt.AlignmentFlag.AlignCenter)
            col.addWidget(counter)

            # Per-zone opacity flash animation on the counter label.
            from PySide6.QtWidgets import QGraphicsOpacityEffect  # local import ok
            effect = QGraphicsOpacityEffect(counter)
            effect.setOpacity(1.0)
            counter.setGraphicsEffect(effect)
            anim = QPropertyAnimation(effect, b"opacity")
            anim.setDuration(300)

            self._zone_lists[zone_name] = zone_list
            self._counters[zone_name] = counter
            self._flash_anims[zone_name] = anim

            group = QGroupBox()
            group.setLayout(col)
            row.addWidget(group)

        vbox.addLayout(row)

        # ── Action buttons ────────────────────────────────────────────────────
        btn_row = QHBoxLayout()
        self._btn_draw: QPushButton = QPushButton("Draw 1")
        self._btn_shuffle: QPushButton = QPushButton("Shuffle Discard→Main")
        self._btn_draw.clicked.connect(
            lambda: self.send_command("gmAlea.deck.draw", {"count": 1})
        )
        self._btn_shuffle.clicked.connect(
            lambda: self.send_command("gmAlea.deck.recycle_discard", {})
        )
        btn_row.addWidget(self._btn_draw)
        btn_row.addWidget(self._btn_shuffle)
        btn_row.addStretch()
        vbox.addLayout(btn_row)

        return container

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

    # ── Private handlers ──────────────────────────────────────────────────────

    def _handle_zone_changed(self, data: dict) -> None:
        zone_name: str = str(data.get("zone_name", ""))
        zone_list: ZoneList | None = self._zone_lists.get(zone_name)
        if zone_list is None:
            return
        zone_list.clear()
        for card in data.get("cards", []):
            card_id: str = str(card.get("card_id", ""))
            name: str = str(card.get("name", card_id))
            self._add_card_item(zone_list, card_id, name)
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
        dst.addItem(taken)
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
        self.send_command(
            "gmAlea.deck.move_card",
            {"card_id": card_id, "from": from_zone, "to": to_zone},
        )

    # ── UI helpers ────────────────────────────────────────────────────────────

    @staticmethod
    def _add_card_item(zone_list: ZoneList, card_id: str, name: str) -> None:
        item = QListWidgetItem(name)
        item.setData(Qt.ItemDataRole.UserRole, card_id)
        zone_list.addItem(item)

    def _update_counter(self, zone_name: str) -> None:
        zone_list: ZoneList | None = self._zone_lists.get(zone_name)
        counter: QLabel | None = self._counters.get(zone_name)
        if zone_list is not None and counter is not None:
            n: int = zone_list.count()
            counter.setText(f"{n} carte" if n != 1 else "1 carta")

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
