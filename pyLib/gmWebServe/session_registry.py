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
import logging
import secrets
import time
import uuid
from collections.abc import Callable, Sequence
from dataclasses import dataclass, field
from pathlib import Path
from threading import Lock

from .auth import DEFAULT_MAX_SESSIONS_PER_USER, DEFAULT_SESSION_IDLE_TIMEOUT_S
from .engine_listener import EngineEventListener, EngineSender
from .engine_process import EngineProcess, EngineProcessError
from .port_utils import find_free_port

_LOGGER = logging.getLogger(__name__)

#: Called with (session, envelope) for every engine event of a session, after
#: the generic fan-out (last-envelope cache + WebSocket subscribers). Lets a
#: game's own SessionManager react server-side to specific events (e.g.
#: Eldhôm auto-resolving a monster's reaction/formation dialog, since no
#: human user owns a monster's identity) without teaching this generic
#: registry any game-specific typeId. Exceptions are caught and logged, never
#: propagated (a buggy hook must not break the generic event fan-out).
OnEventFn = Callable[["GameSession", dict], None]

__all__ = [
    "GameSession",
    "SessionRegistry",
    "SessionNotFoundError",
    "SessionLimitExceededError",
    "SessionFullError",
]

BootstrapFn = Callable[[EngineSender], None]

#: Join-code alphabet excludes visually ambiguous characters (I/O/0/1) so a
#: code can be read aloud or typed from memory without mistakes.
_JOIN_CODE_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
_JOIN_CODE_LENGTH = 6


def _generate_join_code() -> str:
    """Returns one random join code (not guaranteed unique across sessions)."""
    return "".join(secrets.choice(_JOIN_CODE_ALPHABET) for _ in range(_JOIN_CODE_LENGTH))


class SessionNotFoundError(RuntimeError):
    """Raised when a session_id is unknown, or does not belong to the caller.

    Deliberately used for BOTH cases (unknown id vs. wrong owner) so callers
    (typically a FastAPI route) can map it to a single 404 without leaking
    whether a session exists but belongs to someone else.
    """


class SessionLimitExceededError(RuntimeError):
    """Raised when a user is already at their concurrent-session cap."""


class SessionFullError(RuntimeError):
    """Raised when :meth:`SessionRegistry.join_session` finds every seat taken.

    Every :class:`GameSession` role (e.g. ``"X"``/``"O"``) is already held by
    someone else — the join code itself is valid but the match has no free seat.
    """


