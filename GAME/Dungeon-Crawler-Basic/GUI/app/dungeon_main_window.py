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
from gmGui.modules.gm_comp_deck_module import GmCompDeckModule

# Zone names that match GmCompDeckModule exactly.
_DECK_ZONES: list[str] = ["MainDeck", "CardHand", "PlayArea", "Memory", "DiscardPile", "BanishZone"]


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
        """Creates and arranges all child widgets."""
        # ToBeImplemented //
        self._board     = DungeonBoardWidget()
        self._heroes    = HeroPanelWidget()
        self._actions   = ActionPanelWidget()
        self._log       = LogWidget()
        self._errors    = ErrorBarWidget()
        self._area_info = GmMapAreaInfoModule()
        self._flow      = GmFlowModule()
        self._deck      = GmCompDeckModule()

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

        deck_dock = QDockWidget("Carte", self)
        deck_dock.setObjectName(self._deck.module_id)
        deck_dock.setWidget(self._deck.widget())
        self.addDockWidget(Qt.BottomDockWidgetArea, deck_dock)
        self._deck.on_attach()

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
        # Per-hero deck state: hero_id → zone_name → list[card dict].
        self._card_catalog: list[dict] = self._load_card_catalog()
        self._hero_decks: dict[str, dict[str, list[dict]]] = {}
        self._current_deck_hero: str = ""
        # Deck manager: intercept gmAlea.deck.* locally; forward the rest.
        self._deck_proxy = _DeckProxy(self._bridge.sender, self._on_deck_command)
        self._deck.set_sender(self._deck_proxy)
        self._last_round: int = 0
        self._pending_move_hero: str = ""  # hero_id waiting for a destination click
        self._pending_attack_attacker: str = ""  # attacker waiting for a target click
        self._defense_active: bool = False  # True while a defense window is open
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
        # Combat events also feed the log for player feedback.
        for ev in ("dungeon.attack.declared", "dungeon.defense.window.opened",
                   "dungeon.attack.resolved"):
            self._router.register(ev, self._log.on_envelope)
        # Area Info: contents of the selected map area (shared contract)
        self._router.register(AREA_INFO_RESPONSE, self._area_info.on_envelope)
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
        self._pending_attack_attacker = ""
        self._actions.set_awaiting_move(False)
        self._actions.set_awaiting_attack(False)
        actor_id: str = str(msg.get("data", {}).get("actor_id", ""))
        if actor_id:
            self._board.set_active_hero(actor_id)
            self._heroes.select_actor(actor_id)
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

        While in attack-targeting mode the selected actor is the *target* and an
        attack is dispatched; otherwise the selection just drives the action
        panel display (default behaviour).
        """
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

    def _on_defense_window_closed(self, msg: dict) -> None:
        """Leaves reactive-defense mode and restores normal action availability."""
        self._defense_active = False
        self._actions.exit_defense_mode()

    def _on_attack_resolved(self, msg: dict) -> None:
        """Restores the active actor selection after an attack is resolved."""
        if self._defense_active:
            self._defense_active = False
            self._actions.exit_defense_mode()

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
        """Stops the bridge receiver before closing."""
        self._bridge.receiver.stop()
        self._bridge.receiver.wait(2000)
        super().closeEvent(event)
