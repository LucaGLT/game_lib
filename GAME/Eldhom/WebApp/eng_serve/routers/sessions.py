"""sessions router — Shared Multiplayer REST + WebSocket endpoints.

Endpoint summary (every REST endpoint requires ``Authorization: Bearer <token>``
from ``POST /auth/login``, see ``gmWebServe.auth_router``):

- ``GET    /missions``               — lists available missions, including
  each one's ``pg_roster`` (server-side scan of ``data_dir``, replaces the
  desktop ``MissionSelectDialog``) — used to render the "pick your PG/hero"
  screen before creating or joining a session.
- ``GET    /cards``                  — lists the card catalog (pass-through).
- ``POST   /sessions``               — boots the engine, starts *mission_id*,
  and seats the caller on their CHOSEN ``hero_id``. The response's
  ``join_code`` is meant to be shared (out-of-band, e.g. verbally) with a
  second, DIFFERENT user.
- ``GET    /sessions/by-code/{code}``— PREVIEWS a session by its join code,
  WITHOUT joining it — lets a would-be joiner see which PGs/heroes are still
  free before picking one.
- ``POST   /sessions/join``          — a second user joins the SAME mission
  on their CHOSEN remaining ``hero_id``, using the creator's ``join_code``.
- ``GET    /sessions``               — lists the caller's own active sessions
  (created OR joined).
- ``GET    /sessions/{id}``          — returns one session's status (any
  participant).
- ``POST   /sessions/{id}/command``  — forwards any ``eldhom.*`` command
  envelope 1:1 (typeId-agnostic pass-through), except that any command tied
  to one specific hero (see ``session_manager._HERO_OWNED_COMMAND_FIELDS``)
  has that hero-id field re-derived server-side from the caller's own seat.
- ``DELETE /sessions/{id}``          — closes one session, freeing its slot
  (any participant may do this — it ends the mission for the whole party).
- ``WS     /sessions/{id}/ws?token=``— streams engine envelopes 1:1 to every
  participant's browser (WebSocket handshakes cannot set headers, so the
  token travels as a query parameter here instead).
"""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException, Query, Request, WebSocket, WebSocketDisconnect
from pydantic import BaseModel

from gmWebServe.fastapi_deps import authenticate_ws, get_current_user
from gmWebServe.session_registry import GameSession

from ..cards import scan_cards
from ..missions import hero_ids_for_mission, scan_missions
from ..session_manager import SessionFullError, SessionLimitExceededError, SessionNotFoundError

router = APIRouter(tags=["sessions"])


class PgRosterEntry(BaseModel):
    """One playable PG/hero of a mission's roster (``GET /missions``)."""

    hero_id: str
    display_name: str
    class_name: str = ""


class MissionInfo(BaseModel):
    """One entry of ``GET /missions``."""

    mission_id: str
    title: str
    description: str = ""
    pg_roster: list[PgRosterEntry] = []


class CreateSessionRequest(BaseModel):
    """Body of ``POST /sessions``."""

    mission_id: str
    hero_id: str


class JoinSessionRequest(BaseModel):
    """Body of ``POST /sessions/join``."""

    join_code: str
    hero_id: str


class CommandRequest(BaseModel):
    """Body of ``POST /sessions/{id}/command``."""

    type_id: str
    data: dict = {}


class SessionInfo(BaseModel):
    """Response shape shared by the session creation/status/list/join endpoints."""

    session_id: str
    status: str
    join_code: str
    mission_id: str | None
    roles: dict[str, str | None]
    your_role: str | None


class SessionPreview(BaseModel):
    """Response of ``GET /sessions/by-code/{code}`` — a peek before joining."""

    mission_id: str | None
    roles: dict[str, str | None]


def _to_session_info(
    manager, session: GameSession, requester: str
) -> SessionInfo:
    """Builds the caller-facing view of *session* (their own seat + all seats)."""
    return SessionInfo(
        session_id=session.session_id,
        status="running",
        join_code=session.join_code,
        mission_id=manager.mission_id_of(session.session_id),
        roles=dict(session.participants),
        your_role=session.role_of(requester),
    )


@router.get("/missions", response_model=list[MissionInfo])
def list_missions(request: Request, _user: str = Depends(get_current_user)) -> list[dict]:
    """Scans the data directory for available missions (incl. each ``pg_roster``)."""
    settings = request.app.state.settings
    return scan_missions(settings.data_dir)


