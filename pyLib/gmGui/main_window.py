"""MainWindow — QMainWindow with dock manager and central event router.

Manages five QDockWidgets (one per module), wires the engine bridge,
routes incoming envelopes to modules, and persists the dock layout via
QSettings.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Slot
from PySide6.QtGui import QAction, QActionGroup
from PySide6.QtWidgets import (
    QApplication,
    QDockWidget,
    QLabel,
    QMainWindow,
    QMenu,
    QStatusBar,
)

from . import settings
from .engine_bridge.receiver import EngineReceiver
from .engine_bridge.sender import EngineSender
from .modules.base_module import BaseModule
from .modules.gm_actor_module import GmActorModule
from .modules.gm_comp_deck_module import GmCompDeckModule
from .modules.gm_dice_module import GmDiceModule
from .modules.gm_flow_module import GmFlowModule
from .modules.gm_map_module import GmMapModule
from .theme_manager import ThemeManager, _THEMES

# Text shown in the status bar when the C++ engine is not connected.
_STATUS_DISCONNECTED = "Engine: Disconnesso"
# Text shown after the first envelope arrives from the engine.
_STATUS_CONNECTED = "Engine: Connesso"
_DEFAULT_THEME_ID = "scroll"


class MainWindow(QMainWindow):
    """Application shell: registers modules, routes envelopes, persists layout.

    Responsibilities
    ----------------
    - Instantiate all five modules and inject ``EngineSender``.
    - Build the typeId routing table from each module's
      ``subscribed_type_ids()``.
    - Add every module as a ``QDockWidget`` in its default area.
    - Route incoming envelope dicts to matching modules via ``_on_envelope``.
    - Update the ``QStatusBar`` connection indicator on first envelope /
      disconnection.
    - Persist and restore geometry + dock layout via ``settings``.
    - Shut down the bridge and modules cleanly on close.
    """

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("GameLib GUI")
        self.resize(1280, 800)
        self.setDockNestingEnabled(True)

        # ── Bridge ────────────────────────────────────────────────────────────
        self._receiver: EngineReceiver = EngineReceiver()
        self._sender: EngineSender = EngineSender()

        # ── Module registry ───────────────────────────────────────────────────
        self._modules: list[BaseModule] = []
        self._routing: dict[str, list[BaseModule]] = {}
        self._docks: dict[str, QDockWidget] = {}
        self._theme_manager: ThemeManager = ThemeManager(QApplication.instance())
        self._theme_actions: dict[str, QAction] = {}

        # ── Central placeholder ───────────────────────────────────────────────
        central = QLabel("GameLib – Engine View")
        central.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setCentralWidget(central)

        # ── Build UI ──────────────────────────────────────────────────────────
        self._register_modules()
        self._build_menu()
        self._build_status_bar()
        
        # Apply theme AFTER all widgets are created to ensure proper propagation.
        self._set_theme(_DEFAULT_THEME_ID)
        
        # Force palette propagation to all child widgets (critical for Windows).
        self._theme_manager._propagate_theme_to_all_widgets(
            QApplication.instance(),
            self._theme_manager._build_palette(_THEMES[_DEFAULT_THEME_ID])
        )

        # ── Wire bridge signals ───────────────────────────────────────────────
        self._receiver.envelope_received.connect(self._on_envelope)
        self._receiver.connection_lost.connect(self._on_connection_lost)

        # Restore persisted layout (no-op until Phase 9 implements QSettings).
        settings.restore_layout(self)

        # Start the TCP server thread — waits for the C++ client to connect.
        self._receiver.start()

    # ── Module / dock setup ───────────────────────────────────────────────────

    def _register_modules(self) -> None:
        """Instantiates all modules, builds the routing table, adds dock widgets.

        Instantiation order determines the initial tab order for tabified docks.
        """
        self._modules = [
            GmFlowModule(),
            GmMapModule(),
            GmActorModule(),
            GmCompDeckModule(),
            GmDiceModule(),
        ]

        for mod in self._modules:
            mod.set_sender(self._sender)
            for tid in mod.subscribed_type_ids():
                self._routing.setdefault(tid, []).append(mod)
            self._add_dock(mod)
            mod.on_attach()

        # GmActorModule and GmCompDeckModule share the Right area as tabs.
        actor_dock: QDockWidget | None = self._docks.get("gm_actor")
        deck_dock: QDockWidget | None = self._docks.get("gm_comp_deck")
        if actor_dock is not None and deck_dock is not None:
            self.tabifyDockWidget(actor_dock, deck_dock)

    def _add_dock(self, mod: BaseModule) -> None:
        """Creates a ``QDockWidget`` for *mod* and adds it to the layout.

        The ``objectName`` is set to ``mod.module_id`` — this is required for
        ``QMainWindow.saveState()`` to correctly identify each dock across
        application restarts.
        """
        dock = QDockWidget(mod.title, self)
        dock.setObjectName(mod.module_id)
        dock.setWidget(mod.widget())
        dock.setFeatures(
            QDockWidget.DockWidgetFeature.DockWidgetMovable
            | QDockWidget.DockWidgetFeature.DockWidgetFloatable
            | QDockWidget.DockWidgetFeature.DockWidgetClosable
        )
        self.addDockWidget(mod.default_area, dock)
        self._docks[mod.module_id] = dock

    # ── Menu bar ──────────────────────────────────────────────────────────────

    def _build_menu(self) -> None:
        """Builds ``QMenuBar`` with View (dock toggles) and Help menus."""
        menu_bar = self.menuBar()

        # ── View menu — one toggle action per dock ────────────────────────────
        # Stored as instance attribute to prevent PySide6 from GC-ing the
        # Python wrapper and deleting the underlying C++ QMenu object.
        self._view_menu: QMenu = menu_bar.addMenu("&View")
        for dock in self._docks.values():
            # QDockWidget.toggleViewAction() returns a ready-made QAction that
            # shows/hides the dock and keeps its checked state in sync.
            self._view_menu.addAction(dock.toggleViewAction())

        # Theme submenu (single-choice, app-wide stylesheet).
        self._theme_menu: QMenu = self._view_menu.addMenu("&Theme")
        self._theme_group: QActionGroup = QActionGroup(self)
        self._theme_group.setExclusive(True)
        for theme in self._theme_manager.available_themes():
            action = QAction(theme.display_name, self)
            action.setCheckable(True)
            action.triggered.connect(
                lambda checked, theme_id=theme.theme_id: self._set_theme(theme_id)
                if checked else None
            )
            self._theme_group.addAction(action)
            self._theme_menu.addAction(action)
            self._theme_actions[theme.theme_id] = action

        # ── Help menu ─────────────────────────────────────────────────────────
        self._help_menu: QMenu = menu_bar.addMenu("&Help")
        about_action = QAction("&About gmGui", self)
        about_action.setStatusTip("About GameLib GUI")
        about_action.triggered.connect(self._on_about)
        self._help_menu.addAction(about_action)

    # ── Status bar ────────────────────────────────────────────────────────────

    def _build_status_bar(self) -> None:
        """Builds ``QStatusBar`` with a persistent connection-state label."""
        status_bar: QStatusBar = self.statusBar()
        self._conn_label = QLabel(_STATUS_DISCONNECTED)
        # Permanent widget: right-aligned, never overwritten by showMessage().
        status_bar.addPermanentWidget(self._conn_label)

    def _set_theme(self, theme_id: str) -> None:
        """Applies a global UI theme and keeps menu checks in sync."""
        self._theme_manager.apply_theme(theme_id)
        for current_id, action in self._theme_actions.items():
            action.setChecked(current_id == theme_id)

    # ── Envelope routing ──────────────────────────────────────────────────────

    @Slot(dict)
    def _on_envelope(self, msg: dict) -> None:
        """Routes an incoming envelope to all modules subscribed to its typeId.

        Called on the Qt main thread via the cross-thread ``envelope_received``
        Signal from ``EngineReceiver``.  Unknown typeIds are silently ignored.

        Also updates the status bar to *Connesso* on the first call.
        """
        # Mark connected on the first envelope received.
        if self._conn_label.text() != _STATUS_CONNECTED:
            self._conn_label.setText(_STATUS_CONNECTED)

        tid: str = msg.get("typeId", "")
        for mod in self._routing.get(tid, []):
            mod.on_envelope(msg)

    @Slot()
    def _on_connection_lost(self) -> None:
        """Resets the status bar to *Disconnesso* when the engine disconnects."""
        self._conn_label.setText(_STATUS_DISCONNECTED)

    # ── Help ─────────────────────────────────────────────────────────────────

    @Slot()
    def _on_about(self) -> None:
        """Shows a brief About message in the status bar."""
        self.statusBar().showMessage(
            "GameLib GUI — PySide6 frontend for the GameLib C++17 engine", 5000
        )

    # ── Lifecycle ─────────────────────────────────────────────────────────────

    def closeEvent(self, event) -> None:  # type: ignore[override]
        """Saves layout, detaches all modules, and stops the bridge before closing."""
        settings.save_layout(self)
        for mod in self._modules:
            mod.on_detach()
        self._receiver.stop()
        self._sender.close()
        super().closeEvent(event)
