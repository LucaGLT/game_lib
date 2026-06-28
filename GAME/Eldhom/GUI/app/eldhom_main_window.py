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
               + AreaInfoWidget       (info locazione selezionata sulla mappa)
  Bottom dock:   ActionPanelWidget    (Azioni Base ⌛)

All incoming engine events are dispatched via EldhomBridge → EventRouter.
All outgoing commands are sent via EldhomBridge.send_command().
"""
from __future__ import annotations

import sys
import threading
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
from PySide6.QtCore import Qt, QTimer, Signal

from app.eldhom_bridge import EldhomBridge
from app.event_router import EventRouter
from app.mission_select_dialog import MissionSelectDialog
from widgets.eldhom_actor_adapter import EldhomActorAdapter
from widgets.action_panel_widget import ActionPanelWidget
from widgets.area_info_widget import AreaInfoWidget
from widgets.board_widget import EldhomBoardWidget
from widgets.timeline_widget import TimelineWidget
from widgets.log_widget import LogWidget
from widgets.instant_window_dialog import InstantWindowDialog
from widgets.formation_dialog      import FormationDialog
_GUI_DIR   = Path(__file__).resolve().parent
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"
for _p in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from gmGui.modules.gm_comp_deck_module import GmCompDeckModule  # noqa: E402

import json as _json  # noqa: E402

_DATA_DIR   = Path(__file__).resolve().parents[2] / "data"

# Card catalog (loaded from data/cards_base.json) keyed by card_id.
_CARD_CATALOG: dict[str, dict] = {}

_CARD_TYPE_IT: dict[str, str] = {
    "SINGLE":       "Singola",
    "INSTANT":      "Istantanea",
    "SEQ_START":    "Inizio sequenza",
    "SEQ_CONTINUE": "Continua sequenza",
    "SEQ_END":      "Chiudi sequenza",
}

_EFFECT_IT: dict[str, str] = {
    "DAMAGE":         "Danno",
    "HEAL":           "Cura",
    "MOVE":           "Movimento",
    "FORMATION_PUSH": "Spinta formazione",
}

_TARGET_IT: dict[str, str] = {
    "NEAREST_ENEMY_FRONTLINE": "nemico frontline più vicino",
    "ADJACENT_LOCATION":       "locazione adiacente",
    "SELF":                    "sé stesso",
}


def _load_card_catalog() -> None:
    """Loads data/cards_base.json into the module-level catalog (once)."""
    if _CARD_CATALOG:
        return
    path = _DATA_DIR / "cards_base.json"
    try:
        raw = _json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return
    for card in raw:
        cid = str(card.get("card_id", ""))
        if cid:
            _CARD_CATALOG[cid] = card


def _extract_data(msg: dict) -> dict:
    """Returns the payload dict from an event envelope."""
    return msg.get("data", msg)


def _card_name(card_id: str) -> str:
    """Returns the display name for *card_id* (falls back to a prettified id)."""
    card = _CARD_CATALOG.get(card_id)
    if card and card.get("name"):
        return str(card["name"])
    return card_id.replace("_", " ").title()


def _card_tags(card: dict) -> list[str]:
    """Builds the tag list for a card: type + affiliation/origin."""
    tags: list[str] = []
    ctype = str(card.get("card_type", ""))
    if ctype:
        tags.append(_CARD_TYPE_IT.get(ctype, ctype))
    origin = str(card.get("origin", ""))
    if origin:
        tags.append(origin)
    return tags


def _card_description(card: dict) -> str:
    """Synthesises a human-readable detail block for a card.

    The block lists the fixed elements requested in the UI: type, time cost,
    affiliation/origin tags and the list of effects.  It is fed to the deck
    module's detail panel via the card metadata ``description`` field.
    """
    lines: list[str] = []
    ctype = str(card.get("card_type", ""))
    lines.append(f"Tipo: {_CARD_TYPE_IT.get(ctype, ctype or '—')}")
    lines.append(f"Costo: {card.get('timeline_cost', 0)}\u23f3")
    origin = str(card.get("origin", ""))
    if origin:
        lines.append(f"Affiliazione: {origin}")
    lines.append(
        "Bersaglio retro: " + ("Sì" if card.get("can_target_backline") else "No")
    )
    trigger = str(card.get("reaction_trigger", ""))
    if trigger:
        lines.append(f"Reazione a: {trigger}")

    effects = card.get("effects", [])
    if effects:
        lines.append("")
        lines.append("Effetti:")
        for eff in effects:
            etype  = str(eff.get("effect_type", ""))
            amount = eff.get("amount", "")
            target = str(eff.get("target", ""))
            etxt   = _EFFECT_IT.get(etype, etype)
            ttxt   = _TARGET_IT.get(target, target)
            piece  = f"  \u2022 {etxt}"
            if amount not in ("", None):
                piece += f" {amount}"
            if ttxt:
                piece += f" \u2192 {ttxt}"
            lines.append(piece)
    return "\n".join(lines)


def _card_meta(card_id: str) -> dict:
    """Builds the deck-module metadata dict for *card_id* from the catalog."""
    card = _CARD_CATALOG.get(card_id, {})
    return {
        "card_id":     card_id,
        "name":        _card_name(card_id),
        "action_cost": int(card.get("timeline_cost", 1)),
        "tags":        _card_tags(card),
        "description": _card_description(card) if card else "",
    }



class _DeckProxy:
    """Intercepts outgoing gmAlea.deck.* commands from GmCompDeckModule.

    The Eldhôm CoreEngine does not understand generic ``gmAlea.deck.move_card``
    commands.  Playing a card is modelled engine-side as ``eldhom.play_card``.
    This proxy therefore converts a *play* move (CardHand → PlayArea / Memory)
    into an ``eldhom.play_card`` command for the active hero, and drops any
    other deck move (the engine remains the single source of truth for zones).

    Args:
        bridge:        The Eldhom bridge used to send commands.
        active_hero:   Callable returning the current active hero id (may be "").
    """

    _PLAY_ZONES: frozenset[str] = frozenset({"PlayArea", "Memory"})

    def __init__(self, bridge: EldhomBridge, active_hero, pre_play_hook=None) -> None:
        self._bridge       = bridge
        self._active_hero  = active_hero
        self._pre_play_hook = pre_play_hook

    def send_command(self, type_id: str, data: dict) -> None:
        """Routes deck commands to the Eldhom bridge.

        Args:
            type_id: Command typeId string from GmCompDeckModule.
            data:    Command payload dict.
        """
        if type_id == "gmAlea.deck.move_card":
            from_zone = str(data.get("from", ""))
            to_zone   = str(data.get("to", ""))
            card_id   = str(data.get("card_id", ""))
            if from_zone == "CardHand" and to_zone in self._PLAY_ZONES:
                hero_id = self._active_hero()
                if hero_id and card_id:
                    # Let the hook check if this is a MOVE card that needs targeting.
                    if self._pre_play_hook and self._pre_play_hook(hero_id, card_id):
                        return  # Hook armed targeting; don't send play_card yet.
                    self._bridge.send_command(
                        "eldhom.play_card",
                        {"hero_id": hero_id, "card_id": card_id},
                    )
            # Non-play moves are ignored: the engine drives zone changes.
            return
        if type_id.startswith("gmAlea.deck."):
            eldhom_type = type_id.replace("gmAlea.deck.", "eldhom.deck.", 1)
            self._bridge.send_command(eldhom_type, data)
        else:
            self._bridge.send_command(type_id, data)





class EldhomMainWindow(QMainWindow):
    """Main application window for Le Pergamene di Eldhôm.

    No game logic lives here — only presentation and orchestration.
    """

    # Used by _probe_engine (background thread) to safely update the status
    # bar from the main Qt thread via the signal/slot mechanism.
    _engine_status = Signal(str)

    def __init__(self, parent: QWidget | None = None,
                 bridge: EldhomBridge | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Le Pergamene di Eldhôm")
        self.resize(1280, 800)
        self._center_on_screen()

        # Optional pre-built bridge (created early in main.py so the event
        # receiver on port 9210 is bound before the window is shown).
        self._bridge: EldhomBridge | None = bridge

        _load_card_catalog()

        self._active_hero_id: str = ""
        self._hero_data: dict[str, dict] = {}
        self._location_adjacency: dict[str, list[str]] = {}
        self._location_names: dict[str, str] = {}
        self._group_data: dict[str, dict] = {}
        self._mission_started: bool = False
        self._hand_cards: dict[str, list[str]] = {}

        # Move targeting state: when True the next map click is a move
        # destination for the active hero.
        self._awaiting_move: bool = False

        # When a MOVE-effect card was dragged to PlayArea and targeting was
        # armed, this stores the card_id waiting for a destination click.
        self._pending_move_card_id: str = ""

        # Attack targeting state: when True the next actor selection is the
        # target of an interactive attack declared by the active hero.
        self._awaiting_attack: bool = False

        # Pending reaction window: id of the defender that must react (engine
        # owns the truth; the GUI only collects the player's choice).
        self._pending_defender: str = ""

        # Connect status signal before _build_bridge launches the probe.
        self._engine_status.connect(self._on_engine_status)


        self._build_menu()
        self._build_layout()
        self._build_bridge()
        self._build_router()
        self._setup_status_bar()

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
                  + AreaInfoWidget      (info locazione selezionata)
          Bottom:   ActionPanelWidget   (Azioni Base \u23f3)
        """
        self.setDockOptions(
            QMainWindow.DockOption.AllowTabbedDocks
            | QMainWindow.DockOption.AnimatedDocks
            | QMainWindow.DockOption.AllowNestedDocks
            | QMainWindow.DockOption.GroupedDragging
        )
        self.setDockNestingEnabled(True)

        # ── Central: Map ──────────────────────────────────────────────────────
        self._board = EldhomBoardWidget(self)
        self.setCentralWidget(self._board)

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
        self._area_info = AreaInfoWidget()

        # Bottom container: only the simple-action buttons (cards are played
        # from the Deck panel, not from a separate hand widget).
        bottom_widget = QWidget()
        bottom_vbox = QVBoxLayout(bottom_widget)
        bottom_vbox.setContentsMargins(0, 0, 0, 0)
        bottom_vbox.setSpacing(0)
        bottom_vbox.addWidget(self._actions)

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

        # Inner right (below Attori): Area / location info
        area_info_dock = QDockWidget("Info Area", self._actor_inner)
        area_info_dock.setObjectName("inner_dock_area_info")
        area_info_dock.setWidget(self._area_info)
        self._actor_inner.addDockWidget(
            Qt.DockWidgetArea.RightDockWidgetArea, area_info_dock
        )
        self._actor_inner.splitDockWidget(
            actors_inner_dock, area_info_dock, Qt.Orientation.Vertical
        )

        # Inner bottom: Azioni Base
        actions_inner_dock = QDockWidget("Azioni Base", self._actor_inner)
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
        self._area_info.actor_selected.connect(self._on_actor_selected)
        self._actions.move_armed.connect(self._on_move_armed)
        self._actions.attack_armed.connect(self._on_attack_armed)
        self._actions.action_interact.connect(self._on_simple_interact)
        self._actions.action_recover.connect(self._on_simple_recover)
        self._actions.stop_sequence.connect(self._on_stop_sequence)
        self._actions.react_chosen.connect(self._on_react_chosen)
        self._board.area_selected.connect(self._on_area_selected)


    def _build_bridge(self) -> None:
        # Use the bridge provided by main.py (receiver already running) or
        # create a new one (fallback for tests and direct execution).
        if self._bridge is None:
            self._bridge = EldhomBridge()
            self._bridge.receiver.start()
            print("[EldhomGUI] Event receiver avviato su porta 9210", flush=True)
        else:
            print("[EldhomGUI] Usando receiver pre-avviato su porta 9210", flush=True)
        # Wire deck module outgoing commands through the proxy translator
        self._deck.set_sender(_DeckProxy(
            self._bridge,
            lambda: self._active_hero_id,
            lambda hid, cid: self._pre_play_card_hook(hid, cid),
        ))
        self._bridge.set_on_event(self._on_engine_event_thread)
        # Try to connect to the engine eagerly (non-blocking probe).
        # The engine must already be running when the GUI starts.
        threading.Thread(target=self._probe_engine, daemon=True).start()

    def _probe_engine(self) -> None:
        """Background thread: tries to connect to the C++ engine on port 9211.

        Emits _engine_status Signal (thread-safe) to update the status bar
        on the main Qt thread.  Timeout is set in EngineSender (~3 s).
        """
        connected = self._bridge.sender.connect()
        if connected:
            msg = "Engine connesso \u2014 usa Gioca \u203a Inizia Nuova Missione"
            print("[EldhomGUI] Engine connesso su porta 9211", flush=True)
        else:
            msg = "\u26a0 Engine non raggiungibile \u2014 avvia eldhom_engine.exe"
            print("[EldhomGUI] Engine NON raggiungibile su porta 9211", flush=True)
        # Thread-safe: Signal.emit() from non-Qt thread uses AutoConnection
        # → queued delivery on the main thread.
        self._engine_status.emit(msg)

    def _build_router(self) -> None:
        self._router = EventRouter()

        def reg(type_id: str, handler):
            self._router.register(type_id, handler)

        # Full state
        reg("eldhom.state.full",              self._on_state_full)
        reg("eldhom.state.full",              self._board.on_state_full)
        reg("eldhom.state.full",              self._timeline.on_state_full)
        reg("eldhom.state.full",              self._on_deck_state_full)

        # Turn management
        reg("eldhom.turn.next_actor",         self._on_next_actor)
        reg("eldhom.turn.next_actor",         self._timeline.on_next_actor)
        reg("eldhom.pg.turn_ended",           self._on_pg_turn_ended)

        # Interactive attack / reaction window
        reg("eldhom.attack.declared",         self._on_attack_declared)
        reg("eldhom.reaction.window_opened",  self._on_reaction_window_opened)
        reg("eldhom.reaction.window_closed",  self._on_reaction_window_closed)
        reg("eldhom.attack.resolved",         self._on_attack_resolved)

        # Instant-card reaction window (priority over defense)
        reg("eldhom.instant.window_opened",   self._on_instant_window_opened)
        reg("eldhom.instant.window_closed",   self._on_instant_window_closed)

        # Interactive formation dialog (Scompaginamento / Schieramento)
        reg("eldhom.formation.dialog_needed", self._on_formation_dialog_needed)

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
        reg("eldhom.pg.moved",                self._board.on_pg_moved)
        reg("eldhom.monster.defeated",        self._board.on_monster_defeated)

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
            "eldhom.attack.declared", "eldhom.attack.resolved",
        ):
            reg(evt, self._log_widget.on_any_event)
        reg("eldhom.action.result",           self._log_widget.on_action_result)
        reg("eldhom.mission.victory",         self._log_widget.on_mission_victory)
        reg("eldhom.mission.defeat",          self._log_widget.on_mission_defeat)

    def _setup_status_bar(self) -> None:
        sb = QStatusBar(self)
        self._status_label = QLabel("In attesa del CoreEngine...", self)
        self._status_label.setProperty("text_role", "secondary")
        sb.addWidget(self._status_label)
        self.setStatusBar(sb)

    def _on_engine_status(self, msg: str) -> None:
        """Slot: receives status updates from the probe thread (main thread)."""
        if hasattr(self, "_status_label"):
            self._status_label.setText(msg)

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
            hid = str(hero["id"])
            self._hero_data[hid] = hero
            self._hand_cards[hid] = hero.get("hand", [])
        self._group_data.clear()
        for grp in data.get("groups", []):
            self._group_data[str(grp.get("id", ""))] = grp
        self._location_adjacency.clear()
        self._location_names.clear()
        for loc in data.get("locations", []):
            lid = str(loc["id"])
            self._location_adjacency[lid] = [
                str(a) for a in loc.get("adjacent", [])
            ]
            self._location_names[lid] = str(loc.get("name", lid))
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
            self._actions.set_turn(hero.get("name", actor_id))
        else:
            self._actions.set_enabled(False)

    def _on_pg_turn_ended(self, msg: dict) -> None:
        data = _extract_data(msg)
        if data.get("actor_id", "") == self._active_hero_id:
            self._actions.set_enabled(False)


    # ── Deck event translation (eldhom.* → GmCompDeckModule) ─────────────────

    def _on_deck_state_full(self, msg: dict) -> None:
        """Translates hero hand data from full state for GmCompDeckModule."""
        data = _extract_data(msg)
        for hero in data.get("heroes", []):
            hand_ids = [str(c) for c in hero.get("hand", [])]
            cards = [_card_meta(c) for c in hand_ids]
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
        cards = [_card_meta(c) for c in hand_ids]
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
        card_id = str(data.get("payload", data.get("card_id", "")))
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
        """Called when the user selects an actor in the tree or area panel.

        While attack targeting is armed, selecting an enemy declares an
        interactive attack against it; otherwise it just highlights the actor.
        """
        if not actor_id:
            return
        if self._awaiting_attack and self._active_hero_id:
            self._try_declare_attack(actor_id)
            return
        self._actors.select_actor(actor_id)

    def _on_victory(self, msg: dict) -> None:
        self._actions.set_enabled(False)
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
        QTimer.singleShot(
            200,
            lambda: QMessageBox.warning(
                self, "Sconfitta", f"Missione fallita!\n{payload}"
            ),
        )

    # ── Player action senders ──────────────────────────────────────────────────

    def _on_move_armed(self, armed: bool) -> None:
        """Enters/exits move targeting mode triggered by the Muovi button."""
        if not armed:
            self._pending_move_card_id = ""  # Clear any card-driven pending move
        self._awaiting_move = armed and bool(self._active_hero_id)
        if self._awaiting_move:
            msg = "\u25b6 Clicca la locazione di destinazione sulla mappa"
            self._status_label.setText(msg)
            self._actions.set_hint(msg)
        else:
            self._status_label.setText("")
            self._actions.set_hint("")

    def _on_area_selected(self, location_id: str) -> None:
        """Handles a map location click.

        While move targeting is armed the click is interpreted as a move
        destination (with an adjacency check); otherwise it just shows the
        location's info panel.
        """
        if not location_id:
            return
        if self._awaiting_move and self._active_hero_id:
            self._try_move(location_id)
            return
        self._show_area_info(location_id)

    def _try_move(self, destination: str) -> None:
        """Validates adjacency and sends a MOVE action (simple action or card)."""
        hero   = self._hero_data.get(self._active_hero_id, {})
        origin = str(hero.get("location", ""))
        adjacent = self._location_adjacency.get(origin, [])
        if destination == origin:
            self._status_label.setText("\u26a0 Sei gi\u00e0 in questa locazione")
            return
        if destination not in adjacent:
            name = self._location_names.get(destination, destination)
            self._status_label.setText(
                f"\u26a0 {name} non \u00e8 adiacente \u2014 scegli una locazione vicina"
            )
            return
        self._awaiting_move = False
        self._actions.disarm_move()
        if self._pending_move_card_id:
            # Card-driven move: send play_card with destination.
            card_id = self._pending_move_card_id
            self._pending_move_card_id = ""
            self._actions.set_hint("")
            self._bridge.send_command(
                "eldhom.play_card",
                {
                    "hero_id":     self._active_hero_id,
                    "card_id":     card_id,
                    "destination": destination,
                },
            )
        else:
            # Simple action move.
            self._actions.set_hint("")
            self._bridge.send_command(
                "eldhom.simple_action",
                {
                    "hero_id":     self._active_hero_id,
                    "action_type": "MOVE",
                    "destination": destination,
                },
            )

    def _show_area_info(self, location_id: str) -> None:
        """Populates the Info Area panel from the cached full-state snapshot."""
        name = self._location_names.get(location_id, location_id)
        adj_names = [
            self._location_names.get(a, a)
            for a in self._location_adjacency.get(location_id, [])
        ]
        actors: list[dict] = []
        for hero in self._hero_data.values():
            if str(hero.get("location", "")) == location_id:
                actors.append({
                    "id":       str(hero.get("id", "")),
                    "name":     str(hero.get("name", hero.get("id", ""))),
                    "kind":     "HERO",
                    "hp":       hero.get("hp"),
                    "max_hp":   hero.get("max_hp"),
                    "position": str(hero.get("position", "")),
                })
        for grp in self._group_data.values():
            for inst in grp.get("instances", []):
                if str(inst.get("location", "")) == location_id:
                    actors.append({
                        "id":       str(inst.get("id", "")),
                        "name":     str(inst.get("id", "")),
                        "kind":     "MONSTER",
                        "hp":       inst.get("hp"),
                        "max_hp":   inst.get("max_hp"),
                        "position": str(inst.get("position", "")),
                    })
        self._area_info.show_area(location_id, name, adj_names, actors)

    # ── Interactive attack / reaction window ────────────────────────────────────

    def _on_attack_armed(self, armed: bool) -> None:
        """Enters/exits attack targeting mode triggered by the Attacca button."""
        self._awaiting_attack = armed and bool(self._active_hero_id)
        if self._awaiting_attack:
            msg = "\u2694 Clicca il nemico da attaccare (mappa o pannello attori)"
            self._status_label.setText(msg)
            self._actions.set_hint(msg)
        else:
            self._status_label.setText("")
            self._actions.set_hint("")

    def _actor_is_enemy(self, actor_id: str) -> bool:
        """True if actor_id is a live monster instance (not a hero)."""
        if actor_id in self._hero_data:
            return False
        for grp in self._group_data.values():
            for inst in grp.get("instances", []):
                if str(inst.get("id", "")) == actor_id:
                    return True
        return False

    def _try_declare_attack(self, target_id: str) -> None:
        """Validates the target then asks the engine to declare the attack."""
        if not self._actor_is_enemy(target_id):
            self._status_label.setText(
                "\u26a0 Seleziona un nemico valido da attaccare"
            )
            self._actors.select_actor(target_id)
            return
        self._awaiting_attack = False
        self._actions.disarm_attack()
        self._actions.set_hint("")
        self._bridge.send_command(
            "eldhom.declare_attack",
            {"hero_id": self._active_hero_id, "target_id": target_id},
        )

    def _defender_name(self, defender_id: str) -> str:
        """Resolves a human-friendly label for a defender id."""
        for grp in self._group_data.values():
            for inst in grp.get("instances", []):
                if str(inst.get("id", "")) == defender_id:
                    return str(inst.get("id", defender_id))
        return defender_id

    def _on_attack_declared(self, msg: dict) -> None:
        """Status feedback when the engine accepts an attack declaration."""
        data    = _extract_data(msg)
        target  = self._defender_name(str(data.get("payload", "")))
        self._status_label.setText(f"\u2694 Attacco dichiarato contro {target}")

    def _on_reaction_window_opened(self, msg: dict) -> None:
        """Opens the defense panel so the player chooses the monster reaction.

        Fields are sent at the data root (not under payload) by the engine.
        """
        data       = _extract_data(msg)
        defender   = str(data.get("defender_id", ""))
        damage     = int(data.get("incoming_damage", 0))
        reactions  = data.get("reactions", []) or []
        self._pending_defender = defender
        name = self._defender_name(defender)
        self._actions.enter_defense_mode(name, damage, reactions)
        self._status_label.setText(
            f"\U0001f6e1 {name} sotto attacco \u2014 scegli la reazione"
        )

    def _on_react_chosen(self, code: str) -> None:
        """Sends the player's reaction choice to the engine."""
        if not self._pending_defender:
            return
        self._bridge.send_command(
            "eldhom.react_defense",
            {"defender_id": self._pending_defender, "reaction": code},
        )

    def _actor_display_name(self, actor_id: str) -> str:
        """Resolves a friendly label for any hero or monster instance id."""
        hero = self._hero_data.get(actor_id)
        if hero:
            return str(hero.get("name", hero.get("id", actor_id)))
        return self._defender_name(actor_id)

    def _on_instant_window_opened(self, msg: dict) -> None:
        """Shows the instant-card dialog; forwards the choice to the engine.

        Options are sent at the data root by the engine. The single client
        decides which instants (if any) to play before the defense window.
        """
        data    = _extract_data(msg)
        options = data.get("options", []) or []
        names   = {
            aid: self._actor_display_name(aid)
            for aid in {str(o.get("actor_id", "")) for o in options}
        }
        dialog   = InstantWindowDialog(options, names, self)
        accepted = dialog.exec()
        selected = dialog.selected_options() if accepted else []
        payload  = [
            {"actor_id": str(o.get("actor_id", "")),
             "card_id":  str(o.get("card_id", ""))}
            for o in selected
        ]
        self._bridge.send_command("eldhom.play_instants", {"selected": payload})

    def _on_instant_window_closed(self, msg: dict) -> None:
        """Status feedback once the engine resolved the instant window."""
        data  = _extract_data(msg)
        count = int(data.get("count", 0))
        if count > 0:
            self._status_label.setText(
                f"\u26a1 {count} carta/e istantanea/e giocata/e"
            )

    def _pre_play_card_hook(self, hero_id: str, card_id: str) -> bool:
        """Called by _DeckProxy before sending play_card for a card dragged to PlayArea.

        If the card has a MOVE effect requiring player destination choice, arms
        move targeting mode and returns True (play_card is NOT sent yet).
        Returns False to let the proxy send play_card immediately.
        """
        meta = _CARD_CATALOG.get(card_id, {})
        has_choice_move = any(
            e.get("effect_type") == "MOVE" and not e.get("value", "")
            for e in meta.get("effects", [])
        )
        if not has_choice_move:
            return False

        self._pending_move_card_id = card_id
        self._awaiting_move = True
        msg = "\u25b6 Clicca la locazione di destinazione (carta)"
        self._status_label.setText(msg)
        self._actions.set_hint(msg)
        # Send the card back to CardHand visually (the drag already moved it).
        from PySide6.QtCore import QTimer
        QTimer.singleShot(0, lambda: self._deck.on_envelope({
            "typeId": "gmAlea.deck.card_moved",
            "data": {
                "card_id":   card_id,
                "from_zone": "PlayArea",
                "to_zone":   "CardHand",
            },
        }))
        return True

    def _on_formation_dialog_needed(self, msg: dict) -> None:
        """Opens the formation assignment dialog and forwards the choice.

        Fields are sent at the data root by the engine (location_id, faction_id,
        source, actors[]).  The dialog is mandatory — the player cannot skip it.
        """
        data        = _extract_data(msg)
        location_id = str(data.get("location_id", ""))
        faction_id  = str(data.get("faction_id",  ""))
        source      = str(data.get("source",      ""))
        actors      = data.get("actors", []) or []

        dialog = FormationDialog(location_id, faction_id, source, actors, self)
        dialog.exec()  # Always accepted (no cancel button)
        backline = dialog.backline_actor_ids()
        self._bridge.send_command(
            "eldhom.resolve_formation",
            {
                "faction_id":  faction_id,
                "location_id": location_id,
                "backline":    backline,
            },
        )

    def _on_reaction_window_closed(self, msg: dict) -> None:
        """Closes the defense panel once the engine resolved the reaction."""
        self._actions.exit_defense_mode()
        self._pending_defender = ""

    def _on_attack_resolved(self, msg: dict) -> None:
        """Status feedback describing the resolved attack outcome."""
        data     = _extract_data(msg)
        defender = self._defender_name(str(data.get("defender_id", "")))
        dmg      = int(data.get("final_damage", 0))
        if data.get("defender_ko", False):
            self._status_label.setText(f"\u2620 {defender} sconfitto!")
        elif dmg > 0:
            self._status_label.setText(f"\u2694 {defender} subisce {dmg} danni")
        else:
            self._status_label.setText(f"\u26e8 {defender} non subisce danni")

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
            # Engine accepts only "eldhom.start_mission" (CMD_START_MISSION).
            self._bridge.send_command(
                "eldhom.start_mission",
                {"mission_id": dialog.selected_mission_id},
            )

    # ── Helpers ────────────────────────────────────────────────────────────────

    def closeEvent(self, event) -> None:
        """Stops the receiver thread before closing to free port 9210."""
        self._bridge.receiver.stop()
        super().closeEvent(event)

    def _center_on_screen(self) -> None:
        screen = QApplication.primaryScreen()
        if screen is not None:
            geo = screen.availableGeometry()
            self.move(
                geo.center().x() - self.width() // 2,
                geo.center().y() - self.height() // 2,
            )

