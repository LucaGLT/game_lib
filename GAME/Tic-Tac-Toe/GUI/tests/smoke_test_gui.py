"""Headless smoke test for the Tris hybrid gmGui GUI.

Runs fully offscreen (``QT_QPA_PLATFORM=offscreen``) without a live C++ engine:
it instantiates :class:`TrisMainWindow`, feeds a scripted sequence of native
Tris envelopes through the real routing path (board module + adapter +
dashboards) and asserts that:

- the clickable board renders cells and emits ``gmTris.move`` on click,
- the generic ``GmActorModule`` / ``GmFlowModule`` dashboards receive the
  adapter-translated envelopes and update their state without errors,
- a win + ``GAME_OVER`` phase disables the board.

Run::

    python tests/smoke_test_gui.py
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

# ── sys.path bootstrap (mirror of main.py) ────────────────────────────────────
_GUI_DIR = Path(__file__).resolve().parents[1]
_PYLIB_DIR = _GUI_DIR.parents[2] / "pyLib"
for _path in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from PySide6.QtWidgets import QApplication  # noqa: E402

from app.tris_main_window import TrisMainWindow  # noqa: E402


def _env(type_id: str, data: dict) -> dict:
    """Builds a native Tris envelope as emitted by the C++ engine."""
    return {"typeId": type_id, "source": "gmTris.engine", "data": data}


def _check(label: str, condition: bool, failures: list[str]) -> None:
    status = "PASS" if condition else "FAIL"
    print(f"  [{status}] {label}")
    if not condition:
        failures.append(label)


def run() -> int:
    app = QApplication([])
    window = TrisMainWindow()

    # Capture outgoing commands by stubbing the shared sender.
    sent: list[tuple[str, dict]] = []
    window._bridge.sender.send_command = (  # type: ignore[assignment]
        lambda tid, data: sent.append((tid, data))
    )

    board = window._board_module
    actor_mod = window._dashboards[1]  # GmActorModule
    flow_mod = window._dashboards[0]   # GmFlowModule

    failures: list[str] = []
    print("Smoke test — Tris hybrid GUI (offscreen)")

    # 1. New match begins: session + snapshots + X to move.
    window._on_envelope(_env("gmFlow.session.started", {"phase": "PLACING"}))
    window._on_envelope(_env("gmMap.snapshot", {"cells": []}))
    window._on_envelope(
        _env(
            "gmActor.snapshot",
            {
                "actors": [
                    {"actor_id": "Player_X", "statuses": ["ACTIVE_TURN"]},
                    {"actor_id": "Player_O", "statuses": []},
                ]
            },
        )
    )

    _check("Board active mark is X", board._active_mark == "X", failures)
    _check(
        "GmActorModule received both actors",
        set(actor_mod._actor_data.keys()) == {"Player_X", "Player_O"},
        failures,
    )
    _check(
        "GmActorModule registered X ACTIVE_TURN",
        actor_mod._actor_data.get("Player_X", {}).get("statuses", {}).get(
            "ACTIVE_TURN"
        )
        == 1,
        failures,
    )
    _check(
        "GmFlowModule phase label updated",
        "PLACING" in flow_mod._lbl_phase.text(),
        failures,
    )

    # 2. User clicks cell (1, 1) — board must emit a gmTris.move for X.
    board._board.cell_clicked.emit(1, 1)
    _check(
        "Click emitted gmTris.move for X at (1,1)",
        ("gmTris.move", {"player": "X", "row": 1, "col": 1}) in sent,
        failures,
    )

    # 3. Engine confirms the move and switches the turn to O.
    window._on_envelope(_env("gmMap.cell_changed", {"row": 1, "col": 1, "mark": "X"}))
    window._on_envelope(
        _env("gmActor.actor.status_removed", {"actor_id": "Player_X", "status": "ACTIVE_TURN"})
    )
    window._on_envelope(
        _env("gmActor.actor.status_added", {"actor_id": "Player_O", "status": "ACTIVE_TURN"})
    )

    _check(
        "Board cell (1,1) shows X",
        board._board._buttons[(1, 1)].text() == "X",
        failures,
    )
    _check("Board active mark switched to O", board._active_mark == "O", failures)
    _check(
        "GmActorModule moved ACTIVE_TURN to O",
        actor_mod._actor_data["Player_O"]["statuses"].get("ACTIVE_TURN") == 1
        and "ACTIVE_TURN" not in actor_mod._actor_data["Player_X"]["statuses"],
        failures,
    )

    # 4. Dice roll (only meaningful in dice_1d2 mode) reaches GmDiceModule.
    window._on_envelope(_env("gmAlea.dice_rolled", {"value": 2, "first": "O"}))
    dice_mod = window._dashboards[2]
    _check(
        "GmDiceModule rendered the roll total",
        dice_mod._result_label.text() == "2",
        failures,
    )

    # 5. X wins on the top row, then the engine ends the session.
    window._on_envelope(_env("gmRules.game_won", {"player": "X", "line": "row_1"}))
    window._on_envelope(_env("gmFlow.session.phase_changed", {"phase": "GAME_OVER"}))

    _check("Board marked game over", board._game_over is True, failures)
    _check(
        "Board disabled after game over",
        not board._board._buttons[(1, 1)].isEnabled(),
        failures,
    )

    window.close()
    app.quit()

    print()
    if failures:
        print(f"RESULT: FAIL ({len(failures)} check/s failed)")
        return 1
    print("RESULT: PASS (all checks succeeded)")
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
