"""TrisMainWindow — hybrid Tic-Tac-Toe shell built on the gmGui modules.

Layout
------
- **Central widget**: the interactive :class:`GmTrisBoardModule` (clickable 3x3
  board — the playable adaptation of the gmMap visualisation).
- **Docks** (read-only dashboards, reusing the generic gmGui modules):
  - :class:`GmFlowModule`  — session / phase timeline (top)
  - :class:`GmActorModule` — players, turn & result statuses (right)
  - :class:`GmDiceModule`  — 1d2 starter roll result (bottom)

All game logic lives in the C++ CoreEngine.  This window only renders engine
events (translated for the dashboards by :class:`TrisEventAdapter`) and turns
user actions into ``gmTris.move`` / ``gmTris.new_game`` commands.  The C++ engine
and its wire contract are left untouched.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Slot
from PySide6.QtWidgets import (
    QComboBox,
    QDockWidget,
    QLabel,
    QMainWindow,
    QPushButton,
    QToolBar,
)

from gmGui.modules.base_module import BaseModule
from gmGui.modules.gm_actor_module import GmActorModule
from gmGui.modules.gm_dice_module import GmDiceModule
from gmGui.modules.gm_flow_module import GmFlowModule

from app.tris_bridge import TrisBridge
from app.tris_event_adapter import TrisEventAdapter
from modules.gm_tris_board_module import GmTrisBoardModule

_STATUS_CONNECTED = "Engine: Connesso"
_STATUS_DISCONNECTED = "Engine: Disconnesso"


class TrisMainWindow(QMainWindow):
    """Application shell wiring the bridge, the board module and the dashboards."""

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Tic-Tac-Toe — GameLib (moduli gmGui)")
        self.resize(1100, 760)
        self.setDockNestingEnabled(True)

        self._bridge: TrisBridge = TrisBridge()
        self._adapter: TrisEventAdapter = TrisEventAdapter()

        # The clickable board is the interactive centrepiece.
        self._board_module: GmTrisBoardModule = GmTrisBoardModule()
        # Read-only dashboards reusing the generic gmGui modules.
        self._dashboards: list[BaseModule] = [
            GmFlowModule(),
            GmActorModule(),
            GmDiceModule(),
        ]
        self._routing: dict[str, list[BaseModule]] = {}

        self._inject_sender()
        self._build_toolbar()
        self._build_central()
        self._build_docks()
        self._wire_signals()

        self.statusBar().showMessage(_STATUS_DISCONNECTED)
        self._bridge.start()

    # ── Setup ─────────────────────────────────────────────────────────────────

    def _inject_sender(self) -> None:
        """Injects the shared EngineSender into every module."""
        self._board_module.set_sender(self._bridge.sender)
        for mod in self._dashboards:
            mod.set_sender(self._bridge.sender)
            for tid in mod.subscribed_type_ids():
                self._routing.setdefault(tid, []).append(mod)

    def _build_toolbar(self) -> None:
        toolbar = QToolBar("Comandi")
        toolbar.setMovable(False)
        self.addToolBar(toolbar)

        self._save_button = QPushButton("Salvataggio")
        self._reload_button = QPushButton("Nuova partita")
        self._starter_combo = QComboBox()
        self._starter_combo.addItem("Inizio: X (fisso)", "fixed_x")
        self._starter_combo.addItem("Inizio: Dado 1d2", "dice_1d2")

        toolbar.addWidget(self._save_button)
        toolbar.addWidget(self._reload_button)
        toolbar.addSeparator()
        toolbar.addWidget(QLabel("Modalità:"))
        toolbar.addWidget(self._starter_combo)

    def _build_central(self) -> None:
        self.setCentralWidget(self._board_module.widget())
        self._board_module.on_attach()

    def _build_docks(self) -> None:
        areas: dict[str, Qt.DockWidgetArea] = {
            "gm_flow": Qt.DockWidgetArea.TopDockWidgetArea,
            "gm_actor": Qt.DockWidgetArea.RightDockWidgetArea,
            "gm_dice": Qt.DockWidgetArea.BottomDockWidgetArea,
        }
        for mod in self._dashboards:
            dock = QDockWidget(mod.title, self)
            dock.setObjectName(mod.module_id)
            dock.setWidget(mod.widget())
            self.addDockWidget(areas.get(mod.module_id, mod.default_area), dock)
            mod.on_attach()

    def _wire_signals(self) -> None:
        self._bridge.envelope_received.connect(self._on_envelope)
        self._bridge.connection_lost.connect(self._on_connection_lost)
        self._reload_button.clicked.connect(self._on_reload)
        self._save_button.clicked.connect(self._on_save)

    # ── User actions ──────────────────────────────────────────────────────────

    @Slot()
    def _on_reload(self) -> None:
        mode: str = self._starter_combo.currentData()
        self._bridge.send_new_game(mode)

    @Slot()
    def _on_save(self) -> None:
        self.statusBar().showMessage("Salvataggio non ancora disponibile (Fase 4).")

    # ── Envelope routing ──────────────────────────────────────────────────────

    @Slot(dict)
    def _on_envelope(self, msg: dict) -> None:
        if self.statusBar().currentMessage() != _STATUS_CONNECTED:
            self.statusBar().showMessage(_STATUS_CONNECTED)

        # 1. The board consumes the native Tris contract directly.
        self._board_module.on_envelope(msg)

        # 2. The dashboards consume the translated contract.
        for env in self._adapter.translate(msg):
            for mod in self._routing.get(env.get("typeId", ""), []):
                mod.on_envelope(env)

    @Slot()
    def _on_connection_lost(self) -> None:
        self.statusBar().showMessage(_STATUS_DISCONNECTED)

    # ── Lifecycle ─────────────────────────────────────────────────────────────

    def closeEvent(self, event) -> None:  # type: ignore[override]
        self._board_module.on_detach()
        for mod in self._dashboards:
            mod.on_detach()
        self._bridge.stop()
        super().closeEvent(event)
