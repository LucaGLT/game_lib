"""Le Pergamene di Eldhôm — area info widget.

AreaInfoWidget shows details about a location (area) selected on the map:
the location name, its adjacent locations and the actors currently present
(heroes and monster instances).  All data is supplied by the main window
from the cached full-state snapshot — this widget performs no networking.

All visual styling is applied exclusively through QSS — no hardcoded color
values are present in this module.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QFrame,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QVBoxLayout,
    QWidget,
)


class AreaInfoWidget(QFrame):
    """Panel describing the selected map location.

    Signals:
        actor_selected(str): Emitted with the actor id when the player clicks
                             an actor entry in the present-actors list.
    """

    actor_selected = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setFrameShape(QFrame.Shape.StyledPanel)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        self._title = QLabel("Nessuna area selezionata", self)
        self._title.setProperty("text_role", "subtitle")
        layout.addWidget(self._title)

        self._adjacent = QLabel("", self)
        self._adjacent.setProperty("text_role", "secondary")
        self._adjacent.setWordWrap(True)
        layout.addWidget(self._adjacent)

        actors_label = QLabel("Attori presenti", self)
        actors_label.setProperty("text_role", "body")
        layout.addWidget(actors_label)

        self._actors_list = QListWidget(self)
        self._actors_list.itemClicked.connect(self._on_actor_clicked)
        layout.addWidget(self._actors_list, 1)

    def show_area(
        self,
        location_id: str,
        location_name: str,
        adjacent_names: list[str],
        actors: list[dict],
    ) -> None:
        """Populates the panel for the given location.

        Args:
            location_id:    String id of the location.
            location_name:  Human-readable location name.
            adjacent_names: Display names of adjacent locations.
            actors:         List of dicts with keys ``id``, ``name``,
                            ``kind`` ("HERO"/"MONSTER") and optional ``hp``,
                            ``max_hp``, ``position``.
        """
        self._title.setText(f"Area: {location_name}")
        if adjacent_names:
            self._adjacent.setText("Adiacenti: " + ", ".join(adjacent_names))
        else:
            self._adjacent.setText("Adiacenti: (nessuna)")

        self._actors_list.clear()
        for actor in actors:
            self._actors_list.addItem(self._make_item(actor))

        if not actors:
            empty = QListWidgetItem("(area vuota)")
            empty.setFlags(Qt.ItemFlag.NoItemFlags)
            self._actors_list.addItem(empty)

    def clear(self) -> None:
        """Resets the panel to its empty state."""
        self._title.setText("Nessuna area selezionata")
        self._adjacent.setText("")
        self._actors_list.clear()

    # ── Internal helpers ───────────────────────────────────────────────────────

    @staticmethod
    def _make_item(actor: dict) -> QListWidgetItem:
        """Builds a list item describing one actor in the area."""
        name = str(actor.get("name", actor.get("id", "?")))
        kind = str(actor.get("kind", ""))
        prefix = "\u2666" if kind == "HERO" else "\u2620"
        text = f"{prefix} {name}"
        hp = actor.get("hp")
        max_hp = actor.get("max_hp")
        if hp is not None and max_hp is not None:
            text += f"  \u2764 {hp}/{max_hp}"
        position = str(actor.get("position", ""))
        if position:
            text += f"  [{position}]"
        item = QListWidgetItem(text)
        item.setData(Qt.ItemDataRole.UserRole, str(actor.get("id", "")))
        item.setData(int(Qt.ItemDataRole.UserRole) + 1, kind)
        return item

    def _on_actor_clicked(self, item: QListWidgetItem) -> None:
        """Emits actor_selected with the clicked actor's id."""
        actor_id = str(item.data(Qt.ItemDataRole.UserRole) or "")
        if actor_id:
            self.actor_selected.emit(actor_id)
