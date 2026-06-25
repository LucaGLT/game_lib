"""Dungeon Crawler Basic — main window.

DungeonMainWindow assembles the full application layout:
- Central widget: DungeonBoardWidget (dungeon map view).
- Right dock:     HeroPanelWidget (actor HP, status, tags).
- Bottom dock:    ActionPanelWidget (Move / Heal / Equip buttons + target).
- Left dock:      LogWidget (game log).
- Status bar:     ErrorBarWidget (validation feedback).

All incoming engine events are dispatched via DungeonBridge → EventRouter.
All outgoing commands are sent via DungeonBridge.send_command().
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QDockWidget,
    QMainWindow,
    QStatusBar,
    QToolBar,
    QWidget,
)
from PySide6.QtCore import Qt

from app.dungeon_bridge import DungeonBridge
from app.event_router import EventRouter
from widgets.dungeon_board_widget import DungeonBoardWidget
from widgets.hero_panel_widget import HeroPanelWidget
from widgets.action_panel_widget import ActionPanelWidget
from widgets.log_widget import LogWidget
from widgets.error_bar_widget import ErrorBarWidget
from PySide6.QtWidgets import QApplication
from gmGui.theme_manager import ThemeManager
from gmGui.message_ids import AREA_INFO_REQUEST, AREA_INFO_RESPONSE
from gmGui.modules.gm_map_area_info_module import GmMapAreaInfoModule
from gmGui.modules.gm_flow_module import GmFlowModule


class DungeonMainWindow(QMainWindow):
    """Main application window for Dungeon Crawler Basic.

    Owns the engine bridge and routes all events to child widgets.
    No game logic is performed here; this class is pure presentation
    and orchestration.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Initialises the window, creates all child widgets and wires the bridge."""
        super().__init__(parent)
        app = QApplication.instance()
        if app is not None:
            theme_manager = ThemeManager(app)
            theme_manager.apply_theme("scroll")

        self.setWindowTitle("Dungeon Crawler Basic — GameLib")
        self.resize(1024, 700)
        screen = QApplication.primaryScreen()
        if screen is not None:
            geo = screen.availableGeometry()
            self.move(
                geo.center().x() - self.width() // 2,
                geo.center().y() - self.height() // 2,
            )
        self._build_layout()
        self._build_bridge()
        self._build_router()

    # ── Layout ───────────────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        """Creates and arranges all child widgets."""
        # ToBeImplemented //
        self._board     = DungeonBoardWidget()
        self._heroes    = HeroPanelWidget()
        self._actions   = ActionPanelWidget()
        self._log       = LogWidget()
        self._errors    = ErrorBarWidget()
        self._area_info = GmMapAreaInfoModule()
        self._flow      = GmFlowModule()

        self.setCentralWidget(self._board)

        top_dock = QDockWidget(self._flow.title, self)
        top_dock.setObjectName(self._flow.module_id)
        top_dock.setWidget(self._flow.widget())
        self.addDockWidget(Qt.TopDockWidgetArea, top_dock)
        self._flow.on_attach()

        right_dock = QDockWidget("Actors", self)
        right_dock.setWidget(self._heroes)
        self.addDockWidget(Qt.RightDockWidgetArea, right_dock)

        bottom_dock = QDockWidget("Actions", self)
        bottom_dock.setWidget(self._actions)
        self.addDockWidget(Qt.BottomDockWidgetArea, bottom_dock)

        left_dock = QDockWidget("Log", self)
        left_dock.setWidget(self._log)
        self.addDockWidget(Qt.LeftDockWidgetArea, left_dock)

        area_dock = QDockWidget("Area Info", self)
        area_dock.setWidget(self._area_info.widget())
        self.addDockWidget(Qt.LeftDockWidgetArea, area_dock)
        self.tabifyDockWidget(left_dock, area_dock)
        area_dock.raise_()

        self.setStatusBar(QStatusBar())
        self.statusBar().addPermanentWidget(self._errors)

        self._build_toolbar()

    def _build_toolbar(self) -> None:
        """Creates the main toolbar with New Game and other session controls."""
        from PySide6.QtWidgets import QToolBar
        from PySide6.QtGui import QAction
        toolbar = QToolBar("Session", self)
        self.addToolBar(toolbar)
        new_game_action = QAction("New Game", self)
        new_game_action.triggered.connect(self._on_new_game)
        toolbar.addAction(new_game_action)
        quit_action = QAction("Quit", self)
        quit_action.triggered.connect(self.close)
        toolbar.addAction(quit_action)

    # ── Bridge & router ───────────────────────────────────────────────────────

    def _build_bridge(self) -> None:
        """Creates and connects the DungeonBridge to the engine."""
        self._bridge = DungeonBridge()
        self._bridge.receiver.envelope_received.connect(self._on_envelope)
        self._bridge.receiver.start()

    def _build_router(self) -> None:
        """Creates the EventRouter and registers per-typeId handlers."""
        self._router = EventRouter()
        # Flow / Timeline: shared GmFlowModule fed via a dungeon.* → gmFlow.* adapter.
        self._flow.set_sender(self._bridge.sender)
        self._last_round: int = 0
        self._pending_move_hero: str = ""  # hero_id waiting for a destination click
        for ev in ("dungeon.session.started", "dungeon.turn.started",
                   "dungeon.turn.ended", "dungeon.game.over"):
            self._router.register(ev, self._on_flow_event)
        # Auto-select the active actor in the hero panel on every turn change.
        self._router.register("dungeon.turn.started", self._on_turn_started_select)
        # Board: map layout and actor movement
        for ev in ("dungeon.map.snapshot", "dungeon.actor.snapshot",
                   "dungeon.actor.moved", "dungeon.game.over"):
            self._router.register(ev, self._board.on_envelope)
        # Hero panel: actor state
        for ev in ("dungeon.actor.snapshot", "dungeon.actor.hp_changed",
                   "dungeon.actor.status_changed", "dungeon.session.started"):
            self._router.register(ev, self._heroes.on_envelope)
        # Action panel: button availability
        for ev in ("dungeon.actor.snapshot", "dungeon.turn.started",
                   "dungeon.turn.ended", "dungeon.game.over", "dungeon.session.started"):
            self._router.register(ev, self._actions.on_envelope)
        # Log: events of interest
        for ev in ("dungeon.session.started", "dungeon.actor.moved", "dungeon.actor.healed",
                   "dungeon.actor.equipped", "dungeon.action.rejected", "dungeon.game.over"):
            self._router.register(ev, self._log.on_envelope)
        # Error bar: rejections only
        self._router.register("dungeon.action.rejected", self._errors.on_envelope)
        # Area Info: contents of the selected map area (shared contract)
        self._router.register(AREA_INFO_RESPONSE, self._area_info.on_envelope)

        # Wire signals from widgets back to engine
        self._board.area_selected.connect(self._on_area_selected)
        self._actions.move_requested.connect(self._on_move_action)
        self._actions.heal_requested.connect(self._on_heal_requested)
        self._actions.equip_requested.connect(self._on_equip_requested)
        self._actions.end_turn_requested.connect(self._on_end_turn_requested)
        # Actor selection in hero panel → sync action panel display
        self._heroes.actor_selected.connect(self._actions.set_selected_actor)

    # ── Envelope handler (called by bridge on every incoming event) ───────────

    def _on_envelope(self, msg: dict) -> None:
        """Receives a decoded event envelope and dispatches it to the router."""
        self._router.dispatch(msg)

    def _on_turn_started_select(self, msg: dict) -> None:
        """Auto-selects the active actor; cancels any pending move targeting."""
        self._pending_move_hero = ""
        self._actions.set_awaiting_move(False)
        actor_id: str = str(msg.get("data", {}).get("actor_id", ""))
        if actor_id:
            self._board.set_active_hero(actor_id)
            self._heroes.select_actor(actor_id)

    # ── Flow adapter (dungeon.* → gmFlow.*) ───────────────────────────────────

    def _on_flow_event(self, msg: dict) -> None:
        """Translates dungeon lifecycle events into gmFlow envelopes.

        The shared :class:`GmFlowModule` understands the ``gmFlow.*`` contract.
        The dungeon CoreEngine emits its own ``dungeon.*`` events, so this thin
        adapter re-emits the equivalent flow envelopes locally (no C++ change,
        the wire contract is preserved). The ``Phase`` badge reflects the active
        actor, mirroring how Tic-Tac-Toe shows whose turn it is.
        """
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {}) or {}

        if tid == "dungeon.session.started":
            session_id: str = str(data.get("session_id", "?"))
            self._last_round = int(data.get("round", 1))
            self._flow.on_envelope(
                {"typeId": "gmFlow.session.started", "data": {"session_id": session_id}})
            self._flow.on_envelope(
                {"typeId": "gmFlow.round.started", "data": {"index": self._last_round}})

        elif tid == "dungeon.turn.started":
            actor_id: str = str(data.get("actor_id", "?"))
            round_no: int = int(data.get("round", self._last_round))
            if round_no != self._last_round:
                self._last_round = round_no
                self._flow.on_envelope(
                    {"typeId": "gmFlow.round.started", "data": {"index": round_no}})
            self._flow.on_envelope(
                {"typeId": "gmFlow.phase.entered", "data": {"phase_id": actor_id}})
            self._flow.on_envelope(
                {"typeId": "gmFlow.turn.started",
                 "data": {"turn_id": actor_id, "active_actors": [actor_id]}})

        elif tid == "dungeon.turn.ended":
            self._flow.on_envelope(
                {"typeId": "gmFlow.turn.ended",
                 "data": {"turn_id": str(data.get("actor_id", "?"))}})

        elif tid == "dungeon.game.over":
            self._flow.on_envelope({"typeId": "gmFlow.session.completed", "data": {}})

    # ── Command senders ───────────────────────────────────────────────────────

    def _on_area_selected(self, area_id: str) -> None:
        """Handles an area click: completes a pending move or requests area info."""
        if self._pending_move_hero:
            destination = self._board.move_destination()
            if destination:
                self._bridge.send_command("dungeon.move",
                    {"hero_id": self._pending_move_hero, "destination": destination})
                self._pending_move_hero = ""
                self._actions.set_awaiting_move(False)
            else:
                self._errors.on_envelope({
                    "typeId": "dungeon.action.rejected",
                    "data": {
                        "reason": "Area non adiacente. Seleziona una stanza adiacente all'eroe.",
                        "command": "dungeon.move",
                    },
                })
        if area_id:
            self._bridge.send_command(AREA_INFO_REQUEST, {"area_id": area_id})

    def _on_new_game(self) -> None:
        """Sends dungeon.new_game to CoreEngine."""
        self._log.clear()
        self._board.reset()
        self._heroes.reset()
        self._actions.reset()
        self._errors.clear()
        self._pending_move_hero = ""
        self._bridge.send_command("dungeon.new_game", {})

    def _on_move_action(self, hero_id: str) -> None:
        """Toggles move-targeting mode (enter on first press, cancel on second)."""
        if self._pending_move_hero:
            # Cancel: player pressed the button again while in targeting mode.
            self._pending_move_hero = ""
            self._actions.set_awaiting_move(False)
        else:
            # Enter targeting mode: wait for the player to click a destination room.
            self._pending_move_hero = hero_id
            self._actions.set_awaiting_move(True)

    def _on_heal_requested(self, hero_id: str, target_id: str) -> None:
        """Forwards a heal request to CoreEngine."""
        self._bridge.send_command("dungeon.heal",
            {"hero_id": hero_id, "target_id": target_id})

    def _on_equip_requested(self, hero_id: str, item_tag: str) -> None:
        """Forwards an equip request to CoreEngine."""
        self._bridge.send_command("dungeon.equip",
            {"hero_id": hero_id, "item_tag": item_tag})

    def _on_end_turn_requested(self, hero_id: str) -> None:
        """Forwards an end-turn request to CoreEngine."""
        self._bridge.send_command("dungeon.end_turn", {"hero_id": hero_id})

    def closeEvent(self, event) -> None:
        """Stops the bridge receiver before closing."""
        self._bridge.receiver.stop()
        self._bridge.receiver.wait(2000)
        super().closeEvent(event)
