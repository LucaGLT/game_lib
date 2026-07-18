"""Phase 1 smoke test: drives eng_serve end-to-end against the REAL tris_engine executable.

Mirrors the scenario described in GAME/Tic-Tac-Toe/WebApp/PLAN.md, Phase 1::

    new_game -> gmMap.snapshot -> one move -> gmMap.cell_changed

observed through eng_serve's REST + WebSocket API instead of talking to the
engine directly (compare with GAME/Tic-Tac-Toe/GUI/tests/e2e_test.py, which
exercises the same engine executable over a hand-rolled socket harness).

Skips automatically if the engine executable has not been built yet.
"""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient

from eng_serve.main import app
from eng_serve.settings import Settings

_SETTINGS = Settings()

pytestmark = [
    pytest.mark.skipif(
        not _SETTINGS.engine_executable.exists(),
        reason=(
            f"tris_engine executable not found at {_SETTINGS.engine_executable}; "
            "build it with: cmake --build build --target tris_engine --config Debug"
        ),
    ),
    pytest.mark.timeout(30),
]


@pytest.fixture()
def client():
    """A TestClient whose lifespan owns a fresh SessionManager per test."""
    with TestClient(app) as test_client:
        yield test_client
    # SessionManager.shutdown() runs automatically via the app's lifespan
    # context manager when the `with` block above exits.


def test_health(client: TestClient) -> None:
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_single_session_end_to_end(client: TestClient) -> None:
    """The full Phase 1 smoke test scenario from PLAN.md."""
    create_response = client.post("/sessions", json={"starter_mode": "fixed_x"})
    assert create_response.status_code == 201
    session_id = create_response.json()["session_id"]
    assert session_id == "dev-session"

    with client.websocket_connect(f"/sessions/{session_id}/ws") as websocket:
        snapshot = _collect_until(websocket, "gmMap.snapshot")
        assert snapshot is not None, "expected a gmMap.snapshot event after new_game"
        assert "cells" in snapshot["data"]

        move_response = client.post(
            f"/sessions/{session_id}/command",
            json={
                "type_id": "gmTris.move",
                "data": {"player": "X", "row": 1, "col": 1},
            },
        )
        assert move_response.status_code == 200

        cell_changed = _collect_until(websocket, "gmMap.cell_changed")
        assert cell_changed is not None, "expected a gmMap.cell_changed event after the move"
        assert cell_changed["data"].get("row") == 1
        assert cell_changed["data"].get("col") == 1
        assert cell_changed["data"].get("mark") == "X"


def _collect_until(websocket, type_id: str, attempts: int = 30) -> dict | None:
    """Reads envelopes from *websocket* until one matches *type_id* or attempts run out."""
    for _ in range(attempts):
        envelope = websocket.receive_json()
        if envelope.get("typeId") == type_id:
            return envelope
    return None
