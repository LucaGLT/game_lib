"""ZoneList — drag-and-drop QListWidget for GmCompDeck zone visualisation.

Each ZoneList represents one zone (MainDeck, CardHand, PlayArea, DiscardPile,
or BanishZone).  When a card is dragged from one ZoneList to another, the
``card_dropped`` signal carries the card ID and both zone names.
Full implementation: Phase 6.
Phase 1 stub: class skeleton with Signal declared and no-op dropEvent override.
"""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QListWidget


class ZoneList(QListWidget):
    """A QListWidget that emits ``card_dropped`` when a card moves between zones.

    Each ``QListWidgetItem`` stores its card ID via::

        item.setData(Qt.ItemDataRole.UserRole, card_id)

    Signals:
        card_dropped: Emitted with ``(card_id, from_zone, to_zone)`` strings
                      when a drag-and-drop move completes.
    """

    card_dropped: Signal = Signal(str, str, str)

    def __init__(self, zone_name: str, parent: object = None) -> None:
        super().__init__(parent)  # type: ignore[arg-type]
        self._zone_name: str = zone_name
        # TODO: Phase 6 — setDragDropMode(DragDrop), setDefaultDropAction(MoveAction),
        #                  override dropEvent to emit card_dropped and call super()
