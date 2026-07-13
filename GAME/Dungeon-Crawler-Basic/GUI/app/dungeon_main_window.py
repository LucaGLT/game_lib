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

import json
from pathlib import Path

from PySide6.QtWidgets import (
    QDockWidget,
    QMainWindow,
    QStatusBar,
    QToolBar,
    QWidget,
)
from PySide6.QtCore import Qt, QSettings, QByteArray

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
from gmGui.modules.gm_comp_deck_module import GmCompDeckModule

# Zone names that match GmCompDeckModule exactly.
_DECK_ZONES: list[str] = ["MainDeck", "CardHand", "PlayArea", "Memory", "DiscardPile", "BanishZone"]

# Maps card_id → the gameplay action triggered when the card is played to PlayArea.
# action_type values: "MOVE" | "ATTACK" | "HEAL" | "BUFF" (only MOVE is handled here)
_CARD_ACTIONS: dict[str, dict] = {
    "passo_veloce":    {"action_type": "MOVE",   "max_distance": 2, "action_cost": 1},
    "colpo_efficace":  {"action_type": "ATTACK", "card_damage": 2, "max_distance": 1, "action_cost": 1},
    "pugno_di_ferro":  {"action_type": "ATTACK", "card_damage": 4, "max_distance": 1, "action_cost": 2},
    "tiro_rapido":     {"action_type": "ATTACK", "card_damage": 2, "max_distance": 1, "action_cost": 1},
    "veleno":          {"action_type": "ATTACK", "card_damage": 1, "max_distance": 1, "action_cost": 1},
    "furia_cieca":     {"action_type": "BUFF",   "action_cost": 1},
    "grido_di_guerra": {"action_type": "BUFF",   "action_cost": 1},
    "parata":          {"action_type": "BUFF",   "action_cost": 1},
    "pozione_di_cura": {"action_type": "HEAL",   "action_cost": 1},
}


