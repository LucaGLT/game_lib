"""GmFlowModule — Phase / Round / Turn / Timeline visualiser.

Subscribes to gmFlow session, phase, round, turn, and timeline events
and updates a compact top-docked panel:

- Row 1: Session / Phase / Round / Turn status labels
- Row 2: :class:`~gmGui.widgets.timeline_scene.TimelineScene` in a
    ``QGraphicsView`` (compact fixed height)
- Row 3: Event log (last 20 entries, most-recent at top)
"""
from __future__ import annotations

from datetime import datetime

from PySide6.QtCore import Qt
from PySide6.QtGui import QPainter
from PySide6.QtWidgets import (
    QGraphicsView,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from ..widgets.timeline_scene import TimelineScene
from .base_module import BaseModule

_MAX_LOG: int = 20
_TOGGLE_EXPANDED_ICON: str = "▾"
_TOGGLE_COLLAPSED_ICON: str = "▸"


class GmFlowModule(BaseModule):
    """Visualises gmFlow session, phase, round, turn, and timeline state.

    TypeIds from ``FlowEvents.hpp`` and ``TimelineEvents.hpp``.
    Tracks turn count and round count (partite) for the Tris game.
    """

    def __init__(self) -> None:
        super().__init__()
        self._session_count: int = 0
        self._round_count: int = 0
        self._turn_count: int = 0
        self._turn_actors: list[str] = []  # Track actor for each turn
        # Keep the module compact by default; user can expand log on demand.
        self._log_visible: bool = False

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
            "gmFlow.session.resumed",
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
        container.setObjectName("gm_flow_module")
        container.setSizePolicy(
            QSizePolicy.Policy.Preferred,
            QSizePolicy.Policy.Minimum,
        )
        container.setMinimumHeight(72)
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(4, 4, 4, 4)
        vbox.setSpacing(2)

        # ── Row 1: status labels ──────────────────────────────────────────────
        row1 = QHBoxLayout()
        row1.setSpacing(4)
        row1.setContentsMargins(0, 0, 0, 0)

        self._lbl_session: QLabel = QLabel("👥 Session: —")
        self._lbl_session.setProperty("flow_kind", "session")
        self._lbl_session.setFixedHeight(20)
        self._lbl_session.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)

        self._lbl_phase: QLabel = QLabel("📍 Phase: —")
        self._lbl_phase.setProperty("flow_kind", "phase")
        self._lbl_phase.setFixedHeight(20)
        self._lbl_phase.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)

        self._lbl_round: QLabel = QLabel("🔄 Round: —")
        self._lbl_round.setProperty("flow_kind", "round")
        self._lbl_round.setFixedHeight(20)
        self._lbl_round.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)

        self._lbl_turn: QLabel = QLabel("⏱ Turn: —")
        self._lbl_turn.setProperty("flow_kind", "turn")
        self._lbl_turn.setFixedHeight(20)
        self._lbl_turn.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)

        row1.addWidget(self._lbl_session)
        row1.addWidget(self._lbl_phase)
        row1.addWidget(self._lbl_round)
        row1.addWidget(self._lbl_turn)
        row1.addStretch()
        vbox.addLayout(row1)

        # ── Row 2: timeline scene ─────────────────────────────────────────────
        self._timeline_scene: TimelineScene = TimelineScene()
        self._timeline_view: QGraphicsView = QGraphicsView(self._timeline_scene)
        self._timeline_view.setFixedHeight(30)
        self._timeline_view.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        self._timeline_view.setAlignment(
            Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter
        )
        self._timeline_view.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        self._timeline_view.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAsNeeded
        )
        self._timeline_view.setVerticalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )
        vbox.addWidget(self._timeline_view)

        # ── Hidden control buttons (kept for command wiring compatibility) ───
        self._btn_resume: QPushButton = QPushButton("▶ RESUME")
        self._btn_pause: QPushButton = QPushButton("⏸ PAUSE")
        self._btn_stop: QPushButton = QPushButton("⏹ STOP")
        self._btn_pass_turn: QPushButton = QPushButton("⏭ Passa Turno")
        self._btn_resume.setVisible(False)
        self._btn_pause.setVisible(False)
        self._btn_stop.setVisible(False)
        self._btn_pass_turn.setVisible(False)
        self._btn_resume.setEnabled(False)
        self._btn_pause.setEnabled(False)
        self._btn_stop.setEnabled(False)
        self._btn_pass_turn.setEnabled(False)

        self._btn_resume.clicked.connect(
            lambda: self.send_command("gmFlow.session.resume", {})
        )
        self._btn_pause.clicked.connect(
            lambda: self.send_command("gmFlow.session.pause", {})
        )
        self._btn_stop.clicked.connect(
            lambda: self.send_command("gmFlow.session.stop", {})
        )
        self._btn_pass_turn.clicked.connect(
            lambda: self.send_command("gmFlow.turn.pass", {})
        )

        # ── Row 3: event log with collapsible header ──────────────────────────
        log_header = QHBoxLayout()
        log_header.addWidget(self._btn_pass_turn)
        log_header.addStretch()

        self._btn_toggle_log: QPushButton = QPushButton(_TOGGLE_COLLAPSED_ICON)
        self._btn_toggle_log.setToolTip("Mostra/Nascondi log eventi")
        self._btn_toggle_log.setProperty("toggle_icon", "true")
        self._btn_toggle_log.setFixedWidth(20)
        self._btn_toggle_log.clicked.connect(self._toggle_log_visibility)
        log_header.addWidget(self._btn_toggle_log)
        vbox.addLayout(log_header)

        self._log: QListWidget = QListWidget()
        self._log.setMinimumHeight(16)
        self._log.setMaximumHeight(56)
        self._log.setObjectName("flow_event_log")
        self._log.setVisible(self._log_visible)
        vbox.addWidget(self._log)

        return container

    # ── Envelope handler ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {})

        if tid == "gmFlow.session.started":
            session_id: str = str(data.get("session_id", "?"))
            self._session_count += 1
            self._lbl_session.setText(f"👥 Session: {session_id}")
            self._round_count = 0
            self._turn_count = 0
            self._turn_actors = []
            self._lbl_round.setText(f"🔄 Round: —")
            self._lbl_turn.setText(f"⏱ Turn: —")
            self._timeline_scene.clear_tokens()
            self._btn_pause.setEnabled(True)
            self._btn_stop.setEnabled(True)
            self._btn_resume.setEnabled(False)
            self._btn_pass_turn.setEnabled(True)
            self._append_log(f"▶ Session {self._session_count} started: {session_id}")

        elif tid == "gmFlow.session.paused":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(True)
            self._btn_pass_turn.setEnabled(False)
            self._append_log("⏸ Session paused")

        elif tid == "gmFlow.session.completed":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(False)
            self._btn_stop.setEnabled(False)
            self._btn_pass_turn.setEnabled(False)
            self._append_log("⏹ Session completed")

        elif tid == "gmFlow.session.resumed":
            self._btn_pause.setEnabled(True)
            self._btn_resume.setEnabled(False)
            self._btn_pass_turn.setEnabled(True)
            self._append_log("▶ Session resumed")

        elif tid == "gmFlow.phase.entered":
            phase_id: str = str(data.get("phase_id", "?"))
            self._lbl_phase.setText(f"📍 Phase: {phase_id}")
            self._append_log(f"📍 Phase entered: {phase_id}")

        elif tid == "gmFlow.phase.exited":
            self._append_log(f"Phase exited: {data.get('phase_id', '?')}")

        elif tid == "gmFlow.round.started":
            if "index" in data:
                self._round_count = int(data.get("index", 1))
            elif "round_number" in data:
                self._round_count = int(data.get("round_number", 1))
            else:
                self._round_count += 1
            self._lbl_round.setText(f"🔄 Round: {self._round_count}")
            self._append_log(f"🔄 Round {self._round_count} started")

        elif tid == "gmFlow.round.ended":
            self._append_log(f"Round {self._round_count} ended")

        elif tid == "gmFlow.turn.started":
            self._turn_count += 1
            turn_id: str = str(data.get("turn_id", "?"))
            if turn_id == "?" and "turn_number" in data:
                turn_id = str(data.get("turn_number", "?"))
            active_actors: list = data.get("active_actors", [])
            active_name: str = str(active_actors[0]) if active_actors else "?"
            self._turn_actors.append(active_name)
            self._lbl_turn.setText(f"⏱ Turn: {turn_id}")
            token_id: str = f"turn_{self._turn_count}"
            rect = self._timeline_scene.add_turn_token(token_id, active_name)
            self._timeline_scene.select_actor(active_name)
            self._timeline_view.centerOn(rect)
            self._append_log(f"⏱ Turn {self._turn_count}: {turn_id}")

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

    def _toggle_log_visibility(self) -> None:
        """Toggles the event log visibility."""
        self._log_visible = not self._log_visible
        self._log.setVisible(self._log_visible)
        self._btn_toggle_log.setText(
            _TOGGLE_EXPANDED_ICON if self._log_visible else _TOGGLE_COLLAPSED_ICON
        )

    def _append_log(self, text: str) -> None:
        """Prepends *text* with timestamp to the event log."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        full_text = f"{timestamp}  {text}"
        item = QListWidgetItem(full_text)
        self._log.insertItem(0, item)
        while self._log.count() > _MAX_LOG:
            self._log.takeItem(self._log.count() - 1)
