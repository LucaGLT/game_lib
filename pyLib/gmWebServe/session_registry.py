"""session_registry — generic multi-session, multi-user registry for any
``eng_serve``-style gateway.

Owns up to ``max_sessions_per_user`` :class:`GameSession` per authenticated
user, each backed by its own engine subprocess on a dynamically-allocated
events/commands port pair (process-per-session model — mirrors the single
-session ``SessionManager`` every game previously duplicated, generalised and
extracted here so any game_lib WebApp can reuse it unchanged).

Game-specific knowledge (executable path, bootstrap command such as
``gmTris.new_game``/``eldhom.start_mission``, extra CLI args like a data
directory) stays entirely with the caller via the ``bootstrap``/``extra_args``
parameters of :meth:`SessionRegistry.create_session` — this module never sees
a game-specific typeId or payload.
"""
from __future__ import annotations

import asyncio
import time
import uuid
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path
from threading import Lock

from .auth import DEFAULT_MAX_SESSIONS_PER_USER, DEFAULT_SESSION_IDLE_TIMEOUT_S
from .engine_listener import EngineEventListener, EngineSender
from .engine_process import EngineProcess, EngineProcessError
from .port_utils import find_free_port

__all__ = [
    "GameSession",
    "SessionRegistry",
    "SessionNotFoundError",
    "SessionLimitExceededError",
]

BootstrapFn = Callable[[EngineSender], None]


class SessionNotFoundError(RuntimeError):
    """Raised when a session_id is unknown, or does not belong to the caller.

    Deliberately used for BOTH cases (unknown id vs. wrong owner) so callers
    (typically a FastAPI route) can map it to a single 404 without leaking
    whether a session exists but belongs to someone else.
    """


class SessionLimitExceededError(RuntimeError):
    """Raised when a user is already at their concurrent-session cap."""


@dataclass
class GameSession:
    """Everything the registry knows about one running game session.

    Attributes:
        session_id: Opaque unique identifier for this session.
        owner: Username that created this session (from the auth token).
        engine: The subprocess wrapper for the running game engine.
        listener: TCP server accepting the engine's outbound events.
        sender: TCP client used to forward commands to the engine.
        created_at: ``time.time()`` when the session was created.
        last_activity_at: ``time.time()`` of the last user-initiated action
            (command sent or WebSocket (re)subscription) — drives idle-timeout
            eviction, see :meth:`SessionRegistry.reap_idle_sessions`.
        last_envelope_by_type: Latest envelope seen for each ``typeId``, used
            to replay current state to a browser tab that connects late.
        subscribers: Active WebSocket subscriber queues.
    """

    session_id: str
    owner: str
    engine: EngineProcess
    listener: EngineEventListener
    sender: EngineSender
    created_at: float
    last_activity_at: float
    last_envelope_by_type: dict[str, dict] = field(default_factory=dict)
    subscribers: set[asyncio.Queue] = field(default_factory=set)


