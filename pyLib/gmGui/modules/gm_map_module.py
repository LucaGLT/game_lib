"""GmMapModule — Location graph and actor-position visualiser.

Subscribes to gmMap structural events and gmActor movement events.
Full implementation: Phase 8.
Phase 1 stub: renders a placeholder QLabel.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

from .base_module import BaseModule


class GmMapModule(BaseModule):
    """Visualises gmMap locations (nodes), adjacency edges, and actor markers.

    Layout (Phase 8):
    - Top bar: [Zoom -] [Zoom +] [Fit] + QComboBox layer (terrain/items/actors)
    - Centre:  QGraphicsView over MapScene (scroll + wheel zoom)
    - Bottom bar: selected-location info label

    Node colours reflect the ``terrain`` metadata field.
    Actor markers reposition in response to gmActor.actor.moved_area events.
    """

    @property
    def module_id(self) -> str:
        return "gm_map"

    @property
    def title(self) -> str:
        return "Map"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.LeftDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmMap.map.loaded",
            "gmMap.location.item_added",
            "gmMap.location.item_removed",
            "gmMap.location.metadata_changed",
            "gmActor.actor.moved_area",
            "gmActor.actor.position_changed",
        ]

    def _build_widget(self) -> QWidget:
        label = QLabel("stub – GmMap\n(Phase 8)")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return label

    def on_envelope(self, msg: dict) -> None:
        # TODO: Phase 8 — dispatch on msg["typeId"] to update MapScene
        pass
