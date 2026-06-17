"""End-to-end test: drives the real tris_engine executable over TCP.

Acts as the GUI side of the bridge:
  - listens for engine events on port 9100 (engine connects as client),
  - sends commands to the engine's CmdServer on port 9001.

Then it plays scripted scenarios and asserts the emitted event stream, covering
the Phase 4 requirement "Test integrazione end-to-end Engine <-> GUI" plus the
robustness edge cases (out-of-range / wrong-turn / occupied moves, commands
during GAME_OVER, immediate restart, draw, and the 1d2 starter roll).

Usage::

    python e2e_test.py [path/to/tris_engine[.exe]]

If the engine path is omitted, the script tries to locate the Debug build under
``build/GAME/Tic-Tac-Toe/CoreEngine``.
"""
from __future__ import annotations

import json
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

# Reuse the shared framing helpers (4-byte big-endian length prefix + JSON).
_PYLIB_DIR = Path(__file__).resolve().parents[4] / "pyLib" / "gmGui"
if _PYLIB_DIR.is_dir() and str(_PYLIB_DIR) not in sys.path:
    sys.path.append(str(_PYLIB_DIR))

from engine_bridge.framing import recv_frame, send_frame  # noqa: E402

_EVENT_PORT = 9100
_COMMAND_PORT = 9001
_HOST = "127.0.0.1"


class _Failure(Exception):
    """Raised when an assertion in a scenario does not hold."""


def _locate_engine() -> Path:
    """Returns the engine executable path from argv or the default build tree."""
    if len(sys.argv) > 1:
        return Path(sys.argv[1])
    root = Path(__file__).resolve().parents[4]
    base = root / "build" / "GAME" / "Tic-Tac-Toe" / "CoreEngine"
    for candidate in (base / "Debug" / "tris_engine.exe",
                      base / "Release" / "tris_engine.exe",
                      base / "tris_engine.exe",
                      base / "tris_engine"):
        if candidate.exists():
            return candidate
    raise _Failure("tris_engine executable not found; pass its path as argv[1]")