class SessionRegistry:
    """Owns every active :class:`GameSession` for every user of one game.

    Thread-safety: mutations to the session table are guarded by a lock;
    slow I/O (spawning/stopping a subprocess, sending the bootstrap command)
    deliberately happens OUTSIDE the lock so one session's startup/shutdown
    never blocks another session's requests.
    """

    def __init__(
        self,
        executable: Path,
        event_host: str = "127.0.0.1",
        command_host: str = "127.0.0.1",
        connect_timeout_s: float = 10.0,
        max_sessions_per_user: int = DEFAULT_MAX_SESSIONS_PER_USER,
        idle_timeout_seconds: float = DEFAULT_SESSION_IDLE_TIMEOUT_S,
    ) -> None:
        self._executable: Path = executable
        self._event_host: str = event_host
        self._command_host: str = command_host
        self._connect_timeout_s: float = connect_timeout_s
        self.max_sessions_per_user: int = max_sessions_per_user
        self.idle_timeout_seconds: float = idle_timeout_seconds
        self._lock: Lock = Lock()
        self._sessions: dict[str, GameSession] = {}
        self.loop: asyncio.AbstractEventLoop | None = None

    def create_session(
        self,
        owner: str,
        bootstrap: BootstrapFn,
        extra_args: list[str] | None = None,
    ) -> GameSession:
        """Boots a new engine subprocess and registers a session for *owner*.

        Args:
            owner: Username this session belongs to (from the auth token).
            bootstrap: Called once (outside any lock) with the newly-created
                :class:`~gmWebServe.engine_listener.EngineSender`, so the
                caller can send its game-specific "start a match" command
                (e.g. ``gmTris.new_game``).
            extra_args: Extra CLI arguments for the engine executable (e.g.
                a data directory). The dynamically-allocated
                ``--events-port``/``--commands-port`` flags are appended
                automatically, after these.

        Raises:
            SessionLimitExceededError: If *owner* already has
                ``max_sessions_per_user`` active sessions.
            EngineProcessError: If the engine subprocess fails to start or
                its command port never becomes reachable.
        """
        with self._lock:
            owned_count = sum(1 for s in self._sessions.values() if s.owner == owner)
            if owned_count >= self.max_sessions_per_user:
                raise SessionLimitExceededError(
                    f"User '{owner}' already has {owned_count} active session(s) "
                    f"(limit {self.max_sessions_per_user})"
                )

            session_id = uuid.uuid4().hex
            event_port = find_free_port(self._event_host)
            command_port = find_free_port(self._command_host)

            listener = EngineEventListener(
                self._event_host,
                event_port,
                on_envelope=lambda envelope: self._on_envelope(session_id, envelope),
            )
            listener.bind()

            engine_args = list(extra_args) if extra_args else []
            engine_args += ["--events-port", str(event_port), "--commands-port", str(command_port)]

            engine = EngineProcess(
                self._executable,
                self._command_host,
                command_port,
                self._connect_timeout_s,
                extra_args=engine_args,
            )
            try:
                engine.start()
            except EngineProcessError:
                listener.stop()
                raise

            listener.start()
            sender = EngineSender(host=self._command_host, port=command_port)

            now = time.time()
            session = GameSession(
                session_id=session_id,
                owner=owner,
                engine=engine,
                listener=listener,
                sender=sender,
                created_at=now,
                last_activity_at=now,
            )
            self._sessions[session_id] = session

        # Outside the lock: triggers the engine's lazy connect-back to the
        # event listener started above.
        bootstrap(sender)
        return session

    def list_sessions(self, owner: str) -> list[GameSession]:
        """Returns every active session belonging to *owner* (may be empty)."""
        with self._lock:
            return [s for s in self._sessions.values() if s.owner == owner]

    def get_session(self, session_id: str, owner: str) -> GameSession:
        """Returns the session if it exists and belongs to *owner*.

        Raises:
            SessionNotFoundError: If *session_id* is unknown, or belongs to a
                different owner.
        """
        with self._lock:
            session = self._sessions.get(session_id)
        if session is None or session.owner != owner:
            raise SessionNotFoundError(session_id)
        return session

    def send_command(self, session_id: str, owner: str, type_id: str, data: dict) -> None:
        """Forwards one command envelope to the running engine and marks activity.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        session = self.get_session(session_id, owner)
        session.last_activity_at = time.time()
        session.sender.send_command(type_id, data)

    def subscribe(self, session_id: str, owner: str) -> asyncio.Queue:
        """Registers a new WebSocket subscriber, pre-filled with the last known state.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        session = self.get_session(session_id, owner)
        session.last_activity_at = time.time()
        queue: asyncio.Queue = asyncio.Queue()
        for envelope in session.last_envelope_by_type.values():
            queue.put_nowait(envelope)
        session.subscribers.add(queue)
        return queue

    def unsubscribe(self, session_id: str, owner: str, queue: asyncio.Queue) -> None:
        """Removes a previously registered WebSocket subscriber, if still active."""
        try:
            session = self.get_session(session_id, owner)
        except SessionNotFoundError:
            return
        session.subscribers.discard(queue)

    def close_session(self, session_id: str, owner: str) -> None:
        """Stops and forgets one session, freeing its slot in the per-user cap.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        with self._lock:
            session = self._sessions.get(session_id)
            if session is None or session.owner != owner:
                raise SessionNotFoundError(session_id)
            del self._sessions[session_id]
        self._teardown(session)

    def reap_idle_sessions(self) -> list[str]:
        """Closes every session idle for longer than ``idle_timeout_seconds``.

        Intended to be called periodically (e.g. from a background task in
        the app's lifespan) — see each game's ``eng_serve/main.py``.

        Returns:
            The ids of the sessions that were closed.
        """
        now = time.time()
        with self._lock:
            expired = [
                session
                for session in self._sessions.values()
                if now - session.last_activity_at > self.idle_timeout_seconds
            ]
            for session in expired:
                del self._sessions[session.session_id]
        for session in expired:
            self._teardown(session)
        return [session.session_id for session in expired]

    def shutdown(self) -> None:
        """Stops every active session (app shutdown)."""
        with self._lock:
            sessions = list(self._sessions.values())
            self._sessions.clear()
        for session in sessions:
            self._teardown(session)

    def _teardown(self, session: GameSession) -> None:
        """Stops the listener/sender/engine for *session* (slow I/O, no lock held)."""
        session.listener.stop()
        session.sender.close()
        session.engine.stop()

    def _on_envelope(self, session_id: str, envelope: dict) -> None:
        """Runs on the listener's background thread: fan out thread-safely."""
        with self._lock:
            session = self._sessions.get(session_id)
        if session is None:
            return
        session.last_envelope_by_type[envelope.get("typeId", "")] = envelope
        if self.loop is None:
            return
        for queue in list(session.subscribers):
            self.loop.call_soon_threadsafe(queue.put_nowait, envelope)
