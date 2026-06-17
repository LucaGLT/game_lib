"""GmFlowModule — Phase / Round / Turn / Timeline visualiser.

Subscribes to gmFlow session, phase, round, turn, and timeline events.
Full implementation: Phase 4.
Phase 1 stub: renders a placeholder QLabel.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

from .base_module import BaseModule


class GmFlowModule(BaseModule):
    """Visualises gmFlow session, phase, round, turn, and timeline state.

    Layout (Phase 4):
    - Row 1: Session / Phase / Round / Turn labels
    - Row 2: TimelineScene in a QGraphicsView (actors on a time axis)
    - Row 3: RESUME / PAUSE / STOP buttons
    - Row 4: QListWidget event log (last 20 entries)

    TypeIds from FlowEvents.hpp and TimelineEvents.hpp.
    """

    @property
    def module_id(self) -> str:
        return "gm_flow"

    @property
    def title(self) -> str:
        return "Flow / Timeline"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.TopDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmFlow.session.started",
            "gmFlow.session.paused",
            "gmFlow.session.completed",
            "gmFlow.phase.entered",
            "gmFlow.phase.exited",
            "gmFlow.round.started",
            "gmFlow.round.ended",
            "gmFlow.turn.started",
            "gmFlow.turn.ended",
            "gmFlow.timeline.actor_selected",
            "gmFlow.timeline.time_advanced",
        ]

    def _build_widget(self) -> QWidget:
        label = QLabel("stub – GmFlow\n(Phase 4)")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return label

    def on_envelope(self, msg: dict) -> None:
        # TODO: Phase 4 — dispatch on msg["typeId"] to update labels, scene, buttons
        pass
