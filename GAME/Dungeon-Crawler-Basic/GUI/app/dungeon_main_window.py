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
        self._build_layout()
        self._build_bridge()
        self._build_router()

    # ── Layout ───────────────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        """Creates and arranges all child widgets."""
        # ToBeImplemented //
        self._board   = DungeonBoardWidget()
        self._heroes  = HeroPanelWidget()
        self._actions = ActionPanelWidget()
        self._log     = LogWidget()
        self._errors  = ErrorBarWidget()

        self.setCentralWidget(self._board)

        right_dock = QDockWidget("Actors", self)
        right_dock.setWidget(self._heroes)
        self.addDockWidget(Qt.RightDockWidgetArea, right_dock)

        bottom_dock = QDockWidget("Actions", self)
        bottom_dock.setWidget(self._actions)
        self.addDockWidget(Qt.BottomDockWidgetArea, bottom_dock)

        left_dock = QDockWidget("Log", self)
        left_dock.setWidget(self._log)
        self.addDockWidget(Qt.LeftDockWidgetArea, left_dock)

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

        # Wire signals from widgets back to engine
        self._board.move_requested.connect(self._on_move_requested)
        self._actions.heal_requested.connect(self._on_heal_requested)
        self._actions.equip_requested.connect(self._on_equip_requested)

    # ── Envelope handler (called by bridge on every incoming event) ───────────

    def _on_envelope(self, msg: dict) -> None:
        """Receives a decoded event envelope and dispatches it to the router."""
        self._router.dispatch(msg)

    # ── Command senders ───────────────────────────────────────────────────────

    def _on_new_game(self) -> None:
        """Sends dungeon.new_game to CoreEngine."""
        self._log.clear()
        self._board.reset()
        self._heroes.reset()
        self._actions.reset()
        self._errors.clear()
        self._bridge.send_command("dungeon.new_game", {})

    def _on_move_requested(self, hero_id: str, destination: str) -> None:
        """Forwards a move request to CoreEngine."""
        self._bridge.send_command("dungeon.move",
            {"hero_id": hero_id, "destination": destination})

    def _on_heal_requested(self, hero_id: str, target_id: str) -> None:
        """Forwards a heal request to CoreEngine."""
        self._bridge.send_command("dungeon.heal",
            {"hero_id": hero_id, "target_id": target_id})

    def _on_equip_requested(self, hero_id: str, item_tag: str) -> None:
        """Forwards an equip request to CoreEngine."""
        self._bridge.send_command("dungeon.equip",
            {"hero_id": hero_id, "item_tag": item_tag})

    def closeEvent(self, event) -> None:
        """Stops the bridge receiver before closing."""
        self._bridge.receiver.stop()
        self._bridge.receiver.wait(2000)
        super().closeEvent(event)
