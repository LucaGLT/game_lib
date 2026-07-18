"""sessions router — Phase 2 multi-session, multi-user REST + WebSocket endpoints.

Endpoint summary (every REST endpoint requires ``Authorization: Bearer <token>``
from ``POST /auth/login``, see ``gmWebServe.auth_router``):

- ``POST   /sessions``               — boots a new engine and starts a match.
- ``GET    /sessions``               — lists the caller's own active sessions.
- ``GET    /sessions/{id}``          — returns one session's status (owner-only).
- ``POST   /sessions/{id}/command``  — forwards any command envelope (e.g.
  ``gmTris.move``, or ``gmTris.new_game`` again to restart the match without
  recreating the whole session/process).
- ``DELETE /sessions/{id}``          — closes one session, freeing its slot.
- ``WS     /sessions/{id}/ws?token=``— streams engine envelopes 1:1 to the
  browser (WebSocket handshakes cannot set headers, so the token travels as
  a query parameter here instead).
"""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException, Query, Request, WebSocket, WebSocketDisconnect
from pydantic import BaseModel

from gmWebServe.fastapi_deps import authenticate_ws, get_current_user

from ..session_manager import SessionLimitExceededError, SessionNotFoundError

router = APIRouter(prefix="/sessions", tags=["sessions"])


class CreateSessionRequest(BaseModel):
    """Body of ``POST /sessions``."""

    starter_mode: str = "fixed_x"


class CommandRequest(BaseModel):
    """Body of ``POST /sessions/{id}/command``."""

    type_id: str
    data: dict = {}


class SessionInfo(BaseModel):
    """Response shape shared by the session creation/status/list endpoints."""

    session_id: str
    status: str


@router.post("", response_model=SessionInfo, status_code=201)
def create_session(
    payload: CreateSessionRequest, request: Request, owner: str = Depends(get_current_user)
) -> SessionInfo:
    """Boots the engine subprocess and starts a new match for the caller.

    Defined as a plain (sync) function so FastAPI runs it in a worker thread:
    starting the engine subprocess involves blocking socket polling that must
    not stall the asyncio event loop.
    """
    manager = request.app.state.session_manager
    try:
        session = manager.create_session(owner, payload.starter_mode)
    except SessionLimitExceededError as exc:
        raise HTTPException(status_code=429, detail=str(exc)) from exc
    return SessionInfo(session_id=session.session_id, status="running")


@router.get("", response_model=list[SessionInfo])
def list_sessions(request: Request, owner: str = Depends(get_current_user)) -> list[SessionInfo]:
    """Lists every active session owned by the caller."""
    manager = request.app.state.session_manager
    return [
        SessionInfo(session_id=session.session_id, status="running")
        for session in manager.list_sessions(owner)
    ]


@router.get("/{session_id}", response_model=SessionInfo)
def get_session(
    session_id: str, request: Request, owner: str = Depends(get_current_user)
) -> SessionInfo:
    """Returns the status of one of the caller's own sessions, or 404."""
    manager = request.app.state.session_manager
    try:
        manager.get_session(session_id, owner)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return SessionInfo(session_id=session_id, status="running")


@router.post("/{session_id}/command")
def send_command(
    session_id: str,
    payload: CommandRequest,
    request: Request,
    owner: str = Depends(get_current_user),
) -> dict:
    """Forwards one command envelope (``gmTris.move`` / ``gmTris.new_game``) to the engine."""
    manager = request.app.state.session_manager
    try:
        manager.send_command(session_id, owner, payload.type_id, payload.data)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return {"status": "sent"}


@router.delete("/{session_id}", status_code=204)
def close_session(
    session_id: str, request: Request, owner: str = Depends(get_current_user)
) -> None:
    """Closes one of the caller's own sessions, freeing its slot in the per-user cap."""
    manager = request.app.state.session_manager
    try:
        manager.close_session(session_id, owner)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc


@router.websocket("/{session_id}/ws")
async def session_events_ws(
    websocket: WebSocket, session_id: str, token: str | None = Query(default=None)
) -> None:
    """Streams engine envelopes 1:1 to the browser (same typeId/payload contract).

    On connect, replays the last known envelope for each typeId so a browser
    tab that joins after ``new_game`` still sees the current state immediately.
    """
    manager = websocket.app.state.session_manager
    owner = authenticate_ws(websocket.app.state, token)
    if owner is None:
        await websocket.close(code=4401)
        return
    try:
        manager.get_session(session_id, owner)
    except SessionNotFoundError:
        await websocket.close(code=4404)
        return

    await websocket.accept()
    queue = manager.subscribe(session_id, owner)
    try:
        while True:
            envelope = await queue.get()
            await websocket.send_json(envelope)
    except WebSocketDisconnect:
        pass
    finally:
        manager.unsubscribe(session_id, owner, queue)
