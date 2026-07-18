"""sessions router — Phase 1 REST + WebSocket endpoints (single session, no auth).

Endpoint summary:

- ``POST /sessions``               — boots the engine and starts a match.
- ``GET  /sessions/{id}``          — returns the session status.
- ``POST /sessions/{id}/command``  — forwards any command envelope (e.g.
  ``gmTris.move``, or ``gmTris.new_game`` again to restart the match without
  recreating the whole session/process).
- ``WS   /sessions/{id}/ws``       — streams engine envelopes 1:1 to the browser.
"""
from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request, WebSocket, WebSocketDisconnect
from pydantic import BaseModel

from ..session_manager import SessionAlreadyRunningError, SessionNotFoundError

router = APIRouter(prefix="/sessions", tags=["sessions"])


class CreateSessionRequest(BaseModel):
    """Body of ``POST /sessions``."""

    starter_mode: str = "fixed_x"


class CommandRequest(BaseModel):
    """Body of ``POST /sessions/{id}/command``."""

    type_id: str
    data: dict = {}


class SessionInfo(BaseModel):
    """Response shape shared by the session creation/status endpoints."""

    session_id: str
    status: str


@router.post("", response_model=SessionInfo, status_code=201)
def create_session(payload: CreateSessionRequest, request: Request) -> SessionInfo:
    """Boots the engine subprocess and starts a new match.

    Defined as a plain (sync) function so FastAPI runs it in a worker thread:
    starting the engine subprocess involves blocking socket polling that must
    not stall the asyncio event loop.
    """
    manager = request.app.state.session_manager
    try:
        session = manager.create_session(payload.starter_mode)
    except SessionAlreadyRunningError as exc:
        raise HTTPException(
            status_code=409, detail=f"Session '{exc}' is already running"
        ) from exc
    return SessionInfo(session_id=session.session_id, status="running")


@router.get("/{session_id}", response_model=SessionInfo)
def get_session(session_id: str, request: Request) -> SessionInfo:
    """Returns the status of the active session, or 404 if none matches."""
    manager = request.app.state.session_manager
    try:
        manager.get_session(session_id)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return SessionInfo(session_id=session_id, status="running")


@router.post("/{session_id}/command")
def send_command(session_id: str, payload: CommandRequest, request: Request) -> dict:
    """Forwards one command envelope (``gmTris.move`` / ``gmTris.new_game``) to the engine."""
    manager = request.app.state.session_manager
    try:
        manager.send_command(session_id, payload.type_id, payload.data)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return {"status": "sent"}


@router.websocket("/{session_id}/ws")
async def session_events_ws(websocket: WebSocket, session_id: str) -> None:
    """Streams engine envelopes 1:1 to the browser (same typeId/payload contract).

    On connect, replays the last known envelope for each typeId so a browser
    tab that joins after ``new_game`` still sees the current state immediately.
    """
    manager = websocket.app.state.session_manager
    try:
        manager.get_session(session_id)
    except SessionNotFoundError:
        await websocket.close(code=4404)
        return

    await websocket.accept()
    queue = manager.subscribe(session_id)
    try:
        while True:
            envelope = await queue.get()
            await websocket.send_json(envelope)
    except WebSocketDisconnect:
        pass
    finally:
        manager.unsubscribe(session_id, queue)
