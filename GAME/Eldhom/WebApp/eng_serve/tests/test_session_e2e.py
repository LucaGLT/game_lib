"""Shared Multiplayer smoke test: drives eng_serve end-to-end against the REAL
eldhom_engine executable.

Mirrors the scenario described in GAME/Eldhom/WebApp/PLAN.md::

    login -> create session (mission + CHOSEN hero) -> eldhom.state.full ->
    one simple_action -> eldhom.action.result

now through the multi-session + auth + shared-join-code API (mirrors
GAME/Tic-Tac-Toe/WebApp/eng_serve/tests/test_session_e2e.py), instead of the
single fixed "dev-session" of the original Phase 1 test. Compare with
GAME/Eldhom/CoreEngine/tests/test_eldhom_mission01.cpp, which exercises the
engine's C++ API in-process.

Credentials below must match
``GAME/Eldhom/WebApp/eng_serve/auth_config.json`` (created with
``python -m gmWebServe.tools.manage_users``, see PLAN.md).

Skips automatically if the engine executable has not been built yet.
"""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient

from eng_serve.main import app
from eng_serve.settings import Settings

_SETTINGS = Settings()
_DEMO_USERNAME = "demo"
_DEMO_PASSWORD = "EldhomDemo#2026"
_DEMO2_USERNAME = "demo2"
_DEMO2_PASSWORD = "EldhomDemo#2026-b"
_MISSION_ID = "missione_sim_a"

pytestmark = [
    pytest.mark.skipif(
        not _SETTINGS.engine_executable.exists(),
        reason=(
            f"eldhom_engine executable not found at {_SETTINGS.engine_executable}; "
            "build it with: cmake --build build --target eldhom_engine --config Debug"
        ),
    ),
    pytest.mark.timeout(60),
]


@pytest.fixture()
def client():
    """A TestClient whose lifespan owns a fresh SessionManager per test."""
    with TestClient(app) as test_client:
        yield test_client
    # SessionManager.shutdown() runs automatically via the app's lifespan
    # context manager when the `with` block above exits.


def _login(client: TestClient, username: str, password: str) -> str:
    """Logs in and returns the bearer token, failing the test on a bad response."""
    response = client.post("/auth/login", json={"username": username, "password": password})
    assert response.status_code == 200, response.text
    return response.json()["token"]


def _auth_headers(token: str) -> dict:
    return {"Authorization": f"Bearer {token}"}


def test_health(client: TestClient) -> None:
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_login_wrong_password_rejected(client: TestClient) -> None:
    response = client.post("/auth/login", json={"username": _DEMO_USERNAME, "password": "wrong"})
    assert response.status_code == 401


def test_list_missions_requires_auth(client: TestClient) -> None:
    response = client.get("/missions")
    assert response.status_code == 401


def test_create_session_requires_auth(client: TestClient) -> None:
    response = client.post("/sessions", json={"mission_id": _MISSION_ID, "hero_id": "thael"})
    assert response.status_code == 401


def test_list_missions_includes_roster(client: TestClient) -> None:
    """GET /missions must find missione_sim_a with its 2-hero pg_roster (thael/velyr)."""
    token = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    response = client.get("/missions", headers=_auth_headers(token))
    assert response.status_code == 200
    missions = response.json()
    mission = next((m for m in missions if m["mission_id"] == _MISSION_ID), None)
    assert mission is not None
    hero_ids = {entry["hero_id"] for entry in mission["pg_roster"]}
    assert hero_ids == {"thael", "velyr"}


def test_create_session_invalid_hero_id_returns_400(client: TestClient) -> None:
    token = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    response = client.post(
        "/sessions",
        json={"mission_id": _MISSION_ID, "hero_id": "not_a_real_hero"},
        headers=_auth_headers(token),
    )
    assert response.status_code == 400