@router.get("/cards")
def list_cards(request: Request, _user: str = Depends(get_current_user)) -> list[dict]:
    """Scans the data directory for the card catalog.

    No ``response_model``: card shape varies a lot by effect type and
    eng_serve deliberately does not interpret/reshape payloads (pure
    pass-through, see GAME/Eldhom/WebApp/PLAN.md, Key Design Decision 6).
    """
    settings = request.app.state.settings
    return scan_cards(settings.data_dir)


@router.post("/sessions", response_model=SessionInfo, status_code=201)
def create_session(
    payload: CreateSessionRequest, request: Request, owner: str = Depends(get_current_user)
) -> SessionInfo:
    """Boots the engine subprocess, starts *mission_id*, and seats the caller.

    Defined as a plain (sync) function so FastAPI runs it in a worker thread:
    starting the engine subprocess involves blocking socket polling that must
    not stall the asyncio event loop.
    """
    manager = request.app.state.session_manager
    settings = request.app.state.settings
    hero_ids = hero_ids_for_mission(settings.data_dir, payload.mission_id)
    if hero_ids is None:
        raise HTTPException(status_code=404, detail="Mission not found")
    if payload.hero_id not in hero_ids:
        raise HTTPException(
            status_code=400, detail=f"hero_id must be one of {hero_ids}"
        )
    try:
        session = manager.create_session(owner, payload.mission_id, hero_ids, payload.hero_id)
    except SessionLimitExceededError as exc:
        raise HTTPException(status_code=429, detail=str(exc)) from exc
    return _to_session_info(manager, session, owner)


@router.get("/sessions/by-code/{join_code}", response_model=SessionPreview)
def preview_session_by_code(
    join_code: str, request: Request, _user: str = Depends(get_current_user)
) -> SessionPreview:
    """Previews a session's roles WITHOUT joining it (pick-your-PG screen)."""
    manager = request.app.state.session_manager
    try:
        session, mission_id = manager.peek_session_by_code(join_code)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Invalid join code") from exc
    return SessionPreview(mission_id=mission_id, roles=dict(session.participants))


@router.post("/sessions/join", response_model=SessionInfo, status_code=200)
def join_session(
    payload: JoinSessionRequest, request: Request, joining_user: str = Depends(get_current_user)
) -> SessionInfo:
    """Attaches the caller to the SAME mission as the session's creator, on their CHOSEN hero."""
    manager = request.app.state.session_manager
    try:
        session = manager.join_session(payload.join_code, joining_user, payload.hero_id)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Invalid join code") from exc
    except SessionFullError as exc:
        raise HTTPException(status_code=409, detail="That PG/hero is already taken") from exc
    except SessionLimitExceededError as exc:
        raise HTTPException(status_code=429, detail=str(exc)) from exc
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    return _to_session_info(manager, session, joining_user)


@router.get("/sessions", response_model=list[SessionInfo])
def list_sessions(request: Request, owner: str = Depends(get_current_user)) -> list[SessionInfo]:
    """Lists every active session the caller participates in (created or joined)."""
    manager = request.app.state.session_manager
    return [_to_session_info(manager, session, owner) for session in manager.list_sessions(owner)]


@router.get("/sessions/{session_id}", response_model=SessionInfo)
def get_session(
    session_id: str, request: Request, owner: str = Depends(get_current_user)
) -> SessionInfo:
    """Returns the status of a session the caller participates in, or 404."""
    manager = request.app.state.session_manager
    try:
        session = manager.get_session(session_id, owner)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return _to_session_info(manager, session, owner)


@router.post("/sessions/{session_id}/command")
def send_command(
    session_id: str,
    payload: CommandRequest,
    request: Request,
    owner: str = Depends(get_current_user),
) -> dict:
    """Forwards one ``eldhom.*`` command envelope to the engine."""
    manager = request.app.state.session_manager
    try:
        manager.send_command(session_id, owner, payload.type_id, payload.data)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc
    return {"status": "sent"}


@router.delete("/sessions/{session_id}", status_code=204)
def close_session(
    session_id: str, request: Request, owner: str = Depends(get_current_user)
) -> None:
    """Closes one of the caller's own sessions, freeing its slot in the per-user cap."""
    manager = request.app.state.session_manager
    try:
        manager.close_session(session_id, owner)
    except SessionNotFoundError as exc:
        raise HTTPException(status_code=404, detail="Session not found") from exc


@router.websocket("/sessions/{session_id}/ws")
async def session_events_ws(
    websocket: WebSocket, session_id: str, token: str | None = Query(default=None)
) -> None:
    """Streams engine envelopes 1:1 to the browser (same typeId/payload contract).

    On connect, replays the last known envelope for each typeId so a browser
    tab that joins after ``start_mission`` still sees the current state
    immediately.
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

