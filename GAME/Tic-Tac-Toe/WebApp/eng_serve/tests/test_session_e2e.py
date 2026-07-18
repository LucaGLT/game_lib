"""Phase 2 smoke test: drives eng_serve end-to-end against the REAL tris_engine executable.

Mirrors the Phase 1 scenario (``new_game -> gmMap.snapshot -> one move ->
gmMap.cell_changed``) but now through the Phase 2 multi-session + auth API:
login -> create session -> WS with ``?token=`` -> command -> event. Also
covers the Phase 2 checklist items: auth required, per-user isolation, and
the per-user concurrent-session cap.

Credentials below must match
``GAME/Tic-Tac-Toe/WebApp/eng_serve/auth_config.json`` (created with
``python -m gmWebServe.tools.manage_users``, see PLAN.md Phase 2).
"""
from __future__ import annotations

import pytest
from fastapi.testclient import TestClient

from eng_serve.main import app
from eng_serve.settings import Settings

_SETTINGS = Settings()
_DEMO_USERNAME = "demo"
_DEMO_PASSWORD = "TrisDemo#2026"
_DEMO2_USERNAME = "demo2"
_DEMO2_PASSWORD = "TrisDemo#2026-b"

pytestmark = [
    pytest.mark.skipif(
        not _SETTINGS.engine_executable.exists(),
        reason=(
            f"tris_engine executable not found at {_SETTINGS.engine_executable}; "
            "build it with: cmake --build build --target tris_engine --config Debug"
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


def test_create_session_requires_auth(client: TestClient) -> None:
    response = client.post("/sessions", json={"starter_mode": "fixed_x"})
    assert response.status_code == 401


def test_single_session_end_to_end(client: TestClient) -> None:
    """The full Phase 2 smoke test scenario from PLAN.md."""
    token = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)

    create_response = client.post(
        "/sessions", json={"starter_mode": "fixed_x"}, headers=_auth_headers(token)
    )
    assert create_response.status_code == 201
    session_id = create_response.json()["session_id"]
    assert session_id  # random per-session id, no longer the fixed "dev-session"

    with client.websocket_connect(f"/sessions/{session_id}/ws?token={token}") as websocket:
        snapshot = _collect_until(websocket, "gmMap.snapshot")
        assert snapshot is not None, "expected a gmMap.snapshot event after new_game"
        assert "cells" in snapshot["data"]

        move_response = client.post(
            f"/sessions/{session_id}/command",
            json={"type_id": "gmTris.move", "data": {"player": "X", "row": 1, "col": 1}},
            headers=_auth_headers(token),
        )
        assert move_response.status_code == 200

        cell_changed = _collect_until(websocket, "gmMap.cell_changed")
        assert cell_changed is not None, "expected a gmMap.cell_changed event after the move"
        assert cell_changed["data"].get("row") == 1
        assert cell_changed["data"].get("col") == 1
        assert cell_changed["data"].get("mark") == "X"

    close_response = client.delete(f"/sessions/{session_id}", headers=_auth_headers(token))
    assert close_response.status_code == 204


def test_two_users_concurrent_sessions_isolated(client: TestClient) -> None:
    """Two different users each get their own isolated session (Phase 2 requirement)."""
    token_a = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    token_b = _login(client, _DEMO2_USERNAME, _DEMO2_PASSWORD)

    session_a = client.post(
        "/sessions", json={"starter_mode": "fixed_x"}, headers=_auth_headers(token_a)
    ).json()["session_id"]
    session_b = client.post(
        "/sessions", json={"starter_mode": "fixed_x"}, headers=_auth_headers(token_b)
    ).json()["session_id"]
    assert session_a != session_b

    listed_a = client.get("/sessions", headers=_auth_headers(token_a)).json()
    listed_b = client.get("/sessions", headers=_auth_headers(token_b)).json()
    assert [s["session_id"] for s in listed_a] == [session_a]
    assert [s["session_id"] for s in listed_b] == [session_b]

    # User B must not be able to see or command user A's session (no leak of
    # "exists but forbidden" vs "does not exist" — both are a plain 404).
    forbidden_get = client.get(f"/sessions/{session_a}", headers=_auth_headers(token_b))
    assert forbidden_get.status_code == 404
    forbidden_command = client.post(
        f"/sessions/{session_a}/command",
        json={"type_id": "gmTris.new_game", "data": {}},
        headers=_auth_headers(token_b),
    )
    assert forbidden_command.status_code == 404


def test_session_limit_returns_429(client: TestClient) -> None:
    """Exceeding max_sessions_per_user (2, per auth_config.json) is rejected."""
    token = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    headers = _auth_headers(token)

    first = client.post("/sessions", json={"starter_mode": "fixed_x"}, headers=headers)
    second = client.post("/sessions", json={"starter_mode": "fixed_x"}, headers=headers)
    assert first.status_code == 201
    assert second.status_code == 201

    third = client.post("/sessions", json={"starter_mode": "fixed_x"}, headers=headers)
    assert third.status_code == 429


def test_join_session_invalid_code_returns_404(client: TestClient) -> None:
    token = _login(client, _DEMO2_USERNAME, _DEMO2_PASSWORD)
    response = client.post(
        "/sessions/join", json={"join_code": "ZZZZZZ"}, headers=_auth_headers(token)
    )
    assert response.status_code == 404


def test_two_users_join_same_match_and_play(client: TestClient) -> None:
    """Shared Multiplayer: a SECOND (different) user joins the SAME match via a
    join code and pilots the other seat — not an isolated match of their own.

    Also proves the server-side anti-spoofing guard (`SessionManager.send_command`):
    even if a participant's client sends a forged `player` field for `gmTris.move`,
    the engine only ever sees the mark tied to THAT caller's real seat.
    """
    token_x = _login(client, _DEMO_USERNAME, _DEMO_PASSWORD)
    token_o = _login(client, _DEMO2_USERNAME, _DEMO2_PASSWORD)

    create_response = client.post(
        "/sessions", json={"starter_mode": "fixed_x"}, headers=_auth_headers(token_x)
    )
    assert create_response.status_code == 201
    created = create_response.json()
    session_id = created["session_id"]
    join_code = created["join_code"]
    assert created["your_role"] == "X"
    assert created["roles"] == {"X": _DEMO_USERNAME, "O": None}

    join_response = client.post(
        "/sessions/join", json={"join_code": join_code}, headers=_auth_headers(token_o)
    )
    assert join_response.status_code == 200
    joined = join_response.json()
    assert joined["session_id"] == session_id
    assert joined["your_role"] == "O"
    assert joined["roles"] == {"X": _DEMO_USERNAME, "O": _DEMO2_USERNAME}

    # The joiner must now see the match in their OWN session list too.
    listed_o = client.get("/sessions", headers=_auth_headers(token_o)).json()
    assert [s["session_id"] for s in listed_o] == [session_id]

    # A SINGLE websocket (the joiner's) is enough to prove the match is truly
    # SHARED: it must see events caused by the OTHER user's REST calls, which
    # would never happen if "join" merely created an isolated second session.
    with client.websocket_connect(f"/sessions/{session_id}/ws?token={token_o}") as ws_o:
        assert _collect_until(ws_o, "gmMap.snapshot") is not None

        # "X" (demo) moves via their OWN token — a forged "player":"O" in the
        # request body is overridden server-side to demo's TRUE seat ("X").
        move_x = client.post(
            f"/sessions/{session_id}/command",
            json={"type_id": "gmTris.move", "data": {"player": "O", "row": 1, "col": 1}},
            headers=_auth_headers(token_x),
        )
        assert move_x.status_code == 200
        cell_changed_x = _collect_until(ws_o, "gmMap.cell_changed")
        assert cell_changed_x is not None
        assert cell_changed_x["data"]["mark"] == "X"  # demo's TRUE seat, not the forged "O"

        # "O" (demo2) moves — forging "player":"X" is likewise overridden.
        move_o = client.post(
            f"/sessions/{session_id}/command",
            json={"type_id": "gmTris.move", "data": {"player": "X", "row": 2, "col": 2}},
            headers=_auth_headers(token_o),
        )
        assert move_o.status_code == 200
        cell_changed_o = _collect_until(ws_o, "gmMap.cell_changed")
        assert cell_changed_o is not None
        assert cell_changed_o["data"]["mark"] == "O"  # demo2's TRUE seat, not the forged "X"


def _collect_until(websocket, type_id: str, attempts: int = 30) -> dict | None:
    """Reads envelopes from *websocket* until one matches *type_id* or attempts run out."""
    for _ in range(attempts):
        envelope = websocket.receive_json()
        if envelope.get("typeId") == type_id:
            return envelope
    return None