def test_single_session_end_to_end(client: TestClient) -> None:
    """The full smoke test scenario from PLAN.md, single user controlling one PG."""
    token = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)

    create_response = client.post(
        "/sessions",
        json={"mission_id": _MISSION_ID, "hero_id": "thael"},
        headers=_auth_headers(token),
    )
    assert create_response.status_code == 201
    created = create_response.json()
    session_id = created["session_id"]
    assert created["your_role"] == "thael"
    assert created["roles"] == {"thael": _DEMO_USERNAME, "velyr": None}

    with client.websocket_connect(f"/sessions/{session_id}/ws?token={token}") as websocket:
        state_full = _collect_until(websocket, "eldhom.state.full")
        assert state_full is not None, "expected an eldhom.state.full event after start_mission"

        # Whichever hero acts first, a Movimento Semplice IN -> S1 is always a
        # legal first action (both heroes start at "IN", adjacent only to "S1").
        next_actor = state_full["data"]["next_actor"]["actor_id"]
        action_response = client.post(
            f"/sessions/{session_id}/command",
            json={
                "type_id": "eldhom.simple_action",
                "data": {"hero_id": next_actor, "action_type": "MOVE", "destination": "S1"},
            },
            headers=_auth_headers(token),
        )
        assert action_response.status_code == 200

        result = _collect_until(websocket, "eldhom.action.result")
        assert result is not None, "expected an eldhom.action.result event after the action"
        assert result["data"].get("ok") is True, result["data"]

    close_response = client.delete(f"/sessions/{session_id}", headers=_auth_headers(token))
    assert close_response.status_code == 204


def test_preview_session_by_code_invalid_returns_404(client: TestClient) -> None:
    token = _login(client, _DEMO2_USERNAME, _DEMO2_PASSWORD)
    response = client.get("/sessions/by-code/ZZZZZZ", headers=_auth_headers(token))
    assert response.status_code == 404


def test_two_users_join_same_mission_and_play(client: TestClient) -> None:
    """Shared Multiplayer: a SECOND (different) user joins the SAME mission via a
    join code, choosing which of the REMAINING PGs/heroes to play — not an
    isolated mission of their own.

    Also proves the server-side anti-spoofing guard
    (`SessionManager.send_command`): even if a participant's client sends a
    forged `hero_id` for a hero-owned command, the engine only ever sees the
    hero_id tied to THAT caller's real seat.
    """
    token_thael = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    token_velyr = _login(client, _DEMO2_USERNAME, _DEMO2_PASSWORD)

    create_response = client.post(
        "/sessions",
        json={"mission_id": _MISSION_ID, "hero_id": "thael"},
        headers=_auth_headers(token_thael),
    )
    assert create_response.status_code == 201
    created = create_response.json()
    session_id = created["session_id"]
    join_code = created["join_code"]

    # Preview by code BEFORE joining: velyr must show as the only free seat.
    preview_response = client.get(
        f"/sessions/by-code/{join_code}", headers=_auth_headers(token_velyr)
    )
    assert preview_response.status_code == 200
    preview = preview_response.json()
    assert preview["mission_id"] == _MISSION_ID
    assert preview["roles"] == {"thael": _DEMO_USERNAME, "velyr": None}

    join_response = client.post(
        "/sessions/join",
        json={"join_code": join_code, "hero_id": "velyr"},
        headers=_auth_headers(token_velyr),
    )
    assert join_response.status_code == 200
    joined = join_response.json()
    assert joined["session_id"] == session_id
    assert joined["your_role"] == "velyr"
    assert joined["roles"] == {"thael": _DEMO_USERNAME, "velyr": _DEMO2_USERNAME}

    # A SINGLE websocket (the joiner's) is enough to prove the mission is truly
    # SHARED: it must see events caused by the OTHER user's REST calls.
    token_by_hero = {"thael": token_thael, "velyr": token_velyr}
    with client.websocket_connect(f"/sessions/{session_id}/ws?token={token_velyr}") as ws:
        state_full = _collect_until(ws, "eldhom.state.full")
        assert state_full is not None
        next_actor = state_full["data"]["next_actor"]["actor_id"]
        other_hero = "velyr" if next_actor == "thael" else "thael"
        acting_token = token_by_hero[next_actor]

        # The hero whose turn it is moves via THEIR OWN token — a forged
        # hero_id (the OTHER hero) in the request body is overridden
        # server-side to the caller's TRUE seat, so the move still proceeds
        # (both heroes start at "IN", adjacent only to "S1") as the REAL actor.
        move_response = client.post(
            f"/sessions/{session_id}/command",
            json={
                "type_id": "eldhom.simple_action",
                "data": {"hero_id": other_hero, "action_type": "MOVE", "destination": "S1"},
            },
            headers=_auth_headers(acting_token),
        )
        assert move_response.status_code == 200

        pg_moved = _collect_until(ws, "eldhom.pg.moved")
        assert pg_moved is not None
        assert pg_moved["data"]["actor_id"] == next_actor  # TRUE seat, not the forged one


def _collect_until(websocket, type_id: str, attempts: int = 30) -> dict | None:
    """Reads envelopes from *websocket* until one matches *type_id* or attempts run out."""
    for _ in range(attempts):
        envelope = websocket.receive_json()
        if envelope.get("typeId") == type_id:
            return envelope
    return None

