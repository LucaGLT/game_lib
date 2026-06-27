"""Le Pergamene di Eldhom — main window.

EldhomMainWindow assembles the full application layout:

- Centre:   EldhomMapWidget  (3-location map with actor tokens)
- Top-left: HeroPanelWidget x2  (Thael + Velyr)
- Middle:   TimelineWidget
- Bottom:   ActionPanelWidget + HandWidget
- Right:    LogWidget (dock)

All incoming engine events are dispatched via EldhomBridge → EventRouter.
All outgoing commands are sent via EldhomBridge.send_command().
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

from PySide6.QtWidgets import (
    QApplication,
    QDockWidget,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QSplitter,
    QStatusBar,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt, QTimer

from app.eldhom_bridge import EldhomBridge
from app.event_router import EventRouter
from app.mission_select_dialog import MissionSelectDialog
from widgets.map_widget import EldhomMapWidget
from widgets.hero_panel_widget import HeroPanelWidget
from widgets.hand_widget import HandWidget
from widgets.action_panel_widget import ActionPanelWidget
from widgets.timeline_widget import TimelineWidget
from widgets.log_widget import LogWidget

# Path to the data directory: GUI/app/ → GUI/ → Eldhom/ → Eldhom/data/
_DATA_DIR = Path(__file__).resolve().parents[2] / "data"

# Card name lookup: populated from full-state snapshot
_CARD_NAMES: dict[str, str] = {}


class EldhomMainWindow(QMainWindow):
    """Main application window for Le Pergamene di Eldhom.

    No game logic lives here — only presentation and orchestration.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Le Pergamene di Eldhom")
        self.resize(1200, 760)
        self._center_on_screen()

        self._active_hero_id: str = ""
        self._hero_data: dict[str, dict] = {}          # hero_id → hero dict
        self._group_data: dict[str, dict] = {}         # group_id → group dict
        self._location_adjacency: dict[str, list[str]] = {}
        self._mission_started: bool = False
        self._hand_cards: dict[str, list[str]] = {}    # hero_id → [card_id]

        self._build_layout()
        self._build_bridge()
        self._build_router()
        self._setup_status_bar()

        # Show mission selection on startup (deferred so window is visible)
        QTimer.singleShot(300, self._show_mission_select)

    # ── Layout ────────────────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        central = QWidget(self)
        self.setCentralWidget(central)

        root_layout = QVBoxLayout(central)
        root_layout.setSpacing(4)
        root_layout.setContentsMargins(4, 4, 4, 4)

        # ── Top row: hero panels + title ──────────────────────────────────────
        top_row = QHBoxLayout()

        self._thael_panel = HeroPanelWidget("thael", self)
        self._velyr_panel = HeroPanelWidget("velyr", self)
        self._hero_panels: dict[str, HeroPanelWidget] = {
            "thael": self._thael_panel,
            "velyr": self._velyr_panel,
        }

        self._title_label = QLabel("Le Pergamene di Eldhom", self)
        self._title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._title_label.setStyleSheet(
            "color:#c8a060; font-size:16px; font-weight:bold;"
        )
        self._time_label = QLabel("⌛ 0", self)
        self._time_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        self._time_label.setStyleSheet("color:#8ab; font-size:14px; padding-right:8px;")

        top_row.addWidget(self._thael_panel)
        top_row.addWidget(self._velyr_panel)
        top_row.addWidget(self._title_label, 1)
        top_row.addWidget(self._time_label)

        root_layout.addLayout(top_row)

        # ── Map widget ────────────────────────────────────────────────────────
        self._map_widget = EldhomMapWidget(self)
        root_layout.addWidget(self._map_widget, 3)

        # ── Timeline strip ────────────────────────────────────────────────────
        self._timeline_widget = TimelineWidget(self)
        root_layout.addWidget(self._timeline_widget)

        # ── Action panel (simple actions) ──────────────────────────────────────
        self._action_panel = ActionPanelWidget(self)
        root_layout.addWidget(self._action_panel)

        # ── Hand widget ───────────────────────────────────────────────────────
        self._hand_widget = HandWidget(self)
        root_layout.addWidget(self._hand_widget)

        # ── Log dock (right) ──────────────────────────────────────────────────
        self._log_widget = LogWidget(self)
        log_dock = QDockWidget("Log", self)
        log_dock.setWidget(self._log_widget)
        log_dock.setFeatures(
            QDockWidget.DockWidgetFeature.DockWidgetMovable
            | QDockWidget.DockWidgetFeature.DockWidgetFloatable
        )
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, log_dock)

        # ── Wire action signals ───────────────────────────────────────────────
        self._action_panel.action_move.connect(self._on_simple_move)
        self._action_panel.action_attack.connect(self._on_simple_attack)
        self._action_panel.action_recover.connect(self._on_simple_recover)
        self._action_panel.stop_sequence.connect(self._on_stop_sequence)
        self._hand_widget.card_selected.connect(self._on_card_selected)

        # Apply base stylesheet only when ThemeManager is NOT used (fallback).
        # When ThemeManager is active it sets the global QApplication stylesheet.

    def _build_bridge(self) -> None:
        self._bridge = EldhomBridge()
        self._bridge.receiver.start()
        print("[EldhomGUI] Event receiver listening on port 9210", flush=True)

        # Route events through the Qt signal-slot mechanism for thread safety
        self._bridge.set_on_event(self._on_engine_event_thread)

    def _build_router(self) -> None:
        self._router = EventRouter()

        def reg(type_id: str, handler):
            self._router.register(type_id, handler)

        # Full state
        reg("eldhom.state.full",         self._on_state_full)

        # Turn management
        reg("eldhom.turn.next_actor",    self._on_next_actor)
        reg("eldhom.pg.turn_ended",      self._on_pg_turn_ended)

        # PG events
        reg("eldhom.pg.played_card",     self._log_event)
        reg("eldhom.pg.moved",           self._log_event)
        reg("eldhom.pg.attacked",        self._log_event)
        reg("eldhom.pg.healed",          self._log_event)
        reg("eldhom.pg.ko",              self._log_event)

        # Deck events
        reg("eldhom.deck.hand_updated",  self._on_hand_updated)
        reg("eldhom.deck.reshuffled",    self._log_event)

        # Monster events
        reg("eldhom.monster.defeated",   self._on_monster_defeated)
        reg("eldhom.monster.damaged",    self._log_event)
        reg("eldhom.group.activated",    self._log_event)
        reg("eldhom.group.eliminated",   self._log_event)

        # Formation
        reg("eldhom.formation.changed",  self._on_formation_changed)

        # Time
        reg("eldhom.mission.time_advanced", self._on_time_advanced)

        # Mission end
        reg("eldhom.mission.victory",    self._on_victory)
        reg("eldhom.mission.defeat",     self._on_defeat)

        # Action result (errors)
        reg("eldhom.action.result",      self._on_action_result)

        # Map + timeline
        reg("eldhom.state.full",         self._map_widget.on_state_full)
        reg("eldhom.monster.defeated",   self._map_widget.on_monster_defeated)
        reg("eldhom.state.full",         self._timeline_widget.on_state_full)
        reg("eldhom.turn.next_actor",    self._timeline_widget.on_next_actor)

        # Log
        for evt in (
            "eldhom.pg.played_card", "eldhom.pg.moved", "eldhom.pg.attacked",
            "eldhom.pg.healed", "eldhom.pg.ko", "eldhom.monster.damaged",
            "eldhom.monster.defeated", "eldhom.group.activated",
            "eldhom.group.eliminated", "eldhom.formation.changed",
            "eldhom.deck.reshuffled", "eldhom.mission.time_advanced",
        ):
            reg(evt, self._log_widget.on_any_event)

        reg("eldhom.action.result",   self._log_widget.on_action_result)
        reg("eldhom.mission.victory", self._log_widget.on_mission_victory)
        reg("eldhom.mission.defeat",  self._log_widget.on_mission_defeat)

    def _setup_status_bar(self) -> None:
        sb = QStatusBar(self)
        self._status_label = QLabel("Non connesso — avvia eldhom_engine.exe", self)
        self._status_label.setStyleSheet("color:#888;")
        sb.addWidget(self._status_label)
        self.setStatusBar(sb)

    # ── Event routing (thread-safe) ────────────────────────────────────────────

    def _on_engine_event_thread(self, msg: dict) -> None:
        """Called from the receiver background thread — use QTimer for thread safety."""
        type_id = msg.get("typeId", "?")
        print(f"[EldhomGUI] ✓ Received event: {type_id}", flush=True)
        QTimer.singleShot(0, lambda: self._router.dispatch(msg))

    # ── Event handlers ─────────────────────────────────────────────────────────

    def _on_state_full(self, msg: dict) -> None:
        data = _extract_data(msg)
        self._mission_started = True
        self._status_label.setText(
            f"Missione: {data.get('title', '—')}  •  ⌛ {data.get('time', 0)}"
        )
        self._title_label.setText(data.get("title", "Le Pergamene di Eldhom"))
        self._time_label.setText(f"⌛ {data.get('time', 0)}")

        # Update hero panels
        self._hero_data.clear()
        for hero in data.get("heroes", []):
            hid = hero["id"]
            self._hero_data[hid] = hero
            if hid in self._hero_panels:
                self._hero_panels[hid].update_from_dict(hero)
            # Store hand
            self._hand_cards[hid] = hero.get("hand", [])

        # Build adjacency
        self._location_adjacency.clear()
        for loc in data.get("locations", []):
            self._location_adjacency[loc["id"]] = loc.get("adjacent", [])

        # Update groups
        self._group_data.clear()
        for grp in data.get("groups", []):
            self._group_data[grp["id"]] = grp

        # Activate next actor
        next_a = data.get("next_actor", {})
        if next_a:
            self._activate_actor(next_a.get("actor_id", ""), next_a.get("kind", ""))

    def _on_next_actor(self, msg: dict) -> None:
        data = _extract_data(msg)
        self._activate_actor(data.get("actor_id", ""), data.get("kind", ""))

    def _activate_actor(self, actor_id: str, kind: str) -> None:
        """Highlights the active actor and enables/disables controls."""
        self._active_hero_id = actor_id if kind == "HERO" else ""

        # Update hero panel borders
        for hid, panel in self._hero_panels.items():
            panel.set_active(hid == actor_id)

        if kind == "HERO" and actor_id in self._hero_data:
            hero = self._hero_data[actor_id]
            loc  = hero.get("location", "")
            adj  = self._location_adjacency.get(loc, [])
            self._action_panel.set_turn(
                hero.get("name", actor_id),
                adj,
                sequence_active=False,
            )
            # Show hand
            hand_ids = self._hand_cards.get(actor_id, [])
            self._hand_widget.set_hand(
                hero.get("name", actor_id),
                _build_card_list(hand_ids),
                enabled=True,
            )
        else:
            self._action_panel.set_enabled(False)
            self._hand_widget.set_enabled(False)

    def _on_pg_turn_ended(self, msg: dict) -> None:
        """Disable controls when a hero turn ends."""
        data = _extract_data(msg)
        actor_id = data.get("actor_id", "")
        if actor_id == self._active_hero_id:
            self._action_panel.set_enabled(False)
            self._hand_widget.set_enabled(False)

    def _on_hand_updated(self, msg: dict) -> None:
        """Refreshes the hand widget when the engine sends a new hand."""
        data     = _extract_data(msg)
        hero_id  = data.get("actor_id", "")
        payload  = data.get("payload", [])
        if isinstance(payload, list):
            hand_ids = [str(c) for c in payload]
        else:
            hand_ids = []
        self._hand_cards[hero_id] = hand_ids

        if hero_id == self._active_hero_id:
            name = self._hero_data.get(hero_id, {}).get("name", hero_id)
            self._hand_widget.set_hand(
                name,
                _build_card_list(hand_ids),
                enabled=True,
            )
        # Update hero panel deck count
        if hero_id in self._hero_data:
            self._hero_data[hero_id]["hand"] = hand_ids

    def _on_time_advanced(self, msg: dict) -> None:
        data = _extract_data(msg)
        time_val = data.get("payload", 0)
        self._time_label.setText(f"⌛ {time_val}")

    def _on_formation_changed(self, msg: dict) -> None:
        self._log_event(msg)

    def _on_monster_defeated(self, msg: dict) -> None:
        self._log_event(msg)

    def _on_action_result(self, msg: dict) -> None:
        data = _extract_data(msg)
        if not data.get("ok", True):
            error = data.get("error", "Errore")
            self._status_label.setText(f"⚠ {error}")
        else:
            self._status_label.setText(
                f"Missione: {self._title_label.text()}"
            )

    def _on_victory(self, msg: dict) -> None:
        self._action_panel.set_enabled(False)
        self._hand_widget.set_enabled(False)
        QTimer.singleShot(
            200,
            lambda: QMessageBox.information(
                self, "Vittoria!", "🏆 Missione completata con successo!"
            ),
        )

    def _on_defeat(self, msg: dict) -> None:
        data    = _extract_data(msg)
        payload = data.get("payload", "")
        self._action_panel.set_enabled(False)
        self._hand_widget.set_enabled(False)
        QTimer.singleShot(
            200,
            lambda: QMessageBox.warning(
                self, "Sconfitta", f"💀 Missione fallita!\n{payload}"
            ),
        )

    def _log_event(self, msg: dict) -> None:
        """Forwards event to the log widget."""
        self._log_widget.on_any_event(msg)

    # ── Player action senders ─────────────────────────────────────────────────

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

    # ── Mission selection ─────────────────────────────────────────────────────

    def _show_mission_select(self) -> None:
        dialog = MissionSelectDialog(_DATA_DIR, self)
        if dialog.exec() == MissionSelectDialog.DialogCode.Accepted:
            mission_id = dialog.selected_mission_id
            if mission_id:
                print(f"[EldhomGUI] 📤 Sending start_mission for: {mission_id}", flush=True)
                self._log_widget.clear()
                self._log_widget.append(
                    f"Caricamento missione: {mission_id}…", "#8ab"
                )
                self._bridge.send_command(
                    "eldhom.start_mission", {"mission_id": mission_id}
                )
                print(f"[EldhomGUI] 📤 Command sent.", flush=True)
        else:
            self._log_widget.append(
                "Nessuna missione selezionata. Seleziona Missione dal menu per iniziare.",
                "#888",
            )

    # ── Helpers ───────────────────────────────────────────────────────────────

    def _center_on_screen(self) -> None:
        screen = QApplication.primaryScreen()
        if screen:
            geo = screen.availableGeometry()
            self.move(
                geo.center().x() - self.width() // 2,
                geo.center().y() - self.height() // 2,
            )

    def closeEvent(self, event) -> None:
        self._bridge.receiver.stop()
        super().closeEvent(event)


# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────

def _extract_data(msg: dict) -> dict:
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    try:
        return json.loads(raw)
    except Exception:
        return {}


def _build_card_list(card_ids: list[str]) -> list[dict]:
    """Converts a list of card IDs into display dicts for HandWidget.

    Uses the global ``_CARD_NAMES`` lookup when available.
    """
    result = []
    for cid in card_ids:
        # Simple heuristic: derive name from id by replacing underscores
        name = _CARD_NAMES.get(cid, cid.replace("_", " ").title())
        result.append({
            "card_id": cid,
            "name":    name,
        })
    return result