@dataclass
class GameSession:
    """Everything the registry knows about one running game session.

    A session has one or more named *roles* (e.g. Tris' ``("X", "O")``, or a
    single-player game's default ``("player",)``). :attr:`participants` maps
    each role to the username currently holding it, or ``None`` if that seat
    is still free — see :meth:`SessionRegistry.join_session`. A session is
    SHARED: every participant sees the same engine/event stream, which is how
    two different browsers/users end up playing the same match together.

    Attributes:
        session_id: Opaque unique identifier for this session.
        owner: Username that created this session (from the auth token) —
            always the initial holder of ``roles[0]``. Kept for audit/back
            -compat; access checks use :meth:`is_participant`, not this field.
        join_code: Short human-shareable code (see :meth:`SessionRegistry.
            join_session`) a second user types in to join THIS SAME session.
        roles: Ordered seat names for this session (e.g. ``("X", "O")``).
        participants: Role -> username currently holding it, or ``None``.
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
    join_code: str
    roles: tuple[str, ...]
    participants: dict[str, str | None]
    engine: EngineProcess
    listener: EngineEventListener
    sender: EngineSender
    created_at: float
    last_activity_at: float
    last_envelope_by_type: dict[str, dict] = field(default_factory=dict)
    subscribers: set[asyncio.Queue] = field(default_factory=set)

    def is_participant(self, username: str) -> bool:
        """True if *username* currently holds any role in this session."""
        return username in self.participants.values()

    def role_of(self, username: str) -> str | None:
        """Returns the role *username* holds in this session, or ``None``."""
        for role, holder in self.participants.items():
            if holder == username:
                return role
        return None


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
        self._session_id_by_code: dict[str, str] = {}
        self._on_event_by_session: dict[str, OnEventFn] = {}
        self.loop: asyncio.AbstractEventLoop | None = None

    def create_session(
        self,
        owner: str,
        bootstrap: BootstrapFn,
        extra_args: list[str] | None = None,
        roles: Sequence[str] = ("player",),
        owner_role: str | None = None,
        on_event: OnEventFn | None = None,
    ) -> GameSession:
        """Boots a new engine subprocess and registers a session for *owner*.

        Args:
            owner: Username this session belongs to (from the auth token) —
                assigned *owner_role* (or ``roles[0]`` if not given).
            bootstrap: Called once (outside any lock) with the newly-created
                :class:`~gmWebServe.engine_listener.EngineSender`, so the
                caller can send its game-specific "start a match" command
                (e.g. ``gmTris.new_game``).
            extra_args: Extra CLI arguments for the engine executable (e.g.
                a data directory). The dynamically-allocated
                ``--events-port``/``--commands-port`` flags are appended
                automatically, after these.
            roles: Ordered seat names for this session, e.g. Tris'
                ``("X", "O")``. *owner* fills *owner_role* (or ``roles[0]``
                by default); remaining seats start empty and are filled via
                :meth:`join_session` using the returned session's
                ``join_code``. Defaults to a single seat for games that do
                not need shared multiplayer.
            owner_role: Which role *owner* takes, when the caller lets the
                user CHOOSE instead of always defaulting to ``roles[0]``
                (e.g. Eldhôm: the creator picks which PG/hero to play).
                Must be one of *roles* if given.
            on_event: Optional per-session hook called with (session,
                envelope) for every engine event, after the generic fan-out
                (see :data:`OnEventFn`). Lets a game auto-respond to events
                that need a decision no human participant can make (e.g.
                Eldhôm auto-resolving a monster's reaction/formation dialog).

        Raises:
            ValueError: If *owner_role* is given but not one of *roles*.
            SessionLimitExceededError: If *owner* already has
                ``max_sessions_per_user`` active sessions.
            EngineProcessError: If the engine subprocess fails to start or
                its command port never becomes reachable.
        """
        role_tuple = tuple(roles)
        chosen_owner_role = owner_role if owner_role is not None else role_tuple[0]
        if chosen_owner_role not in role_tuple:
            raise ValueError(f"owner_role {chosen_owner_role!r} not in roles {role_tuple!r}")

        with self._lock:
            owned_count = sum(1 for s in self._sessions.values() if s.is_participant(owner))
            if owned_count >= self.max_sessions_per_user:
                raise SessionLimitExceededError(
                    f"User '{owner}' already has {owned_count} active session(s) "
                    f"(limit {self.max_sessions_per_user})"
                )

            session_id = uuid.uuid4().hex
            join_code = self._generate_unique_join_code_locked()
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

            participants: dict[str, str | None] = {role: None for role in role_tuple}
            participants[chosen_owner_role] = owner

            now = time.time()
            session = GameSession(
                session_id=session_id,
                owner=owner,
                join_code=join_code,
                roles=role_tuple,
                participants=participants,
                engine=engine,
                listener=listener,
                sender=sender,
                created_at=now,
                last_activity_at=now,
            )
            self._sessions[session_id] = session
            self._session_id_by_code[join_code] = session_id
            if on_event is not None:
                self._on_event_by_session[session_id] = on_event

        # Outside the lock: triggers the engine's lazy connect-back to the
        # event listener started above.
        bootstrap(sender)
        return session

    def peek_session_by_code(self, join_code: str) -> GameSession:
        """Looks up a session by *join_code* WITHOUT joining it.

        Lets a would-be joiner see which roles are still free (e.g. to render
        a "pick your PG/hero" screen listing only the unclaimed ones) before
        actually committing to :meth:`join_session`. The join code itself is
        the only "auth" required — same trust model as ``join_session``.

        Raises:
            SessionNotFoundError: If *join_code* matches no active session.
        """
        normalized_code = join_code.strip().upper()
        with self._lock:
            session_id = self._session_id_by_code.get(normalized_code)
            session = self._sessions.get(session_id) if session_id is not None else None
            if session is None:
                raise SessionNotFoundError(normalized_code)
            return session

    def join_session(
        self,
        join_code: str,
        joining_user: str,
        requested_role: str | None = None,
    ) -> GameSession:
        """Attaches *joining_user* to the session identified by *join_code*.

        Idempotent for a user who is already a participant (e.g. a page
        reload just reconnects them to their existing seat) — otherwise
        assigns them to *requested_role* (if given) or the first unfilled
        role, so a second (different) user ends up in the SAME
        session/match as the creator.

        Args:
            join_code: The code shown to the session's creator, shared
                out-of-band (e.g. verbally, chat) with the user who should join.
            joining_user: Username of the user attempting to join.
            requested_role: Which role/seat *joining_user* wants (e.g.
                Eldhôm: which remaining PG/hero to play), when the caller lets
                the user CHOOSE instead of auto-filling the first free role.
                Must be one of the session's roles if given.

        Raises:
            SessionNotFoundError: If *join_code* matches no active session.
            ValueError: If *requested_role* is given but not one of the
                session's roles.
            SessionFullError: If *requested_role* is already held by someone
                else, or (when not given) every role is already taken.
            SessionLimitExceededError: If *joining_user* is already at their
                own concurrent-session cap.
        """
        normalized_code = join_code.strip().upper()
        with self._lock:
            session_id = self._session_id_by_code.get(normalized_code)
            session = self._sessions.get(session_id) if session_id is not None else None
            if session is None:
                raise SessionNotFoundError(normalized_code)
            if session.is_participant(joining_user):
                return session

            if requested_role is not None:
                if requested_role not in session.roles:
                    raise ValueError(
                        f"requested_role {requested_role!r} not in roles {session.roles!r}"
                    )
                if session.participants[requested_role] is not None:
                    raise SessionFullError(normalized_code)
                free_role: str | None = requested_role
            else:
                free_role = next(
                    (role for role, holder in session.participants.items() if holder is None), None
                )
                if free_role is None:
                    raise SessionFullError(normalized_code)

            joined_count = sum(1 for s in self._sessions.values() if s.is_participant(joining_user))
            if joined_count >= self.max_sessions_per_user:
                raise SessionLimitExceededError(
                    f"User '{joining_user}' already has {joined_count} active session(s) "
                    f"(limit {self.max_sessions_per_user})"
                )

            session.participants[free_role] = joining_user
            return session

    def _generate_unique_join_code_locked(self) -> str:
        """Generates a join code not already in use. Caller must hold ``self._lock``."""
        for _ in range(10):
            code = _generate_join_code()
            if code not in self._session_id_by_code:
                return code
        raise RuntimeError("Could not generate a unique join code after 10 attempts")

    def list_sessions(self, username: str) -> list[GameSession]:
        """Returns every active session *username* participates in (may be empty).

        Includes sessions *username* created AND sessions they joined via
        :meth:`join_session` — either way they currently hold one of its roles.
        """
        with self._lock:
            return [s for s in self._sessions.values() if s.is_participant(username)]

    def get_session(self, session_id: str, username: str) -> GameSession:
        """Returns the session if it exists and *username* holds one of its roles.

        Raises:
            SessionNotFoundError: If *session_id* is unknown, or *username* is
                not a participant of it.
        """
        with self._lock:
            session = self._sessions.get(session_id)
        if session is None or not session.is_participant(username):
            raise SessionNotFoundError(session_id)
        return session

    def send_command(self, session_id: str, username: str, type_id: str, data: dict) -> None:
        """Forwards one command envelope to the running engine and marks activity.

        Raises:
            SessionNotFoundError: If *username* is not a participant of *session_id*.
        """
        session = self.get_session(session_id, username)
        session.last_activity_at = time.time()
        session.sender.send_command(type_id, data)

    def subscribe(self, session_id: str, username: str) -> asyncio.Queue:
        """Registers a new WebSocket subscriber, pre-filled with the last known state.

        Every participant of a session shares the SAME event stream — this is
        how two different users end up watching/playing the same match.

        Raises:
            SessionNotFoundError: If *username* is not a participant of *session_id*.
        """
        session = self.get_session(session_id, username)
        session.last_activity_at = time.time()
        queue: asyncio.Queue = asyncio.Queue()
        for envelope in session.last_envelope_by_type.values():
            queue.put_nowait(envelope)
        session.subscribers.add(queue)
        return queue

    def unsubscribe(self, session_id: str, username: str, queue: asyncio.Queue) -> None:
        """Removes a previously registered WebSocket subscriber, if still active."""
        try:
            session = self.get_session(session_id, username)
        except SessionNotFoundError:
            return
        session.subscribers.discard(queue)

    def close_session(self, session_id: str, username: str) -> None:
        """Stops and forgets one session, freeing its slot in the per-user cap.

        Any participant (not just the original creator) may close a shared
        session — it ends the match for everyone in it, symmetric with how
        any player might concede/leave a real board game.

        Raises:
            SessionNotFoundError: If *username* is not a participant of *session_id*.
        """
        with self._lock:
            session = self._sessions.get(session_id)
            if session is None or not session.is_participant(username):
                raise SessionNotFoundError(session_id)
            del self._sessions[session_id]
            self._session_id_by_code.pop(session.join_code, None)
            self._on_event_by_session.pop(session_id, None)
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
                self._session_id_by_code.pop(session.join_code, None)
                self._on_event_by_session.pop(session.session_id, None)
        for session in expired:
            self._teardown(session)
        return [session.session_id for session in expired]

    def shutdown(self) -> None:
        """Stops every active session (app shutdown)."""
        with self._lock:
            sessions = list(self._sessions.values())
            self._sessions.clear()
            self._session_id_by_code.clear()
            self._on_event_by_session.clear()
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
            on_event = self._on_event_by_session.get(session_id)
        if session is None:
            return
        session.last_envelope_by_type[envelope.get("typeId", "")] = envelope
        if self.loop is not None:
            for queue in list(session.subscribers):
                self.loop.call_soon_threadsafe(queue.put_nowait, envelope)
        if on_event is not None:
            try:
                on_event(session, envelope)
            except Exception:
                _LOGGER.exception(
                    "on_event hook raised for session %s, typeId=%s",
                    session_id,
                    envelope.get("typeId"),
                )