class _DeckProxy:
    """Sender proxy that intercepts gmAlea.deck.* commands for local per-hero deck tracking.

    All other commands are forwarded to the real EngineSender unchanged.
    """

    def __init__(self, real_sender, local_callback) -> None:
        self._real_sender = real_sender
        self._callback = local_callback

    def send_command(self, type_id: str, data: dict) -> None:
        """Routes gmAlea.deck commands locally; forwards everything else."""
        if type_id.startswith("gmAlea.deck."):
            self._callback(type_id, data)
        elif self._real_sender is not None:
            self._real_sender.send_command(type_id, data)


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
        """Creates and arranges all child widgets.

        Main Window layout:
          Top    : Flow / Timeline        (QDockWidget)
          Left   : Log | Area Info        (two QDockWidgets, tabbed)
          Centre : MAP                    (DungeonBoardWidget — centralWidget)
          Right  : Actor Panel            (QDockWidget wrapping an inner QMainWindow)
          Bottom : Messaggi               (QDockWidget — Error / Warning / Info)

        Actor Panel inner layout (QMainWindow with Qt.WindowType.Widget):
          Centre : Deck Manager           (GmCompDeckModule — centralWidget)
          Right  : Actors                 (QDockWidget)
          Bottom : Actions                (QDockWidget)
          Left   : (empty, dockable)
          Top    : (empty, dockable)

        The entire Actor Panel dock is floatable and can be moved to any area
        of the main window.  Panels inside the inner window are dockable only
        within that inner window (Qt limitation: cross-QMainWindow drag is not
        supported).
        """
        self._board     = DungeonBoardWidget()
        self._heroes    = HeroPanelWidget()
        self._actions   = ActionPanelWidget()
        self._log       = LogWidget()
        self._errors    = ErrorBarWidget()
        self._area_info = GmMapAreaInfoModule()
        self._flow      = GmFlowModule()
        self._deck      = GmCompDeckModule()
        # The CoreEngine is the sole authority on action cost; disable the
        # deck module's client-side "Azioni esaurite" warning.
        self._deck.set_enforce_action_cost(False)

        # ── Main window dock options ──────────────────────────────────────────
        self.setDockOptions(
            QMainWindow.DockOption.AllowTabbedDocks
            | QMainWindow.DockOption.AnimatedDocks
            | QMainWindow.DockOption.GroupedDragging
            | QMainWindow.DockOption.AllowNestedDocks
        )
        self.setDockNestingEnabled(True)

        # ── Central: MAP ──────────────────────────────────────────────────────
        self.setCentralWidget(self._board)

        # ── Top: Flow / Timeline ─────────────────────────────────────────────
        self._flow_dock = QDockWidget(self._flow.title, self)
        self._flow_dock.setObjectName("dock_flow")
        self._flow_dock.setWidget(self._flow.widget())
        self.addDockWidget(Qt.DockWidgetArea.TopDockWidgetArea, self._flow_dock)
        self._flow.on_attach()

        # ── Left: Actor Panel — inner QMainWindow ─────────────────────────────
        # Placed on the left to match the default layout (see screenshot).
        # A QMainWindow embedded as Qt.WindowType.Widget becomes an ordinary
        # widget with its own full dock-area system.  The outer QDockWidget
        # (self._actor_dock) lets the whole panel float or be re-docked in any
        # area of the main window.
        self._actor_inner = QMainWindow()
        self._actor_inner.setWindowFlags(Qt.WindowType.Widget)
        self._actor_inner.setDockNestingEnabled(True)
        self._actor_inner.setDockOptions(
            QMainWindow.DockOption.AllowTabbedDocks
            | QMainWindow.DockOption.AnimatedDocks
            | QMainWindow.DockOption.GroupedDragging
            | QMainWindow.DockOption.AllowNestedDocks
        )

        # Inner central: Deck Manager
        self._actor_inner.setCentralWidget(self._deck.widget())
        self._deck.on_attach()

        # Inner right: Actors
        self._actors_inner_dock = QDockWidget("Actors", self._actor_inner)
        self._actors_inner_dock.setObjectName("inner_dock_actors")
        self._actors_inner_dock.setWidget(self._heroes)
        self._actor_inner.addDockWidget(
            Qt.DockWidgetArea.RightDockWidgetArea, self._actors_inner_dock
        )

        # Inner bottom: Actions
        self._actions_inner_dock = QDockWidget("Actions", self._actor_inner)
        self._actions_inner_dock.setObjectName("inner_dock_actions")
        self._actions_inner_dock.setWidget(self._actions)
        self._actor_inner.addDockWidget(
            Qt.DockWidgetArea.BottomDockWidgetArea, self._actions_inner_dock
        )

        # Initial inner proportions
        self._actor_inner.resizeDocks(
            [self._actors_inner_dock], [260], Qt.Orientation.Horizontal
        )
        self._actor_inner.resizeDocks(
            [self._actions_inner_dock], [120], Qt.Orientation.Vertical
        )

        # Wrap inner window in a dock widget and place it in main window Left
        self._actor_dock = QDockWidget("Actor Panel", self)
        self._actor_dock.setObjectName("dock_actor_panel")
        self._actor_dock.setWidget(self._actor_inner)
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, self._actor_dock)

        # ── Right: Area Info + Log (tabbed) ───────────────────────────────────
        self._area_dock = QDockWidget("Area Info", self)
        self._area_dock.setObjectName("dock_area_info")
        self._area_dock.setWidget(self._area_info.widget())
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self._area_dock)

        self._log_dock = QDockWidget("Log", self)
        self._log_dock.setObjectName("dock_log")
        self._log_dock.setWidget(self._log)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self._log_dock)
        self.tabifyDockWidget(self._area_dock, self._log_dock)
        self._area_dock.raise_()

        # ── Bottom: Messaggi (Error / Warning / Info) ─────────────────────────
        self._messages_dock = QDockWidget("Messaggi", self)
        self._messages_dock.setObjectName("dock_messages")
        self._messages_dock.setWidget(self._errors)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, self._messages_dock)

        # ── Initial main window proportions ───────────────────────────────────
        self.resizeDocks(
            [self._actor_dock, self._area_dock],
            [660, 200],
            Qt.Orientation.Horizontal,
        )
        self.resizeDocks(
            [self._flow_dock, self._messages_dock],
            [130, 55],
            Qt.Orientation.Vertical,
        )

        # ── Restore persisted layout (QSettings) ──────────────────────────────
        # If the user has rearranged docks in a previous session, restore that
        # arrangement.  objectName of every dock must stay stable for this to
        # work correctly.
        settings = QSettings("GameLib", "DungeonCrawlerBasic")
        main_state: QByteArray = settings.value("layout/main")  # type: ignore[assignment]
        inner_state: QByteArray = settings.value("layout/actor_inner")  # type: ignore[assignment]
        if main_state:
            self.restoreState(main_state)
        if inner_state:
            self._actor_inner.restoreState(inner_state)

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
        # Per-hero deck state: hero_id → zone_name → list[card dict].
        self._card_catalog: list[dict] = self._load_card_catalog()
        self._hero_decks: dict[str, dict[str, list[dict]]] = {}
        self._current_deck_hero: str = ""
        # Deck manager: intercept gmAlea.deck.* locally; forward the rest.
        self._deck_proxy = _DeckProxy(self._bridge.sender, self._on_deck_command)
        self._deck.set_sender(self._deck_proxy)
        self._last_round: int = 0
        self._pending_move_hero: str = ""  # hero_id waiting for a destination click
        self._pending_card_move: dict = {}  # {"hero_id", "card_id", "max_distance"} when card-move pending
        self._pending_card_attack: dict = {}  # {"hero_id", "card_id", "card_damage", "max_distance"}
        self._current_turn_actor: str = ""  # actor_id whose turn is active
        self._current_actions_remaining: int = 0  # mirrors ActionPanel counter; updated on turn start
        self._pending_attack_attacker: str = ""  # attacker waiting for a target click
        self._defense_active: bool = False  # True while a defense window is open
        for ev in ("dungeon.session.started", "dungeon.turn.started",
                   "dungeon.turn.ended", "dungeon.game.over"):
            self._router.register(ev, self._on_flow_event)
        # Auto-select the active actor in the hero panel on every turn change.
        self._router.register("dungeon.turn.started", self._on_turn_started_select)
        # Board: map layout and actor movement
        for ev in ("dungeon.map.snapshot", "dungeon.actor.snapshot",
                   "dungeon.actor.moved", "dungeon.actor.removed", "dungeon.game.over"):
            self._router.register(ev, self._board.on_envelope)
        # Clear card-move targeting state on successful move.
        self._router.register("dungeon.actor.moved", self._on_actor_moved)
        # Hero panel: actor state
        for ev in ("dungeon.actor.snapshot", "dungeon.actor.hp_changed",
                   "dungeon.actor.status_changed", "dungeon.actor.removed",
                   "dungeon.session.started"):
            self._router.register(ev, self._heroes.on_envelope)
        # Deck initialisation: detect heroes and build per-hero deck state.
        self._router.register("dungeon.actor.snapshot", self._on_actor_snapshot_for_decks)
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
        # Reactive defense flow (Phase 5): drive the defense window UX.
        self._router.register("dungeon.defense.window.opened", self._on_defense_window_opened)
        self._router.register("dungeon.defense.window.closed", self._on_defense_window_closed)
        self._router.register("dungeon.attack.resolved", self._on_attack_resolved)
        # Test automation: allow remote clients to start a new game via TCP
        self._router.register("dungeon.new_game", self._on_new_game)
        # Combat events also feed the log for player feedback.
        for ev in ("dungeon.attack.declared", "dungeon.defense.window.opened",
                   "dungeon.attack.resolved"):
            self._router.register(ev, self._log.on_envelope)
        # Area Info: contents of the selected map area (shared contract)
        self._router.register(AREA_INFO_RESPONSE, self._area_info.on_envelope)
        # Actor removed: also forward to board and log
        self._router.register("dungeon.actor.removed", self._on_actor_removed)
        # Deck manager: gmAlea card events + gmActor resource tracking
        for ev in ("gmAlea.deck.zone_changed", "gmAlea.deck.card_moved",
                   "gmAlea.deck.shuffled", "gmAlea.deck.drawn",
                   "gmActor.snapshot", "gmActor.actor.resource_changed"):
            self._router.register(ev, self._deck.on_envelope)

        # Wire signals from widgets back to engine
        self._board.area_selected.connect(self._on_area_selected)
        self._actions.move_requested.connect(self._on_move_action)
        self._actions.heal_requested.connect(self._on_heal_requested)
        self._actions.equip_requested.connect(self._on_equip_requested)
        self._actions.end_turn_requested.connect(self._on_end_turn_requested)
        self._actions.attack_requested.connect(self._on_attack_action)
        self._actions.defend_requested.connect(self._on_defend_requested)
        self._actions.defend_pass_requested.connect(self._on_defend_pass_requested)
        self._actions.actions_remaining_changed.connect(self._on_actions_remaining_changed)
        # Area Info: actor click → same routing as hero-panel actor click.
        self._area_info.on_actor_selected = self._on_area_info_actor_selected
        # Actor selection in hero panel → sync action panel display
        # (or pick the attack target while in attack-targeting mode).
        self._heroes.actor_selected.connect(self._on_actor_selected)

    # ── Envelope handler (called by bridge on every incoming event) ───────────

    def _on_envelope(self, msg: dict) -> None:
        """Receives a decoded event envelope and dispatches it to the router."""
        self._router.dispatch(msg)

    def _on_turn_started_select(self, msg: dict) -> None:
        """Auto-selects the active actor; cancels any pending move targeting."""
        if self._defense_active:
            # Do not steal selection while a defense window is open.
            return
        self._pending_move_hero = ""
        self._pending_card_move = {}
        self._pending_card_attack = {}
        self._pending_attack_attacker = ""
        self._actions.set_awaiting_move(False)
        self._actions.set_awaiting_attack(False)
        data: dict = msg.get("data", {})
        actor_id: str = str(data.get("actor_id", ""))
        self._current_actions_remaining = int(data.get("actions_remaining", 2))
        self._current_turn_actor = actor_id
        if actor_id:
            self._board.set_active_hero(actor_id)
            self._heroes.select_actor(actor_id)
            self._heroes.update_actions_remaining(actor_id, self._current_actions_remaining)
            # Switch deck display to this hero (no-op for monsters).
            self._inject_deck_zones(actor_id)

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
        if self._pending_card_move:
            # Card-enhanced move: Core validates distance via BFS.
            # Do NOT clear _pending_card_move here: if the engine rejects the
            # move (e.g. destination out of range) the targeting mode must stay
            # active so the player can pick a different room.
            # State is cleared by _on_actor_moved (success) or by
            # _on_turn_started_select (turn change).
            hero_id = self._pending_card_move.get("hero_id", "")
            card_id = self._pending_card_move.get("card_id", "")
            max_distance = int(self._pending_card_move.get("max_distance", 1))
            if area_id and hero_id:
                self._bridge.send_command("dungeon.move", {
                    "hero_id": hero_id,
                    "destination": area_id,
                    "max_distance": max_distance,
                    "card_id": card_id,
                })
        elif self._pending_move_hero:
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
        self._pending_card_move = {}
        self._pending_card_attack = {}
        self._pending_attack_attacker = ""
        self._defense_active = False
        self._hero_decks = {}
        self._current_deck_hero = ""
        # Clear all deck zones so the module shows empty until session.started.
        for zone in _DECK_ZONES:
            self._deck.on_envelope({
                "typeId": "gmAlea.deck.zone_changed",
                "data": {"zone_name": zone, "cards": []},
            })
        self._bridge.send_command("dungeon.new_game", {})

    def _on_actions_remaining_changed(self, remaining: int) -> None:
        """Syncs the heroes-panel 'azioni' resource whenever the action counter changes."""
        self._current_actions_remaining = remaining
        if self._current_turn_actor:
            self._heroes.update_actions_remaining(self._current_turn_actor, remaining)

    def _on_area_info_actor_selected(self, actor_id: str) -> None:
        """Routes an actor click from the Area Info panel.

        During a defense window the selection is locked (defense must be resolved
        first).  Otherwise the click is treated identically to a hero-panel click:
        attack targeting is resolved first, then normal selection.
        """
        if self._defense_active:
            self._errors.show_error(
                "Difesa in corso: completa la difesa prima di cambiare selezione."
            )
            return
        self._on_actor_selected(actor_id)

    def _on_actor_moved(self, msg: dict) -> None:
        """Shows a move confirmation message and updates action count from CoreEngine."""
        data: dict = msg.get("data", {})
        actor_id: str = str(data.get("actor_id", ""))
        to_room: str = str(data.get("to", ""))
        if actor_id and to_room:
            self._errors.show_info(f"✓ {actor_id} si è spostato in {to_room}")
        self._pending_card_move = {}
        self._pending_move_hero = ""
        self._actions.set_awaiting_move(False)
        # Update remaining actions from CoreEngine state.
        actions_remaining = int(data.get("actions_remaining", 0))
        if actions_remaining >= 0 and actions_remaining != self._current_actions_remaining:
            self._current_actions_remaining = actions_remaining
            self._heroes.update_actions_remaining(self._current_turn_actor, actions_remaining)

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

    # ── Combat: attack targeting (Phase 5) ────────────────────────────────────

    def _on_actor_selected(self, actor_id: str) -> None:
        """Routes a hero-panel actor selection.

        Priority:
          1. Card-triggered attack  (_pending_card_attack set)
          2. Button-triggered attack (_pending_attack_attacker set)
          3. Normal selection        (drives the action panel display)
        """
        # ── Card-triggered attack targeting ───────────────────────────────────
        if self._pending_card_attack:
            attacker_id = self._pending_card_attack.get("hero_id", "")
            card_id     = self._pending_card_attack.get("card_id", "")
            card_damage = int(self._pending_card_attack.get("card_damage", 0))
            self._pending_card_attack = {}
            self._actions.set_awaiting_attack(False)
            if actor_id and actor_id != attacker_id:
                self._bridge.send_command(
                    "dungeon.attack",
                    {"attacker_id": attacker_id, "target_id": actor_id,
                     "card_id": card_id, "card_damage": card_damage})
            else:
                self._errors.show_error("Seleziona un bersaglio nemico diverso da te.")
            return
        # ── Button-triggered attack targeting ─────────────────────────────────
        if self._pending_attack_attacker:
            attacker_id = self._pending_attack_attacker
            target_id = actor_id
            self._pending_attack_attacker = ""
            self._actions.set_awaiting_attack(False)
            if target_id and target_id != attacker_id:
                self._bridge.send_command(
                    "dungeon.attack",
                    {"attacker_id": attacker_id, "target_id": target_id,
                     "card_id": "", "card_damage": 0})
                self._actions.mark_action_consumed()
            else:
                self._errors.on_envelope({
                    "typeId": "dungeon.action.rejected",
                    "data": {"reason": "Seleziona un bersaglio nemico valido.",
                             "command": "dungeon.attack"},
                })
            return
        # ── Normal selection: blocked during active defense window ─────────────
        if self._defense_active:
            return
        self._actions.set_selected_actor(actor_id)

    def _on_attack_action(self, attacker_id: str) -> None:
        """Toggles attack-targeting mode (enter on first press, cancel on second)."""
        if self._pending_attack_attacker:
            self._pending_attack_attacker = ""
            self._actions.set_awaiting_attack(False)
        else:
            # Entering attack mode cancels any pending move targeting.
            self._pending_move_hero = ""
            self._actions.set_awaiting_move(False)
            self._pending_attack_attacker = attacker_id
            self._actions.set_awaiting_attack(True)
            self._errors.on_envelope({
                "typeId": "dungeon.action.rejected",
                "data": {"reason": "Seleziona il bersaglio nel pannello attori.",
                         "command": "dungeon.attack"},
            })

    # ── Combat: reactive defense (Phase 5) ────────────────────────────────────

    def _on_defense_window_opened(self, msg: dict) -> None:
        """Switches the GUI into reactive-defense mode for the defender."""
        data: dict = msg.get("data", {}) or {}
        defender_id: str = str(data.get("defender_id", ""))
        if not defender_id:
            return
        self._defense_active = True
        # Cancel any pending targeting so the player must react first.
        self._pending_move_hero = ""
        self._pending_attack_attacker = ""
        self._actions.set_awaiting_move(False)
        self._actions.set_awaiting_attack(False)
        # The defender becomes the selected actor (reuse existing panels).
        self._heroes.select_actor(defender_id)
        self._actions.enter_defense_mode(
            defender_id,
            int(data.get("incoming_damage", 0)),
            bool(data.get("can_pass", True)),
            bool(data.get("can_cancel", True)),
        )
        self._errors.show_info(f"⚔️ {defender_id} si Difende: scegli l'azione difensiva")

    def _on_defense_window_closed(self, msg: dict) -> None:
        """Leaves reactive-defense mode and restores the turn actor selection."""
        self._defense_active = False
        self._actions.exit_defense_mode()
        # Restore focus to the actor whose turn it currently is.
        if self._current_turn_actor:
            self._heroes.select_actor(self._current_turn_actor)
            self._actions.set_selected_actor(self._current_turn_actor)

    def _on_attack_resolved(self, msg: dict) -> None:
        """Updates action state when CoreEngine reports attack resolution."""
        data: dict = msg.get("data", {}) or {}
        # Show damage message to player.
        defender_id: str = str(data.get("defender_id", ""))
        final_damage: int = int(data.get("final_damage", 0))
        if defender_id:
            self._errors.show_info(f"{defender_id} riceve {final_damage} danni")
        # Update remaining actions from CoreEngine state.
        actions_remaining = int(data.get("actions_remaining", self._current_actions_remaining))
        if actions_remaining != self._current_actions_remaining:
            self._current_actions_remaining = actions_remaining
            self._heroes.update_actions_remaining(self._current_turn_actor, actions_remaining)
        # If this attack is part of defense resolution, exit defense mode.
        if self._defense_active:
            self._defense_active = False
            self._actions.exit_defense_mode()

    def _on_actor_removed(self, msg: dict) -> None:
        """Handles dungeon.actor.removed — logs the defeat and shows a message."""
        data: dict = msg.get("data", {}) or {}
        actor_id: str = str(data.get("actor_id", ""))
        if actor_id:
            self._errors.show_info(f"☠️ {actor_id} è stato sconfitto e rimosso dalla mappa")
            self._log.on_envelope(msg)

    def _on_defend_requested(self, defender_id: str, mode: str, block: int) -> None:
        """Forwards an active defense choice (reduce / cancel) to CoreEngine."""
        self._bridge.send_command(
            "dungeon.defend",
            {"defender_id": defender_id, "mode": mode, "block": block})

    def _on_defend_pass_requested(self, defender_id: str) -> None:
        """Forwards a defense pass (take full damage minus stat) to CoreEngine."""
        self._bridge.send_command("dungeon.defend.pass", {"defender_id": defender_id})

    # ── Per-hero deck management (Phase 5+) ───────────────────────────────────

    def _load_card_catalog(self) -> list[dict]:
        """Reads cards_dungeon.json and returns the list of card dicts, or []."""
        catalog_path = Path(__file__).parent.parent.parent / "data" / "cards_dungeon.json"
        try:
            with catalog_path.open("r", encoding="utf-8") as fp:
                raw = json.load(fp)
            return list(raw.get("cards", []))
        except Exception:
            return []

    def _init_hero_deck(self, hero_id: str) -> None:
        """Creates a fresh deck for hero_id — all cards placed in MainDeck."""
        self._hero_decks[hero_id] = {z: [] for z in _DECK_ZONES}
        self._hero_decks[hero_id]["MainDeck"] = [dict(c) for c in self._card_catalog]

    def _inject_deck_zones(self, hero_id: str) -> None:
        """Refreshes the deck module with the saved zone state for hero_id."""
        if hero_id not in self._hero_decks:
            return
        deck = self._hero_decks[hero_id]
        for zone in _DECK_ZONES:
            self._deck.on_envelope({
                "typeId": "gmAlea.deck.zone_changed",
                "data": {"zone_name": zone, "cards": list(deck.get(zone, []))},
            })
        self._current_deck_hero = hero_id

    def _on_actor_snapshot_for_decks(self, msg: dict) -> None:
        """Initialises per-hero decks the first time each hero is seen."""
        for actor in msg.get("data", {}).get("actors", []):
            actor_id = str(actor.get("id", ""))
            kind = str(actor.get("kind", ""))
            if kind == "HERO" and actor_id and actor_id not in self._hero_decks:
                self._init_hero_deck(actor_id)

    def _start_card_attack(self, hero_id: str, card_id: str,
                           card_damage: int, max_distance: int) -> None:
        """Enters attack-targeting mode triggered by an ATTACK card played to PlayArea."""
        self._pending_move_hero = ""
        self._pending_card_move = {}
        self._pending_attack_attacker = ""
        self._actions.set_awaiting_move(False)
        self._pending_card_attack = {
            "hero_id":      hero_id,
            "card_id":      card_id,
            "card_damage":  card_damage,
            "max_distance": max_distance,
        }
        self._actions.set_awaiting_attack(True)
        self._errors.show_info(
            f"Carta \u2018{card_id}\u2019 (danno {card_damage}): "
            f"seleziona il bersaglio nel pannello Actors "
            f"(distanza max {max_distance})."
        )

    def _start_card_move(self, hero_id: str, card_id: str, max_distance: int) -> None:
        """Enters move-targeting mode triggered by a card played to PlayArea."""
        self._pending_move_hero = ""           # not a button-move
        self._pending_attack_attacker = ""
        self._actions.set_awaiting_attack(False)
        self._pending_card_move = {
            "hero_id": hero_id,
            "card_id": card_id,
            "max_distance": max_distance,
        }
        self._actions.set_awaiting_move(True)
        self._errors.on_envelope({
            "typeId": "dungeon.action.rejected",
            "data": {
                "reason": f"Carta '{card_id}': clicca sulla stanza destinazione (max {max_distance} locazioni).",
                "command": "dungeon.move",
            },
        })

    def _on_deck_command(self, type_id: str, data: dict) -> None:
        """Handles gmAlea.deck.* commands locally (no C++ engine manages decks)."""
        hero = self._current_deck_hero
        if not hero or hero not in self._hero_decks:
            return
        deck = self._hero_decks[hero]

        if type_id == "gmAlea.deck.move_card":
            card_id = str(data.get("card_id", ""))
            from_zone = str(data.get("from") or data.get("from_zone", ""))
            to_zone = str(data.get("to") or data.get("to_zone", ""))
            cards_from = deck.get(from_zone, [])
            card = next((c for c in cards_from if c.get("card_id") == card_id), None)
            if card:
                cards_from.remove(card)
                deck.setdefault(to_zone, []).insert(0, card)
            self._deck.on_envelope({
                "typeId": "gmAlea.deck.card_moved",
                "data": {"card_id": card_id, "from_zone": from_zone, "to_zone": to_zone},
            })
            # When a card lands in PlayArea, trigger the corresponding game action.
            # Do NOT decrement actions here — the CoreEngine controls action logic.
            # The GUI will receive feedback (ACTION_REJECTED or the action succeeds)
            # and then update from CoreEngine state.
            if to_zone == "PlayArea" and card_id in _CARD_ACTIONS:
                action = _CARD_ACTIONS[card_id]
                if action["action_type"] == "MOVE":
                    self._start_card_move(hero, card_id, int(action.get("max_distance", 1)))
                elif action["action_type"] == "ATTACK":
                    self._start_card_attack(
                        hero, card_id,
                        int(action.get("card_damage", 0)),
                        int(action.get("max_distance", 1)),
                    )

        elif type_id == "gmAlea.deck.recycle_discard":
            discarded = list(deck.get("DiscardPile", []))
            deck["DiscardPile"] = []
            deck["MainDeck"] = list(deck.get("MainDeck", [])) + discarded
            for zone in ("DiscardPile", "MainDeck"):
                self._deck.on_envelope({
                    "typeId": "gmAlea.deck.zone_changed",
                    "data": {"zone_name": zone, "cards": list(deck.get(zone, []))},
                })

    def closeEvent(self, event) -> None:
        """Saves the dock layout and stops the bridge receiver before closing."""
        settings = QSettings("GameLib", "DungeonCrawlerBasic")
        settings.setValue("layout/main", self.saveState())
        settings.setValue("layout/actor_inner", self._actor_inner.saveState())
        self._bridge.receiver.stop()
        self._bridge.receiver.wait(2000)
        super().closeEvent(event)
