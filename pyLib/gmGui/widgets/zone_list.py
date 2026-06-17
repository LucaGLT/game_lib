"""ZoneList — drag-and-drop QListWidget for GmCompDeck zone visualisation.

Each ZoneList represents one zone (MainDeck, CardHand, PlayArea, DiscardPile,
or BanishZone).  When a card is dragged from one ZoneList to another the
``card_dropped`` signal is emitted and the Qt visual move is **suppressed**:
the actual item transfer is driven by the engine's ``gmAlea.deck.card_moved``
response, keeping the GUI in sync with C++ state at all times.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QDropEvent
from PySide6.QtWidgets import QAbstractItemView, QListWidget, QListWidgetItem


class ZoneList(QListWidget):
    """A QListWidget that emits ``card_dropped`` when a card moves between zones.

    Each ``QListWidgetItem`` stores its card ID via::

        item.setData(Qt.ItemDataRole.UserRole, card_id)

    Drop behaviour
    --------------
    - ``DragDrop`` mode on all zones except BanishZone.
    - BanishZone uses ``NoDragDrop`` (insert-only; UI cannot drag into or out of it).
    - On an inter-zone drop the signal is emitted and ``event.ignore()`` is called
      so the item stays in the source zone until the engine confirms the move.
    - Intra-zone drops (reordering within the same zone) call ``super()`` normally.

    Signals:
        card_dropped: Emitted with ``(card_id, from_zone, to_zone)`` strings
                      when a drag-and-drop move between two different zones
                      completes.
    """

    card_dropped: Signal = Signal(str, str, str)

    def __init__(self, zone_name: str, parent: object = None) -> None:
        super().__init__(parent)  # type: ignore[arg-type]
        self._zone_name: str = zone_name
        self.setDragDropMode(QAbstractItemView.DragDropMode.DragDrop)
        self.setDefaultDropAction(Qt.DropAction.MoveAction)
        self.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)

    def dropEvent(self, event: QDropEvent) -> None:
        """Intercepts inter-zone drops to emit ``card_dropped`` without moving the item.

        Intra-zone drops (reordering) are forwarded to Qt's default handler.
        """
        source = event.source()
        if isinstance(source, ZoneList) and source is not self:
            selected: list[QListWidgetItem] = source.selectedItems()
            if selected:
                card_id: str = (
                    selected[0].data(Qt.ItemDataRole.UserRole)
                    or selected[0].text()
                )
                self.card_dropped.emit(card_id, source._zone_name, self._zone_name)
            # Suppress Qt's built-in visual move — engine confirmation drives the UI.
            event.ignore()
        else:
            super().dropEvent(event)
