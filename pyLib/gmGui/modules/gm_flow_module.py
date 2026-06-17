"""GmFlowModule — Phase / Round / Turn / Timeline visualiser.

Subscribes to gmFlow session, phase, round, turn, and timeline events
and updates a compact top-docked panel:

- Row 1: Session / Phase / Round / Turn status labels
- Row 2: :class:`~gmGui.widgets.timeline_scene.TimelineScene` in a
  ``QGraphicsView`` (fixed 120 px height)
- Row 3: RESUME / PAUSE / STOP command buttons
- Row 4: Event log (last 20 entries, most-recent at top)
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QGraphicsView,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..widgets.timeline_scene import TimelineScene
from .base_module import BaseModule

_MAX_LOG: int = 20


class GmFlowModule(BaseModule):
    """Visualises gmFlow session, phase, round, turn, and timeline state.

    TypeIds from ``FlowEvents.hpp`` and ``TimelineEvents.hpp``.
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

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        container = QWidget()
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(4, 4, 4, 4)
        vbox.setSpacing(4)

        # ── Row 1: status labels ──────────────────────────────────────────────
        self._lbl_session: QLabel = QLabel("Session: —")
        self._lbl_phase: QLabel = QLabel("Phase: —")
        self._lbl_round: QLabel = QLabel("Round: —")
        self._lbl_turn: QLabel = QLabel("Turn: —")

        row1 = QHBoxLayout()
        for lbl in (
            self._lbl_session,
            self._lbl_phase,
            self._lbl_round,
            self._lbl_turn,
        ):
            row1.addWidget(lbl)
            row1.addWidget(QLabel("|"))
        row1.addStretch()
        vbox.addLayout(row1)

        # ── Row 2: timeline scene ─────────────────────────────────────────────
        self._timeline_scene: TimelineScene = TimelineScene()
        timeline_view = QGraphicsView(self._timeline_scene)
        timeline_view.setFixedHeight(120)
        timeline_view.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAsNeeded
        )
        timeline_view.setVerticalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )
        vbox.addWidget(timeline_view)

        # ── Row 3: control buttons ────────────────────────────────────────────
        self._btn_resume: QPushButton = QPushButton("▶ RESUME")
        self._btn_pause: QPushButton = QPushButton("⏸ PAUSE")
        self._btn_stop: QPushButton = QPushButton("⏹ STOP")
        self._btn_resume.setEnabled(False)
        self._btn_pause.setEnabled(False)
        self._btn_stop.setEnabled(False)

        self._btn_resume.clicked.connect(
            lambda: self.send_command("gmFlow.session.resume", {})
        )
        self._btn_pause.clicked.connect(
            lambda: self.send_command("gmFlow.session.pause", {})
        )
        self._btn_stop.clicked.connect(
            lambda: self.send_command("gmFlow.session.stop", {})
        )

        row3 = QHBoxLayout()
        row3.addWidget(self._btn_resume)
        row3.addWidget(self._btn_pause)
        row3.addWidget(self._btn_stop)
        row3.addStretch()
        vbox.addLayout(row3)

        # ── Row 4: event log ──────────────────────────────────────────────────
        self._log: QListWidget = QListWidget()
        self._log.setMaximumHeight(120)
        vbox.addWidget(self._log)

        return container

    # ── Envelope handler ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {})

        if tid == "gmFlow.session.started":
            session_id: str = str(data.get("session_id", "?"))
            self._lbl_session.setText(f"Session: {session_id}")
            self._btn_pause.setEnabled(True)
            self._btn_stop.setEnabled(True)
            self._btn_resume.setEnabled(False)
            self._append_log(f"Session started: {session_id}")

        elif tid == "gmFlow.session.paused":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(True)
            self._append_log("Session paused")

        elif tid == "gmFlow.session.completed":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(False)
            self._btn_stop.setEnabled(False)
            self._append_log("Session completed")

        elif tid == "gmFlow.phase.entered":
            phase_id: str = str(data.get("phase_id", "?"))
            self._lbl_phase.setText(f"Phase: {phase_id}")
            self._append_log(f"Phase entered: {phase_id}")

        elif tid == "gmFlow.phase.exited":
            self._append_log(f"Phase exited: {data.get('phase_id', '?')}")

        elif tid == "gmFlow.round.started":
            index: object = data.get("index", "?")
            self._lbl_round.setText(f"Round: {index}")
            self._append_log(f"Round {index} started")

        elif tid == "gmFlow.round.ended":
            self._append_log(f"Round {data.get('index', '?')} ended")

        elif tid == "gmFlow.turn.started":
            turn_id: str = str(data.get("turn_id", "?"))
            self._lbl_turn.setText(f"Turn: {turn_id}")
            active: list = data.get("active_actors", [])
            if active:
                self._timeline_scene.select_actor(str(active[0]))
            self._append_log(f"Turn started: {turn_id}")

        elif tid == "gmFlow.turn.ended":
            self._append_log(f"Turn ended: {data.get('turn_id', '?')}")

        elif tid == "gmFlow.timeline.actor_selected":
            self._timeline_scene.select_actor(str(data.get("actor_id", "")))

        elif tid == "gmFlow.timeline.time_advanced":
            self._timeline_scene.advance_time(int(data.get("new_time", 0)))

    # ── Persistence ───────────────────────────────────────────────────────────

    def save_state(self) -> dict:
        """Returns the current timeline zoom level for QSettings persistence."""
        if self._widget is None:
            return {}
        return {"pixels_per_unit": self._timeline_scene._pixels_per_unit}

    def restore_state(self, state: dict) -> None:
        """Restores timeline zoom level from a previously saved state dict."""
        if self._widget is None:
            return
        ppu = state.get("pixels_per_unit")
        if isinstance(ppu, int) and ppu > 0:
            self._timeline_scene._pixels_per_unit = ppu

    # ── Internal ──────────────────────────────────────────────────────────────

    def _append_log(self, text: str) -> None:
        """Prepends *text* to the event log, keeping at most ``_MAX_LOG`` entries."""
        self._log.insertItem(0, text)
        while self._log.count() > _MAX_LOG:
            self._log.takeItem(self._log.count() - 1)
