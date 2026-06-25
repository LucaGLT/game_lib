"""Integration & E2E tests for gmGui — Phase 10.

A ``MockEngine`` replaces the C++ Core Engine:
- connects to EngineReceiver (:19000) and sends canned event frames,
- listens on :19001 and captures GUI command frames.

The ``e2e_env`` fixture wires a real ``MainWindow`` (with patched bridge
ports) to a ``MockEngine`` so these tests never touch the real 9000/9001
port pair.

Qt signals emitted from EngineReceiver's background thread are delivered to
the main thread by ``_wait()``, which calls ``QApplication.processEvents()``
in a polling loop.
"""
from __future__ import annotations

import functools
import json
import os
import sys
import time
from unittest.mock import patch

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.engine_bridge.receiver import EngineReceiver
from gmGui.engine_bridge.sender import EngineSender
from gmGui.engine_bridge.framing import send_frame
from gmGui.tests.mock_engine import MockEngine

# ── Test-only TCP ports ───────────────────────────────────────────────────────

_EVENT_PORT: int = 19000   # EngineReceiver listens; MockEngine connects as client
_CMD_PORT: int = 19001     # MockEngine listens; EngineSender connects as client

# Partials that create EngineReceiver/EngineSender with test ports.
_TestReceiver = functools.partial(EngineReceiver, port=_EVENT_PORT)
_TestSender = functools.partial(EngineSender, port=_CMD_PORT)


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Qt event helper ───────────────────────────────────────────────────────────

def _wait(condition, timeout: float = 3.0, interval: float = 0.05) -> bool:
    """Polls Qt events until *condition()* is True or *timeout* seconds elapse.

    Returns ``True`` if the condition became true, ``False`` on timeout.
    """
    deadline = time.time() + timeout
    while time.time() < deadline:
        QApplication.processEvents()
        if condition():
            return True
        time.sleep(interval)
    QApplication.processEvents()
    return condition()


# ── E2E fixture ───────────────────────────────────────────────────────────────

@pytest.fixture
def e2e_env(qapp: QApplication):
    """Yields ``(win, engine)``: a live MainWindow wired to a MockEngine.

    Sequence:
      1. MockEngine.start_cmd_server()   — bind :19001
      2. MainWindow()                    — EngineReceiver binds :19000
      3. MockEngine.connect_to_receiver() — connect to :19000
    The first ``send_command`` from the GUI lazily connects to :19001.
    """
    engine = MockEngine(event_port=_EVENT_PORT, cmd_port=_CMD_PORT)
    engine.start_cmd_server()

    with patch("gmGui.main_window.EngineReceiver", _TestReceiver), \
         patch("gmGui.main_window.EngineSender", _TestSender):
        from gmGui.main_window import MainWindow
        win = MainWindow()

    # EngineReceiver is now listening on _EVENT_PORT; connect the mock.
    engine.connect_to_receiver(timeout=5.0)

    try:
        yield win, engine
    finally:
        engine.stop()
        try:
            win._receiver.stop()
        except Exception:
            pass
        try:
            win._sender.close()
        except Exception:
            pass
        try:
            win.close()
        except Exception:
            pass


# ── Envelope builders ─────────────────────────────────────────────────────────

def _env(type_id: str, data: dict) -> dict:
    """Shorthand: envelope dict for direct on_envelope() injection."""
    return {"typeId": type_id, "data": data,
            "headers": {"data": json.dumps(data)}}


# ══════════════════════════════════════════════════════════════════════════════
# Test classes
# ══════════════════════════════════════════════════════════════════════════════

