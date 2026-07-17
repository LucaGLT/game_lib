"""Phase 1 smoke test: drives eng_serve end-to-end against the REAL eldhom_engine executable.

Mirrors the scenario described in GAME/Eldhom/WebApp/PLAN.md, Phase 1::

    eldhom.start_mission -> eldhom.state.full -> one simple_action -> eldhom.action.result

observed through eng_serve's REST + WebSocket API instead of talking to the
engine directly (compare with GAME/Eldhom/CoreEngine/tests/test_eldhom_mission01.cpp,
which exercises the engine's C++ API in-process).

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
            f"eldhom_engine executable not found at {_SETTINGS.engine_executable}; "
            "build it with: cmake --build build --target eldhom_engine --config Debug"
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


def test_list_missions(client: TestClient) -> None:
    """GET /missions must find at least the missione_sim_a fixture used below."""
    response = client.get("/missions")
    assert response.status_code == 200
    missions = response.json()
    assert any(m["mission_id"] == "missione_sim_a" for m in missions)


def test_single_session_end_to_end(client: TestClient) -> None:
    """The full Phase 1 smoke test scenario from PLAN.md."""
    create_response = client.post("/sessions", json={"mission_id": "missione_sim_a"})
    assert create_response.status_code == 201
    session_id = create_response.json()["session_id"]
    assert session_id == "dev-session"

    with client.websocket_connect(f"/sessions/{session_id}/ws") as websocket:
        state_full = _collect_until(websocket, "eldhom.state.full")
        assert state_full is not None, "expected an eldhom.state.full event after start_mission"

        # Thael starts at "IN" (missione_sim_a.json), adjacent only to "S1" —
        # a single Movimento Semplice there is always a legal first action.
        action_response = client.post(
            f"/sessions/{session_id}/command",
            json={
                "type_id": "eldhom.simple_action",
                "data": {"hero_id": "thael", "action_type": "MOVE", "destination": "S1"},
            },
        )
        assert action_response.status_code == 200

        result = _collect_until(websocket, "eldhom.action.result")
        assert result is not None, "expected an eldhom.action.result event after the action"
        assert result["data"].get("ok") is True, result["data"]


def _collect_until(websocket, type_id: str, attempts: int = 30) -> dict | None:
    """Reads envelopes from *websocket* until one matches *type_id* or attempts run out."""
    for _ in range(attempts):
        envelope = websocket.receive_json()
        if envelope.get("typeId") == type_id:
            return envelope
    return None
