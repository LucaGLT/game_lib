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
    QVBoxLayout,
    QWidget,
)

from ..widgets.timeline_scene import TimelineScene
from .base_module import BaseModule

_MAX_LOG: int = 20


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
        self._log_visible: bool = True

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
        container.setObjectName("gm_flow_module")
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(8, 8, 8, 8)
        vbox.setSpacing(8)

        # ── Row 1: status labels ──────────────────────────────────────────────
        row1 = QHBoxLayout()
        row1.setSpacing(12)

        self._lbl_session: QLabel = QLabel("👥 Session: —")
        self._lbl_session.setProperty("flow_badge", "true")
        self._lbl_session.setProperty("flow_kind", "session")

        self._lbl_phase: QLabel = QLabel("📍 Phase: —")
        self._lbl_phase.setProperty("flow_badge", "true")
        self._lbl_phase.setProperty("flow_kind", "phase")

        self._lbl_round: QLabel = QLabel("🔄 Round: —")
        self._lbl_round.setProperty("flow_badge", "true")
        self._lbl_round.setProperty("flow_kind", "round")

        self._lbl_turn: QLabel = QLabel("⏱ Turn: —")
        self._lbl_turn.setProperty("flow_badge", "true")
        self._lbl_turn.setProperty("flow_kind", "turn")

        row1.addWidget(self._lbl_session)
        row1.addWidget(self._lbl_phase)
        row1.addWidget(self._lbl_round)
        row1.addWidget(self._lbl_turn)
        row1.addStretch()
        vbox.addLayout(row1)

        # ── Row 2: timeline scene with title ──────────────────────────────────
        timeline_title = QLabel("⏳ Timeline turni")
        timeline_title.setProperty("text_role", "subtitle")
        vbox.addWidget(timeline_title)

        self._timeline_scene: TimelineScene = TimelineScene()
        timeline_view = QGraphicsView(self._timeline_scene)
        timeline_view.setFixedHeight(100)
        timeline_view.setAlignment(
            Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter
        )
        timeline_view.setRenderHint(QPainter.RenderHint.Antialiasing, True)
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

        self._status_msg: QLabel = QLabel("")
        self._status_msg.setProperty("text_role", "secondary")

        row3 = QHBoxLayout()
        row3.setSpacing(8)
        row3.addWidget(self._btn_resume)
        row3.addWidget(self._btn_pause)
        row3.addWidget(self._btn_stop)
        row3.addStretch()
        row3.addWidget(self._status_msg)
        vbox.addLayout(row3)

        # ── Row 4: event log with collapsible header ──────────────────────────
        log_header = QHBoxLayout()
        log_title = QLabel("📋 Log eventi")
        log_title.setProperty("text_role", "subtitle")
        log_header.addWidget(log_title)
        log_header.addStretch()

        self._btn_toggle_log: QPushButton = QPushButton("Nascondi ▲")
        self._btn_toggle_log.setMaximumWidth(100)
        self._btn_toggle_log.clicked.connect(self._toggle_log_visibility)
        log_header.addWidget(self._btn_toggle_log)
        vbox.addLayout(log_header)

        self._log: QListWidget = QListWidget()
        self._log.setMaximumHeight(140)
        self._log.setObjectName("flow_event_log")
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
            self._init_timeline()
            self._btn_pause.setEnabled(True)
            self._btn_stop.setEnabled(True)
            self._btn_resume.setEnabled(False)
            self._status_msg.setText("")
            self._append_log(f"▶ Session {self._session_count} started: {session_id}")

        elif tid == "gmFlow.session.paused":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(True)
            self._status_msg.setText("Sessione in pausa")
            self._append_log("⏸ Session paused")

        elif tid == "gmFlow.session.completed":
            self._btn_pause.setEnabled(False)
            self._btn_resume.setEnabled(False)
            self._btn_stop.setEnabled(False)
            self._status_msg.setText("Sessione terminata")
            self._append_log("⏹ Session completed")

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
            self._populate_timeline()
            if active_name != "?":
                self._timeline_scene.select_actor(active_name)
            self._append_log(f"⏱ Turn {self._turn_count}: {turn_id}")

        elif tid == "gmFlow.turn.ended":
            self._append_log(f"Turn ended: {data.get('turn_id', '?')}")

        elif tid == "gmFlow.timeline.actor_selected":
            self._timeline_scene.select_actor(str(data.get("actor_id", "")))

        elif tid == "gmFlow.timeline.time_advanced":
            self._timeline_scene.advance_time(int(data.get("new_time", 0)))

    # ── Timeline management ───────────────────────────────────────────────────

    def _init_timeline(self) -> None:
        """Initializes timeline with 9 placeholder turn blocks (for Tris max turns).
        
        Each block shows "—" and is in inactive (gray) state.
        Uses unique slot IDs (slot_0 to slot_8) as dictionary keys.
        """
        placeholder_turns: list[dict] = []
        for slot_idx in range(9):  # 0 to 8
            placeholder_turns.append({
                "actor_id": f"slot_{slot_idx}",  # Unique ID for dict key
                "label": "—",                     # Display text
                "timeline_position": slot_idx
            })
        
        self._timeline_scene.set_actors(placeholder_turns)
        self._timeline_scene.advance_time(0)

    def _populate_timeline(self) -> None:
        """Updates the timeline with actual turn data as turns are played.
        
        Each turn slot shows the actor name (X, O, etc) or placeholder (—).
        Highlights the current active turn.
        """
        turns: list[dict] = []
        
        # Build 9 slots: filled with actors, rest with placeholders
        for slot_idx in range(9):
            if slot_idx < len(self._turn_actors):
                # Slot has a real turn
                actor_name = self._turn_actors[slot_idx]
                turns.append({
                    "actor_id": f"actor_{actor_name}_{slot_idx}",
                    "label": actor_name,
                    "timeline_position": slot_idx
                })
            else:
                # Slot is empty, use placeholder
                turns.append({
                    "actor_id": f"placeholder_{slot_idx}",
                    "label": "—",
                    "timeline_position": slot_idx
                })
        
        self._timeline_scene.set_actors(turns)
        
        # Highlight current turn actor
        if self._turn_actors and self._turn_count <= len(self._turn_actors):
            current_actor_id = f"actor_{self._turn_actors[self._turn_count - 1]}_{self._turn_count - 1}"
            self._timeline_scene.select_actor(current_actor_id)
        
        self._timeline_scene.advance_time(self._turn_count - 1)

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
        self._btn_toggle_log.setText("Nascondi ▲" if self._log_visible else "Mostra ▼")

    def _append_log(self, text: str) -> None:
        """Prepends *text* with timestamp to the event log."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        full_text = f"{timestamp}  {text}"
        item = QListWidgetItem(full_text)
        self._log.insertItem(0, item)
        while self._log.count() > _MAX_LOG:
            self._log.takeItem(self._log.count() - 1)
