"""GmActorModule — Actor tree and detail panel.

Subscribes to the eight actor lifecycle events defined in ActorEvents.hpp.
Full implementation: Phase 5.
Phase 1 stub: renders a placeholder QLabel.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

from .base_module import BaseModule


class GmActorModule(BaseModule):
    """Visualises gmActor state: actor tree, HP bar, statuses, equipment.

    Layout (Phase 5):
    - Left pane:  QComboBox faction filter + QTreeWidget (Name / HP / State columns)
    - Right pane: detail panel — HpBar, QFormLayout stats,
                  status list, equipment list

    TypeIds from ActorEvents.hpp (EVT_HP_CHANGED … EVT_LIFE_STATE_CHANGED).
    """

    @property
    def module_id(self) -> str:
        return "gm_actor"

    @property
    def title(self) -> str:
        return "Actors"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.RightDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmActor.actor.hp_changed",
            "gmActor.actor.status_added",
            "gmActor.actor.status_removed",
            "gmActor.actor.moved_area",
            "gmActor.actor.life_state_changed",
            "gmActor.actor.item_equipped",
            "gmActor.actor.item_unequipped",
        ]

    def _build_widget(self) -> QWidget:
        label = QLabel("stub – GmActor\n(Phase 5)")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return label

    def on_envelope(self, msg: dict) -> None:
        # TODO: Phase 5 — dispatch on msg["typeId"] to update tree / detail panel
        pass
