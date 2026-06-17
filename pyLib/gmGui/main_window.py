"""MainWindow — QMainWindow with dock manager and central event router.

Manages five QDockWidgets (one per module), wires the engine bridge, and
routes incoming envelopes to the correct module via a typeId routing table.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Slot
from PySide6.QtWidgets import QDockWidget, QLabel, QMainWindow

from . import settings
from .engine_bridge.receiver import EngineReceiver
from .engine_bridge.sender import EngineSender
from .modules.base_module import BaseModule
from .modules.gm_actor_module import GmActorModule
from .modules.gm_comp_deck_module import GmCompDeckModule
from .modules.gm_dice_module import GmDiceModule
from .modules.gm_flow_module import GmFlowModule
from .modules.gm_map_module import GmMapModule


class MainWindow(QMainWindow):
    """Application shell: registers modules as dock widgets and routes envelopes.

    Responsibilities:
    - Instantiate all five modules and inject the EngineSender.
    - Build the typeId routing table from each module's subscribed_type_ids().
    - Add every module as a QDockWidget in its default area.
    - Route incoming envelope dicts (via _on_envelope) to matching modules.
    - Persist and restore layout via settings.save/restore_layout() (Phase 9).
    - Shut down the bridge and modules cleanly on close.
    """

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("GameLib GUI")
        self.resize(1280, 800)
        self.setDockNestingEnabled(True)

        self._receiver: EngineReceiver = EngineReceiver()
        self._sender: EngineSender = EngineSender()

        self._modules: list[BaseModule] = []
        self._routing: dict[str, list[BaseModule]] = {}
        self._docks: dict[str, QDockWidget] = {}

        central = QLabel("GameLib – Engine View")
        central.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setCentralWidget(central)

        self._register_modules()
        # TODO: Phase 3 — wire receiver.envelope_received → self._on_envelope
        #                  and call self._receiver.start()

    def _register_modules(self) -> None:
        """Instantiates all modules, builds the routing table, and adds dock widgets.

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
        """Creates a QDockWidget for *mod* and registers it in the window layout.

        The ``objectName`` is set to ``mod.module_id`` so that
        ``QMainWindow.saveState()`` can identify each dock across restarts.
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

    @Slot(dict)
    def _on_envelope(self, msg: dict) -> None:
        """Routes an incoming envelope dict to all modules subscribed to its typeId.

        Called on the Qt main thread via a cross-thread Signal from EngineReceiver.
        Unknown typeIds are silently ignored.
        """
        tid: str = msg.get("typeId", "")
        for mod in self._routing.get(tid, []):
            mod.on_envelope(msg)

    def closeEvent(self, event) -> None:  # type: ignore[override]
        """Saves layout, detaches all modules, and stops the bridge before closing."""
        settings.save_layout(self)
        for mod in self._modules:
            mod.on_detach()
        self._receiver.stop()
        self._sender.close()
        super().closeEvent(event)