class TestEventSequence:
    """Engine events sent via real TCP update module widgets correctly."""

    def test_session_started_updates_flow_label(
        self, e2e_env: tuple
    ) -> None:
        """gmFlow.session.started propagates through TCP → EngineReceiver Signal →
        MainWindow._on_envelope → GmFlowModule label update."""
        win, engine = e2e_env
        flow = next(m for m in win._modules if m.module_id == "gm_flow")
        flow.widget()

        engine.send_event("gmFlow.session.started", {"session_id": "e2e_001"})

        assert _wait(
            lambda: "e2e_001" in flow._lbl_session.text()
        ), f"Label text: {flow._lbl_session.text()!r}"

    def test_actor_snapshot_populates_tree(self, e2e_env: tuple) -> None:
        """gmActor.snapshot received via TCP populates GmActorModule tree."""
        win, engine = e2e_env
        actor = next(m for m in win._modules if m.module_id == "gm_actor")
        actor.widget()

        engine.send_event("gmActor.snapshot", {
            "actors": [
                {
                    "actor_id": "player_x",
                    "display_name": "Player X",
                    "faction_id": "Players",
                    "kind": "HERO",
                    "life_state": "ALIVE",
                    "current_hp": 3,
                    "max_hp": 3,
                },
                {
                    "actor_id": "player_o",
                    "display_name": "Player O",
                    "faction_id": "Players",
                    "kind": "HERO",
                    "life_state": "ALIVE",
                    "current_hp": 3,
                    "max_hp": 3,
                },
            ]
        })

        assert _wait(lambda: "player_x" in actor._actor_items), \
            "player_x not in actor tree"
        assert "player_o" in actor._actor_items

    def test_map_loaded_creates_scene_nodes(self, e2e_env: tuple) -> None:
        """gmMap.map.loaded received via TCP creates correct nodes and edges."""
        win, engine = e2e_env
        gm_map = next(m for m in win._modules if m.module_id == "gm_map")
        gm_map.widget()

        engine.send_event("gmMap.map.loaded", {
            "locations": [{"location_id": i} for i in range(5)],
            "edges": [[i, i + 1] for i in range(4)],
        })

        assert _wait(lambda: gm_map._map_scene.node_count() == 5), \
            f"node_count={gm_map._map_scene.node_count()}"
        assert gm_map._map_scene.edge_count() == 4

    def test_dice_roll_result_updates_labels(self, e2e_env: tuple) -> None:
        """gmAlea.dice.roll_result updates result and detail labels."""
        win, engine = e2e_env
        dice = next(m for m in win._modules if m.module_id == "gm_dice")
        dice.widget()

        engine.send_event("gmAlea.dice.roll_result", {"dice": [4, 3], "total": 7})

        assert _wait(lambda: dice._result_label.text() == "7"), \
            f"result label: {dice._result_label.text()!r}"
        assert dice._detail_label.text() == "4 + 3"


class TestCommandFlow:
    """GUI commands triggered by buttons arrive at the mock engine."""

    def _ensure_sender_connected(self, win, engine) -> None:
        """Forces EngineSender to establish its lazy TCP connection."""
        win._sender.send_command("_ping", {})
        # Allow time for MockEngine's cmd thread to accept + read the frame.
        engine.wait_for_command("_ping", timeout=2.0)
        engine.clear_commands()

    def test_pause_button_sends_command(self, e2e_env: tuple) -> None:
        """PAUSE button sends 'gmFlow.session.pause' to the engine."""
        win, engine = e2e_env
        flow = next(m for m in win._modules if m.module_id == "gm_flow")
        flow.widget()

        # Enable the PAUSE button via a session.started event.
        engine.send_event("gmFlow.session.started", {"session_id": "cmd_test"})
        assert _wait(lambda: flow._btn_pause.isEnabled()), \
            "PAUSE button not enabled after session.started"

        self._ensure_sender_connected(win, engine)

        flow._btn_pause.click()
        QApplication.processEvents()

        result = engine.wait_for_command("gmFlow.session.pause", timeout=3.0)
        assert result is not None, \
            f"'gmFlow.session.pause' not received; got: {engine.received_commands}"

    def test_pass_turn_button_sends_command(self, e2e_env: tuple) -> None:
        """Passa Turno button sends 'gmFlow.turn.pass' to the engine."""
        win, engine = e2e_env
        flow = next(m for m in win._modules if m.module_id == "gm_flow")
        flow.widget()

        engine.send_event("gmFlow.session.started", {"session_id": "cmd_turn_pass"})
        assert _wait(lambda: flow._btn_pass_turn.isEnabled()), \
            "Passa Turno button not enabled after session.started"

        self._ensure_sender_connected(win, engine)
        engine.clear_commands()

        flow._btn_pass_turn.click()
        QApplication.processEvents()

        result = engine.wait_for_command("gmFlow.turn.pass", timeout=3.0)
        assert result is not None, \
            f"'gmFlow.turn.pass' not received; got: {engine.received_commands}"

    def test_roll_button_sends_command(self, e2e_env: tuple) -> None:
        """[LANCIA] button sends 'gmAlea.dice.roll_request' to the engine."""
        win, engine = e2e_env
        dice = next(m for m in win._modules if m.module_id == "gm_dice")
        dice.widget()

        self._ensure_sender_connected(win, engine)
        engine.clear_commands()

        dice._roll_btn.click()
        QApplication.processEvents()

        result = engine.wait_for_command("gmAlea.dice.roll_request", timeout=3.0)
        assert result is not None, \
            f"'gmAlea.dice.roll_request' not received; got: {engine.received_commands}"
        assert "count" in result.get("data", {})
        assert "faces" in result.get("data", {})


