"""GmCompDeckModule — Five-zone deck manager (GmCompDeck).

Subscribes to deck card-move, zone-change, shuffle, and draw events.
Full implementation: Phase 6.
Phase 1 stub: renders a placeholder QLabel.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

from .base_module import BaseModule


class GmCompDeckModule(BaseModule):
    """Visualises GmCompDeck zones: MainDeck, CardHand, PlayArea, DiscardPile, BanishZone.

    Layout (Phase 6):
    - QComboBox deck selector (multi-player future support)
    - Five ZoneList columns with card-count labels below each
    - [Draw 1] and [Shuffle Discard→Main] contextual buttons

    Zone invariant: BanishZone has DragDropMode=NoDragDrop (insert-only policy).
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

    def _build_widget(self) -> QWidget:
        label = QLabel("stub – GmCompDeck\n(Phase 6)")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return label

    def on_envelope(self, msg: dict) -> None:
        # TODO: Phase 6 — dispatch on msg["typeId"] to update zone lists
        pass
