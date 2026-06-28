"""Le Pergamene di Eldhôm — main window.

EldhomMainWindow assembles the full application layout:

- QMenuBar:      Gioca (Nuova Missione / Visita Villaggio / Gestisci PG / Esci)
                 Impostazioni (Tema / File di Setting / Informazioni)
- Top dock:      TimelineWidget       (Linea Temporale continua — non chiudibile)
- Left dock:     LogWidget            (Messaggi — chiudibile)
- Centre:        EldhomMapWidget      (mappa locazioni con token attori)
- Right dock:    Actor Panel          (QDockWidget che contiene un inner QMainWindow)

Actor Panel inner layout (QMainWindow con Qt.WindowType.Widget):
  Central:       GmCompDeckModule     (tutte le zone del mazzo)
  Right dock:    EldhomActorAdapter   (albero attori gmActor, HP, stati, risorse)
  Bottom dock:   ActionPanelWidget    (Azioni Semplici ⌛)
               + HandWidget           (carte in mano cliccabili)

All incoming engine events are dispatched via EldhomBridge → EventRouter.
All outgoing commands are sent via EldhomBridge.send_command().
"""
from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtWidgets import (
    QApplication,
    QDockWidget,
    QFileDialog,
    QLabel,
    QMainWindow,
    QMessageBox,
    QStatusBar,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt, QTimer

from app.eldhom_bridge import EldhomBridge
from app.event_router import EventRouter
from app.mission_select_dialog import MissionSelectDialog
from widgets.eldhom_actor_adapter import EldhomActorAdapter
from widgets.action_panel_widget import ActionPanelWidget
from widgets.hand_widget import HandWidget
from widgets.map_widget import EldhomMapWidget
from widgets.timeline_widget import TimelineWidget
from widgets.log_widget import LogWidget

_GUI_DIR   = Path(__file__).resolve().parent
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"
for _p in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from gmGui.modules.gm_comp_deck_module import GmCompDeckModule  # noqa: E402

_DATA_DIR   = Path(__file__).resolve().parents[2] / "data"
_CARD_NAMES: dict[str, str] = {}


def _extract_data(msg: dict) -> dict:
    return msg.get("data", msg)


def _build_card_list(card_ids: list[str]) -> list[dict]:
    """Converts a list of card IDs into display dicts for HandWidget."""
    result: list[dict] = []
    for cid in card_ids:
        name = _CARD_NAMES.get(cid, cid.replace("_", " ").title())
        result.append({"card_id": cid, "name": name})
    return result


class _DeckProxy:
    """Intercepts outgoing gmAlea.deck.* commands from GmCompDeckModule.

    Translates ``gmAlea.deck.*`` type IDs to ``eldhom.deck.*`` before
    forwarding to the real bridge.  This keeps the generic deck module
    decoupled from game-specific command namespaces.
    """

    def __init__(self, bridge: EldhomBridge) -> None:
        self._bridge = bridge

    def send_command(self, type_id: str, data: dict) -> None:
        """Routes deck commands to the Eldhom bridge.

        Args:
            type_id: Command typeId string from GmCompDeckModule.
            data:    Command payload dict.
        """
        if type_id.startswith("gmAlea.deck."):
            eldhom_type = type_id.replace("gmAlea.deck.", "eldhom.deck.", 1)
            self._bridge.send_command(eldhom_type, data)
        else:
            self._bridge.send_command(type_id, data)




class EldhomMainWindow(QMainWindow):
    """Main application window for Le Pergamene di Eldhôm.

    No game logic lives here — only presentation and orchestration.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Le Pergamene di Eldhôm")
        self.resize(1280, 800)
        self._center_on_screen()

        self._active_hero_id: str = ""
        self._hero_data: dict[str, dict] = {}
        self._location_adjacency: dict[str, list[str]] = {}
        self._mission_started: bool = False
        self._hand_cards: dict[str, list[str]] = {}

        self._build_menu()
        self._build_layout()
        self._build_bridge()
        self._build_router()
        self._setup_status_bar()

        QTimer.singleShot(300, self._show_mission_select)

    # ── Menu (Windows-style QMenuBar) ─────────────────────────────────────────

    def _build_menu(self) -> None:
        """Builds the Windows-style QMenuBar.

        Gioca menu:        Inizia Nuova Missione | Visita un Villaggio |
                           Gestisci PG | ---- | Esci
        Impostazioni menu: Tema... | File di Setting... | ---- | Informazioni...
        """
        menubar = self.menuBar()

        # ── Gioca ──────────────────────────────────────────────────────────────
        play_menu = menubar.addMenu("Gioca")
        play_menu.addAction("Inizia Nuova Missione", self._on_new_mission)
        play_menu.addAction("Visita un Villaggio",   self._on_visit_village)
        play_menu.addAction("Gestisci PG",           self._on_manage_characters)
        play_menu.addSeparator()
        play_menu.addAction("Esci", self.close)

        # ── Impostazioni ───────────────────────────────────────────────────────
        settings_menu = menubar.addMenu("Impostazioni")
        settings_menu.addAction("Tema\u2026",             self._on_theme_settings)
        settings_menu.addAction("File di Setting\u2026",  self._on_file_settings)
        settings_menu.addSeparator()
        settings_menu.addAction("Informazioni\u2026",     self._on_about)

    # ── Layout ────────────────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        """Creates and arranges all child widgets as dockable panels.

        Main window layout:
          Top    : TimelineWidget       (QDockWidget — non chiudibile)
          Left   : LogWidget            (QDockWidget — Messaggi)
          Centre : EldhomMapWidget      (centralWidget)
          Right  : Actor Panel          (QDockWidget → inner QMainWindow)

        Actor Panel inner layout (QMainWindow with Qt.WindowType.Widget):
          Central:  GmCompDeckModule    (tutte le zone del mazzo)
          Right:    EldhomActorAdapter  (albero attori, HP, stati, risorse)
          Bottom:   ActionPanelWidget   (Azioni Semplici \u23f3)
                  + HandWidget          (carte in mano cliccabili)
        """
        self.setDockOptions(
            QMainWindow.DockOption.AllowTabbedDocks
            | QMainWindow.DockOption.AnimatedDocks
            | QMainWindow.DockOption.AllowNestedDocks
            | QMainWindow.DockOption.GroupedDragging
        )
        self.setDockNestingEnabled(True)

        # ── Central: Map ──────────────────────────────────────────────────────
        self._map_widget = EldhomMapWidget(self)
        self.setCentralWidget(self._map_widget)

        # ── Top: Timeline ──────────────────────────────────────────────────────
        self._timeline = TimelineWidget(self)
        timeline_dock = QDockWidget("Linea Temporale", self)
        timeline_dock.setObjectName("dock_timeline")
        timeline_dock.setWidget(self._timeline)
        timeline_dock.setFeatures(
            QDockWidget.DockWidgetFeature.DockWidgetMovable
            | QDockWidget.DockWidgetFeature.DockWidgetFloatable
        )
        self.addDockWidget(Qt.DockWidgetArea.TopDockWidgetArea, timeline_dock)

        # ── Left: Log ─────────────────────────────────────────────────────────
        self._log_widget = LogWidget(self)
        log_dock = QDockWidget("Messaggi", self)
        log_dock.setObjectName("dock_log")
        log_dock.setWidget(self._log_widget)
        log_dock.setFeatures(
            QDockWidget.DockWidgetFeature.DockWidgetMovable
            | QDockWidget.DockWidgetFeature.DockWidgetFloatable
            | QDockWidget.DockWidgetFeature.DockWidgetClosable
        )
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, log_dock)

        # ── Right: Actor Panel (inner QMainWindow) ─────────────────────────────
        self._deck    = GmCompDeckModule()
        self._deck.set_enforce_action_cost(False)
        self._actors  = EldhomActorAdapter()
        self._actions = ActionPanelWidget()
        self._hand    = HandWidget()

        # Bottom container: ActionPanelWidget stacked above HandWidget
        bottom_widget = QWidget()
        bottom_vbox = QVBoxLayout(bottom_widget)
        bottom_vbox.setContentsMargins(0, 0, 0, 0)
        bottom_vbox.setSpacing(0)
        bottom_vbox.addWidget(self._actions)
        bottom_vbox.addWidget(self._hand)

        # Inner QMainWindow owns the sub-docks of the Actor Panel
        self._actor_inner = QMainWindow()
        self._actor_inner.setWindowFlags(Qt.WindowType.Widget)
        self._actor_inner.setDockNestingEnabled(True)
        self._actor_inner.setDockOptions(
            QMainWindow.DockOption.AllowTabbedDocks
            | QMainWindow.DockOption.AnimatedDocks
            | QMainWindow.DockOption.AllowNestedDocks
        )

        # Inner central: Deck Manager
        self._actor_inner.setCentralWidget(self._deck.widget())
        self._deck.on_attach()

        # Inner right: Actor tree + details
        actors_inner_dock = QDockWidget("Attori", self._actor_inner)
        actors_inner_dock.setObjectName("inner_dock_actors")
        actors_inner_dock.setWidget(self._actors)
        self._actor_inner.addDockWidget(
            Qt.DockWidgetArea.RightDockWidgetArea, actors_inner_dock
        )

        # Inner bottom: Azioni Semplici + Mano
        actions_inner_dock = QDockWidget("Azioni & Mano", self._actor_inner)
        actions_inner_dock.setObjectName("inner_dock_actions")
        actions_inner_dock.setWidget(bottom_widget)
        self._actor_inner.addDockWidget(
            Qt.DockWidgetArea.BottomDockWidgetArea, actions_inner_dock
        )

        # Proportion: actor tree ~280 px wide
        self._actor_inner.resizeDocks(
            [actors_inner_dock], [280], Qt.Orientation.Horizontal
        )

        # Outer dock: the entire Actor Panel — floatable and dockable
        self._actor_dock = QDockWidget("Actor Panel", self)
        self._actor_dock.setObjectName("dock_actor_panel")
        self._actor_dock.setWidget(self._actor_inner)
        self._actor_dock.setFeatures(
            QDockWidget.DockWidgetFeature.DockWidgetMovable
            | QDockWidget.DockWidgetFeature.DockWidgetFloatable
        )
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self._actor_dock)

        # Give Actor Panel ~500 px initial width
        self.resizeDocks(
            [self._actor_dock], [500], Qt.Orientation.Horizontal
        )

        # ── Signal wiring ──────────────────────────────────────────────────────
        self._actors.actor_selected.connect(self._on_actor_selected)
        self._actions.action_move.connect(self._on_simple_move)
        self._actions.action_attack.connect(self._on_simple_attack)
        self._actions.action_interact.connect(self._on_simple_interact)
        self._actions.action_recover.connect(self._on_simple_recover)
        self._actions.stop_sequence.connect(self._on_stop_sequence)
        self._hand.card_selected.connect(self._on_card_selected)

    def _build_bridge(self) -> None:
        self._bridge = EldhomBridge()
        self._bridge.receiver.start()
        # Wire deck module outgoing commands through the proxy translator
        self._deck.set_sender(_DeckProxy(self._bridge))
        print("[EldhomGUI] Event receiver listening on port 9210", flush=True)
        self._bridge.set_on_event(self._on_engine_event_thread)

    def _build_router(self) -> None:
        self._router = EventRouter()

        def reg(type_id: str, handler):
            self._router.register(type_id, handler)

        # Full state
        reg("eldhom.state.full",              self._on_state_full)
        reg("eldhom.state.full",              self._map_widget.on_state_full)
        reg("eldhom.state.full",              self._timeline.on_state_full)
        reg("eldhom.state.full",              self._on_deck_state_full)

        # Turn management
        reg("eldhom.turn.next_actor",         self._on_next_actor)
        reg("eldhom.turn.next_actor",         self._timeline.on_next_actor)
        reg("eldhom.pg.turn_ended",           self._on_pg_turn_ended)

        # Actor events → EldhomActorAdapter
        for evt in (
            "eldhom.state.full",
            "eldhom.pg.ko",
            "eldhom.pg.healed",
            "eldhom.pg.moved",
            "eldhom.monster.damaged",
            "eldhom.monster.defeated",
            "eldhom.mission.time_advanced",
        ):
            reg(evt, self._actors.on_envelope)

        # Map
        reg("eldhom.monster.defeated",        self._map_widget.on_monster_defeated)

        # Deck events → GmCompDeckModule (translated)
        reg("eldhom.deck.hand_updated",       self._on_hand_updated)
        reg("eldhom.deck.reshuffled",         self._on_deck_reshuffled)
        reg("eldhom.pg.played_card",          self._on_pg_played_card)

        # Mission outcome
        reg("eldhom.mission.victory",         self._on_victory)
        reg("eldhom.mission.defeat",          self._on_defeat)
        reg("eldhom.action.result",           self._on_action_result)

        # Timeline advancement
        reg("eldhom.mission.time_advanced",   self._on_time_advanced)

        # Log (all interesting events)
        for evt in (
            "eldhom.pg.played_card", "eldhom.pg.moved", "eldhom.pg.attacked",
            "eldhom.pg.healed", "eldhom.pg.ko", "eldhom.monster.damaged",
            "eldhom.monster.defeated", "eldhom.group.activated",
            "eldhom.group.eliminated", "eldhom.formation.changed",
            "eldhom.deck.reshuffled", "eldhom.mission.time_advanced",
        ):
            reg(evt, self._log_widget.on_any_event)
        reg("eldhom.action.result",           self._log_widget.on_action_result)
        reg("eldhom.mission.victory",         self._log_widget.on_mission_victory)
        reg("eldhom.mission.defeat",          self._log_widget.on_mission_defeat)

    def _setup_status_bar(self) -> None:
        sb = QStatusBar(self)
        self._status_label = QLabel("Non connesso \u2014 avvia eldhom_engine.exe", self)
        self._status_label.setProperty("text_role", "secondary")
        sb.addWidget(self._status_label)
        self.setStatusBar(sb)

    # ── Event routing (thread-safe) ────────────────────────────────────────────

    def _on_engine_event_thread(self, msg: dict) -> None:
        """Called from the receiver background thread — routed via QTimer."""
        type_id = msg.get("typeId", "?")
        print(f"[EldhomGUI] \u2713 {type_id}", flush=True)
        QTimer.singleShot(0, lambda: self._router.dispatch(msg))

    # ── State event handlers ───────────────────────────────────────────────────

    def _on_state_full(self, msg: dict) -> None:
        data = _extract_data(msg)
        self._mission_started = True
        title = data.get("title", "Le Pergamene di Eldhôm")
        self.setWindowTitle(f"Le Pergamene di Eldhôm \u2014 {title}")
        self._status_label.setText(
            f"Missione: {title}  \u2022  \u231b {data.get('time', 0)}"
        )
        self._hero_data.clear()
        for hero in data.get("heroes", []):
            hid = hero["id"]
            self._hero_data[hid] = hero
            self._hand_cards[hid] = hero.get("hand", [])
        self._location_adjacency.clear()
        for loc in data.get("locations", []):
            self._location_adjacency[loc["id"]] = loc.get("adjacent", [])
        next_a = data.get("next_actor", {})
        if next_a:
            self._activate_actor(next_a.get("actor_id", ""), next_a.get("kind", ""))

    def _on_next_actor(self, msg: dict) -> None:
        data = _extract_data(msg)
        self._activate_actor(data.get("actor_id", ""), data.get("kind", ""))

    def _activate_actor(self, actor_id: str, kind: str) -> None:
        """Highlights the active actor and enables/disables action controls."""
        self._active_hero_id = actor_id if kind == "HERO" else ""
        self._actors.select_actor(actor_id)
        if kind == "HERO" and actor_id in self._hero_data:
            hero = self._hero_data[actor_id]
            loc  = hero.get("location", "")
            adj  = self._location_adjacency.get(loc, [])
            self._actions.set_turn(hero.get("name", actor_id), adj)
            hand_ids = self._hand_cards.get(actor_id, [])
            self._hand.set_hand(
                hero.get("name", actor_id),
                _build_card_list(hand_ids),
                enabled=True,
            )
        else:
            self._actions.set_enabled(False)
            self._hand.set_enabled(False)

    def _on_pg_turn_ended(self, msg: dict) -> None:
        data = _extract_data(msg)
        if data.get("actor_id", "") == self._active_hero_id:
            self._actions.set_enabled(False)
            self._hand.set_enabled(False)

    # ── Deck event translation (eldhom.* → GmCompDeckModule) ─────────────────

    def _on_deck_state_full(self, msg: dict) -> None:
        """Translates hero hand data from full state for GmCompDeckModule."""
        data = _extract_data(msg)
        for hero in data.get("heroes", []):
            hand_ids = [str(c) for c in hero.get("hand", [])]
            cards = [{"card_id": c, "name": c, "action_cost": 1} for c in hand_ids]
            self._deck.on_envelope({
                "typeId": "gmAlea.deck.zone_changed",
                "data":   {"zone_name": "CardHand", "cards": cards},
            })

    def _on_hand_updated(self, msg: dict) -> None:
        data     = _extract_data(msg)
        hero_id  = data.get("actor_id", "")
        payload  = data.get("payload", [])
        hand_ids = [str(c) for c in payload] if isinstance(payload, list) else []
        self._hand_cards[hero_id] = hand_ids
        if hero_id in self._hero_data:
            self._hero_data[hero_id]["hand"] = hand_ids
        if hero_id == self._active_hero_id:
            name = self._hero_data.get(hero_id, {}).get("name", hero_id)
            self._hand.set_hand(name, _build_card_list(hand_ids), enabled=True)
        cards = [{"card_id": c, "name": c, "action_cost": 1} for c in hand_ids]
        self._deck.on_envelope({
            "typeId": "gmAlea.deck.zone_changed",
            "data":   {"zone_name": "CardHand", "cards": cards},
        })

    def _on_deck_reshuffled(self, msg: dict) -> None:
        self._deck.on_envelope({
            "typeId": "gmAlea.deck.shuffled",
            "data":   {"zone_name": "MainDeck"},
        })

    def _on_pg_played_card(self, msg: dict) -> None:
        data    = _extract_data(msg)
        card_id = str(data.get("card_id", ""))
        if card_id:
            self._deck.on_envelope({
                "typeId": "gmAlea.deck.card_moved",
                "data":   {
                    "card_id":   card_id,
                    "from_zone": "CardHand",
                    "to_zone":   "PlayArea",
                },
            })

    # ── Misc event handlers ────────────────────────────────────────────────────

    def _on_time_advanced(self, msg: dict) -> None:
        data     = _extract_data(msg)
        time_val = data.get("payload", 0)
        self._status_label.setText(
            f"Missione: {self.windowTitle().split(' \u2014 ')[-1]}  \u2022  \u231b {time_val}"
        )

    def _on_action_result(self, msg: dict) -> None:
        data = _extract_data(msg)
        if not data.get("ok", True):
            self._status_label.setText(f"\u26a0 {data.get('error', 'Errore')}")

    def _on_actor_selected(self, actor_id: str) -> None:
        """Called when the user selects an actor in the tree."""
        pass  # Future: switch deck view to selected actor's deck

    def _on_victory(self, msg: dict) -> None:
        self._actions.set_enabled(False)
        self._hand.set_enabled(False)
        QTimer.singleShot(
            200,
            lambda: QMessageBox.information(
                self, "Vittoria!", "Missione completata con successo!"
            ),
        )

    def _on_defeat(self, msg: dict) -> None:
        data    = _extract_data(msg)
        payload = data.get("payload", "")
        self._actions.set_enabled(False)
        self._hand.set_enabled(False)
        QTimer.singleShot(
            200,
            lambda: QMessageBox.warning(
                self, "Sconfitta", f"Missione fallita!\n{payload}"
            ),
        )

    # ── Player action senders ──────────────────────────────────────────────────

    def _on_card_selected(self, card_id: str) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.play_card",
            {"hero_id": self._active_hero_id, "card_id": card_id},
        )

    def _on_simple_move(self, destination: str) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.simple_action",
            {
                "hero_id":     self._active_hero_id,
                "action_type": "MOVE",
                "destination": destination,
            },
        )

    def _on_simple_attack(self) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.simple_action",
            {"hero_id": self._active_hero_id, "action_type": "ATTACK"},
        )

    def _on_simple_interact(self) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.simple_action",
            {"hero_id": self._active_hero_id, "action_type": "INTERACT"},
        )

    def _on_simple_recover(self) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.simple_action",
            {"hero_id": self._active_hero_id, "action_type": "RECOVER"},
        )

    def _on_stop_sequence(self) -> None:
        if not self._active_hero_id:
            return
        self._bridge.send_command(
            "eldhom.stop_sequence",
            {"hero_id": self._active_hero_id},
        )

    # ── Menu handlers ──────────────────────────────────────────────────────────

    def _on_new_mission(self) -> None:
        """Opens the mission selection dialog."""
        self._show_mission_select()

    def _on_visit_village(self) -> None:
        QMessageBox.information(
            self, "Visita Villaggio", "Funzionalit\u00e0 non ancora disponibile."
        )

    def _on_manage_characters(self) -> None:
        QMessageBox.information(
            self, "Gestisci PG", "Funzionalit\u00e0 non ancora disponibile."
        )

    def _on_theme_settings(self) -> None:
        try:
            from gmGui.theme_manager import ThemeManager
        except ImportError:
            QMessageBox.warning(self, "Tema", "ThemeManager non disponibile.")
            return
        themes = ["scroll", "stone", "dark_moon", "dungeon", "slate"]
        from PySide6.QtWidgets import QInputDialog
        theme, ok = QInputDialog.getItem(
            self, "Scegli Tema", "Tema:", themes, 0, False
        )
        if ok and theme:
            app = QApplication.instance()
            if app:
                ThemeManager(app).apply_theme(theme)

    def _on_file_settings(self) -> None:
        path, _ = QFileDialog.getOpenFileName(
            self, "Apri File di Setting", "",
            "JSON (*.json);;Tutti i file (*)"
        )
        if path:
            QMessageBox.information(
                self, "File di Setting", f"File selezionato:\n{path}"
            )

    def _on_about(self) -> None:
        QMessageBox.information(
            self,
            "Informazioni",
            "Le Pergamene di Eldhôm\nVersione 0.1.0\n"
            "Game-Lib \u2014 framework motore C++17 + GUI PySide6",
        )

    # ── Mission dialog ─────────────────────────────────────────────────────────

    def _show_mission_select(self) -> None:
        dialog = MissionSelectDialog(data_dir=str(_DATA_DIR), parent=self)
        if dialog.exec() and dialog.selected_mission_id:
            self._bridge.send_command(
                "eldhom.session.start",
                {"mission_id": dialog.selected_mission_id},
            )

    # ── Helpers ────────────────────────────────────────────────────────────────

    def _center_on_screen(self) -> None:
        screen = QApplication.primaryScreen()
        if screen is not None:
            geo = screen.availableGeometry()
            self.move(
                geo.center().x() - self.width() // 2,
                geo.center().y() - self.height() // 2,
            )