class TestConnectionManagement:
    """Resilience tests for bridge connect/disconnect lifecycle."""

    def test_connection_lost_updates_status_bar(self, e2e_env: tuple) -> None:
        """When MockEngine closes the event socket, QStatusBar shows 'Disconnesso'."""
        win, engine = e2e_env

        # Send one event so the status bar transitions to 'Connesso'.
        engine.send_event("gmFlow.session.started", {"session_id": "disconnect_test"})
        assert _wait(
            lambda: "Connesso" in win._conn_label.text(),
            timeout=3.0,
        ), f"Status before disconnect: {win._conn_label.text()!r}"

        # Simulate engine crash: close the event socket.
        engine.disconnect_event()

        # EngineReceiver detects the closure within ~1 s (its recv timeout).
        assert _wait(
            lambda: "Disconnesso" in win._conn_label.text(),
            timeout=4.0,
        ), f"Status after disconnect: {win._conn_label.text()!r}"

    def test_malformed_json_does_not_crash(self, e2e_env: tuple) -> None:
        """A malformed JSON frame is silently discarded — no crash or exception."""
        win, engine = e2e_env

        if engine._event_sock is None:
            pytest.skip("Event socket not available")

        # Bypass send_event() and send a raw invalid JSON frame.
        send_frame(engine._event_sock, "{this: is not json}")

        # Process Qt events for 500 ms; the test passes if no exception is raised.
        _wait(lambda: False, timeout=0.5)


class TestFullSessionSmoke:
    """Full simulated game session: actors, 3x3 map, flow, and dice."""

    def test_tris_session_smoke(self, e2e_env: tuple) -> None:
        """Simulates a complete Tris-like session via TCP and verifies all modules."""
        win, engine = e2e_env

        flow = next(m for m in win._modules if m.module_id == "gm_flow")
        actor = next(m for m in win._modules if m.module_id == "gm_actor")
        gm_map = next(m for m in win._modules if m.module_id == "gm_map")
        dice = next(m for m in win._modules if m.module_id == "gm_dice")
        for mod in (flow, actor, gm_map, dice):
            mod.widget()

        # ── 1. Actor snapshot: 2 players ─────────────────────────────────────
        engine.send_event("gmActor.snapshot", {
            "actors": [
                {"actor_id": "player_x", "display_name": "Player X",
                 "faction_id": "Players", "kind": "HERO",
                 "life_state": "ALIVE", "current_hp": 3, "max_hp": 3},
                {"actor_id": "player_o", "display_name": "Player O",
                 "faction_id": "Players", "kind": "HERO",
                 "life_state": "ALIVE", "current_hp": 3, "max_hp": 3},
            ]
        })

        # ── 2. Map: 3×3 grid (9 locations, 12 edges) ─────────────────────────
        locs = [{"location_id": r * 3 + c} for r in range(3) for c in range(3)]
        h_edges = [[r * 3 + c, r * 3 + c + 1]
                   for r in range(3) for c in range(2)]
        v_edges = [[r * 3 + c, (r + 1) * 3 + c]
                   for r in range(2) for c in range(3)]
        engine.send_event("gmMap.map.loaded",
                          {"locations": locs, "edges": h_edges + v_edges})

        # ── 3. Session started + phase + round ───────────────────────────────
        engine.send_event("gmFlow.session.started", {"session_id": "tris_001"})
        engine.send_event("gmFlow.phase.entered",   {"phase_id": "PLAY"})
        engine.send_event("gmFlow.round.started",   {"round_number": 1})

        # ── 4. Two dice rolls (decide first player) ───────────────────────────
        engine.send_event("gmAlea.dice.roll_result", {"dice": [5], "total": 5})
        engine.send_event("gmAlea.dice.roll_result", {"dice": [2], "total": 2})

        # ── 5. Three turns: X → O → X ────────────────────────────────────────
        turn_map = [("player_x", 0), ("player_o", 4), ("player_x", 8)]
        for turn_n, (actor_id, cell) in enumerate(turn_map, start=1):
            engine.send_event("gmFlow.turn.started", {
                "turn_number": turn_n, "active_actors": [actor_id]
            })
            engine.send_event("gmActor.actor.status_added", {
                "actor_id": actor_id, "status_id": "ACTIVE_TURN", "stacks": 1
            })
            engine.send_event("gmActor.actor.moved_area", {
                "actor_id": actor_id, "new_area_id": cell
            })
            engine.send_event("gmActor.actor.status_removed", {
                "actor_id": actor_id, "status_id": "ACTIVE_TURN"
            })
            engine.send_event("gmFlow.turn.ended", {"turn_number": turn_n})

        # ── 6. Session completed ──────────────────────────────────────────────
        engine.send_event("gmFlow.session.completed", {"winner": "player_x"})

        # ── Assertions ────────────────────────────────────────────────────────
        assert _wait(lambda: not flow._btn_pause.isEnabled()), \
            "PAUSE should be disabled after session.completed"

        assert _wait(lambda: "tris_001" in flow._lbl_session.text()), \
            f"Session label: {flow._lbl_session.text()!r}"

        assert _wait(lambda: "player_x" in actor._actor_items), \
            "player_x not in actor tree"

        assert _wait(lambda: gm_map._map_scene.node_count() == 9), \
            f"Map nodes: {gm_map._map_scene.node_count()}"
        assert gm_map._map_scene.edge_count() == 12

        assert _wait(lambda: dice._history_list.count() == 2), \
            f"Dice history: {dice._history_list.count()} entries"

        assert _wait(
            lambda: gm_map._map_scene.marker_location("player_x") == 8
        ), "player_x marker not at cell 8"