class EngineHarness:
    """Owns the engine subprocess and both sides of the bridge."""

    def __init__(self, engine_path: Path) -> None:
        self._engine_path = engine_path
        self._proc: subprocess.Popen | None = None
        self._cmd: socket.socket | None = None
        self._events: socket.socket | None = None

    # ── Lifecycle ─────────────────────────────────────────────────────────────

    def start(self) -> None:
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((_HOST, _EVENT_PORT))
        srv.listen(1)
        srv.settimeout(10.0)

        self._proc = subprocess.Popen(
            [str(self._engine_path)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        self._cmd = self._connect_commands()
        # The engine connects back lazily on its first emitted event, which is
        # triggered by the first new_game command below.
        self.send("gmTris.new_game", {"starter_mode": "fixed_x"})

        try:
            self._events, _ = srv.accept()
        finally:
            srv.close()
        self._events.settimeout(0.6)
        # Drain the start-up burst from that first new_game.
        self.collect()

    def _connect_commands(self) -> socket.socket:
        deadline = time.time() + 10.0
        while time.time() < deadline:
            try:
                sock = socket.create_connection((_HOST, _COMMAND_PORT), timeout=1.0)
                return sock
            except OSError:
                time.sleep(0.1)
        raise _Failure("could not connect to engine command port 9001")

    def stop(self) -> None:
        for sock in (self._cmd, self._events):
            if sock is not None:
                try:
                    sock.close()
                except OSError:
                    pass
        if self._proc is not None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                self._proc.kill()

    # ── Bridge I/O ────────────────────────────────────────────────────────────

    def send(self, type_id: str, data: dict) -> None:
        """Sends a command frame to the engine."""
        assert self._cmd is not None
        send_frame(self._cmd, json.dumps({"typeId": type_id, "source": "E2E",
                                          "data": data}))

    def collect(self, idle: float = 0.6, hard: float = 4.0) -> list[dict]:
        """Reads engine events until the stream goes idle or *hard* elapses.

        Returns a list of ``{"typeId": str, "data": dict}`` entries.
        """
        assert self._events is not None
        events: list[dict] = []
        deadline = time.time() + hard
        self._events.settimeout(idle)
        while time.time() < deadline:
            try:
                raw = recv_frame(self._events)
            except (socket.timeout, TimeoutError):
                break
            except (ConnectionError, OSError):
                break
            try:
                msg = json.loads(raw)
            except (json.JSONDecodeError, ValueError):
                continue
            headers = msg.get("headers", {})
            data = {}
            if isinstance(headers, dict) and "data" in headers:
                try:
                    data = json.loads(headers["data"])
                except (json.JSONDecodeError, TypeError, ValueError):
                    data = {}
            events.append({"typeId": msg.get("typeId", ""), "data": data})
        return events

    def play(self, type_id: str, data: dict) -> list[dict]:
        """Sends a command and returns the events it produced."""
        self.send(type_id, data)
        return self.collect()


# ── Scenario assertions ───────────────────────────────────────────────────────

def _find(events: list[dict], type_id: str) -> dict | None:
    for event in events:
        if event["typeId"] == type_id:
            return event
    return None


def _new_game(harness: EngineHarness, mode: str = "fixed_x") -> list[dict]:
    return harness.play("gmTris.new_game", {"starter_mode": mode})


def _move(harness: EngineHarness, player: str, row: int, col: int) -> list[dict]:
    return harness.play("gmTris.move", {"player": player, "row": row, "col": col})


class Scenarios:
    """Runs each scenario and records pass/fail results."""

    def __init__(self, harness: EngineHarness) -> None:
        self._h = harness
        self.passed = 0
        self.failed = 0

    def _check(self, name: str, condition: bool, reason: str = "") -> None:
        if condition:
            print(f"  [PASS] {name}")
            self.passed += 1
        else:
            print(f"  [FAIL] {name} - {reason or 'assertion failed'}")
            self.failed += 1

    def run_all(self) -> None:
        self.session_start()
        self.invalid_out_of_range()
        self.invalid_wrong_turn()
        self.invalid_occupied()
        self.win_and_game_over()
        self.move_after_game_over()
        self.restart()
        self.draw()
        self.dice_starter()

    # ── Individual scenarios ──────────────────────────────────────────────────

    def session_start(self) -> None:
        events = _new_game(self._h, "fixed_x")
        started = _find(events, "gmFlow.session.started")
        status = _find(events, "gmActor.actor.status_added")
        self._check(
            "session_start",
            started is not None and status is not None and
            status["data"].get("status") == "ACTIVE_TURN" and
            status["data"].get("actor_id") == "Player_X",
            "expected session.started + X ACTIVE_TURN",
        )

    def invalid_out_of_range(self) -> None:
        _new_game(self._h)
        events = _move(self._h, "X", 5, 5)
        invalid = _find(events, "gmTris.invalid_move")
        self._check(
            "invalid_out_of_range",
            invalid is not None and invalid["data"].get("reason") == "out_of_range",
            f"got {invalid}",
        )

    def invalid_wrong_turn(self) -> None:
        _new_game(self._h)
        events = _move(self._h, "O", 1, 1)  # X starts; O is not active
        invalid = _find(events, "gmTris.invalid_move")
        self._check(
            "invalid_wrong_turn",
            invalid is not None and invalid["data"].get("reason") == "not_your_turn",
            f"got {invalid}",
        )

    def invalid_occupied(self) -> None:
        _new_game(self._h)
        _move(self._h, "X", 1, 1)            # X plays, turn -> O
        events = _move(self._h, "O", 1, 1)   # O tries the occupied cell
        invalid = _find(events, "gmTris.invalid_move")
        self._check(
            "invalid_occupied",
            invalid is not None and invalid["data"].get("reason") == "cell_occupied",
            f"got {invalid}",
        )

    def win_and_game_over(self) -> None:
        _new_game(self._h)
        _move(self._h, "X", 1, 1)
        _move(self._h, "O", 2, 1)
        _move(self._h, "X", 1, 2)
        _move(self._h, "O", 2, 2)
        events = _move(self._h, "X", 1, 3)   # completes row_1 for X
        won = _find(events, "gmRules.game_won")
        phase = _find(events, "gmFlow.session.phase_changed")
        self._check(
            "win_and_game_over",
            won is not None and won["data"].get("player") == "X" and
            won["data"].get("line") == "row_1" and
            phase is not None and phase["data"].get("phase") == "GAME_OVER",
            f"won={won} phase={phase}",
        )

    def move_after_game_over(self) -> None:
        # Continues from the won game above (still in GAME_OVER).
        events = _move(self._h, "O", 3, 3)
        invalid = _find(events, "gmTris.invalid_move")
        self._check(
            "move_after_game_over",
            invalid is not None and invalid["data"].get("reason") == "game_over",
            f"got {invalid}",
        )

    def restart(self) -> None:
        events = _new_game(self._h, "fixed_x")
        started = _find(events, "gmFlow.session.started")
        self._check(
            "restart_after_game_over",
            started is not None,
            "expected a fresh session.started",
        )

    def draw(self) -> None:
        _new_game(self._h)
        sequence = [
            ("X", 1, 1), ("O", 1, 2), ("X", 1, 3),
            ("O", 2, 2), ("X", 2, 1), ("O", 2, 3),
            ("X", 3, 3), ("O", 3, 1),
        ]
        for player, row, col in sequence:
            _move(self._h, player, row, col)
        events = _move(self._h, "X", 3, 2)   # last cell, no winner -> draw
        drawn = _find(events, "gmRules.game_draw")
        phase = _find(events, "gmFlow.session.phase_changed")
        self._check(
            "draw_full_board",
            drawn is not None and phase is not None and
            phase["data"].get("phase") == "GAME_OVER",
            f"draw={drawn} phase={phase}",
        )

    def dice_starter(self) -> None:
        events = _new_game(self._h, "dice_1d2")
        rolled = _find(events, "gmAlea.dice_rolled")
        ok = (rolled is not None and rolled["data"].get("value") in (1, 2) and
              rolled["data"].get("first") in ("X", "O"))
        self._check("dice_starter_roll", ok, f"got {rolled}")


def main() -> int:
    engine_path = _locate_engine()
    print(f"E2E test — engine: {engine_path}")
    harness = EngineHarness(engine_path)
    try:
        harness.start()
        scenarios = Scenarios(harness)
        scenarios.run_all()
    finally:
        harness.stop()

    print()
    if scenarios.failed:
        print(f"RESULT: FAIL ({scenarios.failed} scenario/s failed)")
        return 1
    print(f"RESULT: PASS ({scenarios.passed} scenarios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
