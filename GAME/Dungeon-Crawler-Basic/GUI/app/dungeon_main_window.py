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


class DungeonMainWindow(QMainWindow):
    """Main application window for Dungeon Crawler Basic.

    Owns the engine bridge and routes all events to child widgets.
    No game logic is performed here; this class is pure presentation
    and orchestration.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        """Initialises the window, creates all child widgets and wires the bridge."""
        super().__init__(parent)
        self.setWindowTitle("Dungeon Crawler Basic — GameLib")
        self._build_layout()
        self._build_bridge()
        self._build_router()
        # ToBeImplemented //

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
        # ToBeImplemented //
        toolbar = QToolBar("Session", self)
        self.addToolBar(toolbar)

    # ── Bridge & router ───────────────────────────────────────────────────────

    def _build_bridge(self) -> None:
        """Creates and connects the DungeonBridge to the engine."""
        # ToBeImplemented //
        self._bridge = DungeonBridge()

    def _build_router(self) -> None:
        """Creates the EventRouter and registers per-typeId handlers."""
        # ToBeImplemented //
        self._router = EventRouter()

    # ── Envelope handler (called by bridge on every incoming event) ───────────

    def _on_envelope(self, msg: dict) -> None:
        """Receives a decoded event envelope and dispatches it to the router.

        Args:
            msg: Decoded JSON dict with at least ``typeId`` and ``data`` keys.
        """
        # ToBeImplemented //
        self._router.dispatch(msg)
