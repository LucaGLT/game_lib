"""sessions router — Phase 1 REST + WebSocket endpoints (single session, no auth).

Endpoint summary:

- ``GET  /missions``               — lists available missions (server-side
  scan of ``data_dir``, replaces the desktop ``MissionSelectDialog``).
- ``POST /sessions``                — boots the engine and starts a mission.
- ``GET  /sessions/{id}``           — returns the session status.
- ``POST /sessions/{id}/command``   — forwards any ``eldhom.*`` command
  envelope 1:1 (typeId-agnostic pass-through — no domain logic here).
- ``WS   /sessions/{id}/ws``        — streams engine envelopes 1:1 to the browser.
"""
from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request, WebSocket, WebSocketDisconnect
from pydantic import BaseModel

from ..missions import scan_missions
from ..session_manager import SessionAlreadyRunningError, SessionNotFoundError

router = APIRouter(tags=["sessions"])


class MissionInfo(BaseModel):
    """One entry of ``GET /missions``."""

    mission_id: str
    title: str
    description: str = ""


class CreateSessionRequest(BaseModel):
    """Body of ``POST /sessions``. Falls back to ``settings.default_mission_id``."""

    mission_id: str | None = None


class CommandRequest(BaseModel):
    """Body of ``POST /sessions/{id}/command``."""

    type_id: str
    data: dict = {}


class SessionInfo(BaseModel):
    """Response shape shared by the session creation/status endpoints."""

    session_id: str
    status: str


@router.get("/missions", response_model=list[MissionInfo])
def list_missions(request: Request) -> list[dict]:
    """Scans the data directory for available missions."""
    settings = request.app.state.settings
    return scan_missions(settings.data_dir)


@router.post("/sessions", response_model=SessionInfo, status_code=201)
def create_session(payload: CreateSessionRequest, request: Request) -> SessionInfo:
    """Boots the engine subprocess and starts a new mission.

    Defined as a plain (sync) function so FastAPI runs it in a worker thread:
    starting the engine subprocess involves blocking socket polling that must
    not stall the asyncio event loop.
    """
    manager = request.app.state.session_manager
    settings = request.app.state.settings
    mission_id = payload.mission_id or settings.default_mission_id
    try:
        session = manager.create_session(mission_id)
    except SessionAlreadyRunningError as exc:
        raise HTTPException(
            status_code=409, detail=f"Session '{exc}' is already running"
        ) from exc
    return SessionInfo(session_id=session.session_id, status="running")


@router.get("/sessions/{session_id}", response_model=SessionInfo)
def get_session(session_id: str, request: Request) -> SessionInfo:
    """Returns the status of the active session, or 404 if none matches."""
    manager = request.app.state.session_manager
    try:
        manager.get_session(session_id)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return SessionInfo(session_id=session_id, status="running")


@router.post("/sessions/{session_id}/command")
def send_command(session_id: str, payload: CommandRequest, request: Request) -> dict:
    """Forwards one ``eldhom.*`` command envelope to the engine, unchanged."""
    manager = request.app.state.session_manager
    try:
        manager.send_command(session_id, payload.type_id, payload.data)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return {"status": "sent"}


@router.websocket("/sessions/{session_id}/ws")
async def session_events_ws(websocket: WebSocket, session_id: str) -> None:
    """Streams engine envelopes 1:1 to the browser (same typeId/payload contract).

    On connect, replays the last known envelope for each typeId so a browser
    tab that joins after ``start_mission`` still sees the current state
    immediately.
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
