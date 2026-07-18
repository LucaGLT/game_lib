"""session_manager — Phase 1: a single, process-wide game session (no auth).

Deliberately hardcodes ONE session so Phase 1 can validate the full engine
<-> eng_serve <-> browser round trip end-to-end. Phase 2 replaces this with
a real per-user registry (session_id -> SessionState), authentication, and
idle-session cleanup — see GAME/Tic-Tac-Toe/WebApp/PLAN.md, Phase 2.
"""
from __future__ import annotations

import asyncio
import sys
from pathlib import Path
from threading import Lock

# ── Make the shared gmWebServe toolkit importable (pyLib is the common parent
#    of gmWebServe and gmGui) — see GAME/Eldhom/WebApp/PLAN.md, Phase 1, for
#    the extraction rationale (shared 1:1 with the Eldhom eng_serve).
_PYLIB_DIR = Path(__file__).resolve().parents[4] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))

from gmWebServe import (  # noqa: E402
    EngineEventListener,
    EngineProcess,
    EngineProcessError,
    EngineSender,
)

from .settings import Settings

SESSION_ID = "dev-session"

__all__ = [
    "SessionManager",
    "SessionState",
    "SessionNotFoundError",
    "SessionAlreadyRunningError",
    "SESSION_ID",
]


class SessionNotFoundError(RuntimeError):
    """Raised when the requested session_id does not match the active session."""


class SessionAlreadyRunningError(RuntimeError):
    """Raised when create_session() is called while a session is already active."""


class SessionState:
    """Everything eng_serve knows about the one running game session (Phase 1).

    Attributes:
        session_id:            Identifier of this session (fixed in Phase 1).
        last_envelope_by_type:  Latest envelope seen for each ``typeId``, used
            to replay current state to a browser tab that connects late.
        subscribers:           Active WebSocket subscriber queues.
    """

    def __init__(self, session_id: str) -> None:
        self.session_id: str = session_id
        self.last_envelope_by_type: dict[str, dict] = {}
        self.subscribers: set[asyncio.Queue] = set()


class SessionManager:
    """Owns the single Phase-1 session: engine process, bridge, and WS fan-out.

    Thread-safety: :meth:`create_session` and :meth:`shutdown` are guarded by
    a lock because they mutate process/socket state; :meth:`_on_envelope`
    runs on the listener's background thread and hands events to the asyncio
    loop via ``call_soon_threadsafe``.
    """

    def __init__(self, settings: Settings) -> None:
        self._settings: Settings = settings
        self._lock: Lock = Lock()
        self._session: SessionState | None = None
        self._engine: EngineProcess | None = None
        self._listener: EngineEventListener | None = None
        self._command_sender: EngineSender | None = None
        self.loop: asyncio.AbstractEventLoop | None = None

    @property
    def active_session_id(self) -> str | None:
        """The id of the currently running session, or None if none is active."""
        return self._session.session_id if self._session is not None else None

    def create_session(self, starter_mode: str) -> SessionState:
        """Boots the engine subprocess and the bridge, then starts a match.

        Sending the bootstrap ``gmTris.new_game`` command is what triggers
        the engine to lazily connect back on the event port — the same
        sequence used by ``GAME/Tic-Tac-Toe/GUI/tests/e2e_test.py``.

        Raises:
            SessionAlreadyRunningError: If a session is already active.
            EngineProcessError: If the engine subprocess fails to start or
                its command port never becomes reachable.
        """
        with self._lock:
            if self._session is not None:
                raise SessionAlreadyRunningError(self._session.session_id)

            session = SessionState(SESSION_ID)
            listener = EngineEventListener(
                self._settings.event_host,
                self._settings.event_port,
                on_envelope=lambda envelope: self._on_envelope(session, envelope),
            )
            listener.bind()

            engine = EngineProcess(
                self._settings.engine_executable,
                self._settings.command_host,
                self._settings.command_port,
                self._settings.connect_timeout_s,
            )
            try:
                engine.start()
            except EngineProcessError:
                listener.stop()
                raise

            listener.start()
            sender = EngineSender(
                host=self._settings.command_host, port=self._settings.command_port
            )

            self._session = session
            self._engine = engine
            self._listener = listener
            self._command_sender = sender

        # Outside the lock: triggers the engine's lazy connect-back to the
        # event listener started above.
        sender.send_command("gmTris.new_game", {"starter_mode": starter_mode})
        return session

    def get_session(self, session_id: str) -> SessionState:
        """Returns the active session if *session_id* matches.

        Raises:
            SessionNotFoundError: If no session is active, or the id differs.
        """
        if self._session is None or self._session.session_id != session_id:
            raise SessionNotFoundError(session_id)
        return self._session

    def send_command(self, session_id: str, type_id: str, data: dict) -> None:
        """Forwards one command envelope to the running engine.

        Raises:
            SessionNotFoundError: If *session_id* does not match the active session.
        """
        self.get_session(session_id)
        assert self._command_sender is not None
        self._command_sender.send_command(type_id, data)

    def subscribe(self, session_id: str) -> asyncio.Queue:
        """Registers a new WebSocket subscriber, pre-filled with the last known state.

        Raises:
            SessionNotFoundError: If *session_id* does not match the active session.
        """
        session = self.get_session(session_id)
        queue: asyncio.Queue = asyncio.Queue()
        for envelope in session.last_envelope_by_type.values():
            queue.put_nowait(envelope)
        session.subscribers.add(queue)
        return queue

    def unsubscribe(self, session_id: str, queue: asyncio.Queue) -> None:
        """Removes a previously registered WebSocket subscriber, if still active."""
        session = self._session
        if session is not None and session.session_id == session_id:
            session.subscribers.discard(queue)

    def shutdown(self) -> None:
        """Stops the engine subprocess and the event listener (app shutdown)."""
        with self._lock:
            if self._listener is not None:
                self._listener.stop()
            if self._command_sender is not None:
                self._command_sender.close()
            if self._engine is not None:
                self._engine.stop()
            self._session = None
            self._engine = None
            self._listener = None
            self._command_sender = None

    def _on_envelope(self, session: SessionState, envelope: dict) -> None:
        """Runs on the listener's background thread: fan out thread-safely."""
        session.last_envelope_by_type[envelope.get("typeId", "")] = envelope
        if self.loop is None:
            return
        for queue in list(session.subscribers):
            self.loop.call_soon_threadsafe(queue.put_nowait, envelope)
