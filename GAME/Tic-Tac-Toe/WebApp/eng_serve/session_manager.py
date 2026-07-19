"""session_manager — Tris-specific thin wrapper over gmWebServe.SessionRegistry.

Phase 2: real multi-session, multi-user registry (dynamic ports, per-user
concurrent-session cap, idle-timeout cleanup, ownership-checked access) — see
GAME/Tic-Tac-Toe/WebApp/PLAN.md, Phase 2. Phase "Shared Multiplayer": a session
now has two named roles (``"X"``/``"O"``), and a SECOND (different) user can
join the SAME match via a short join code — see
:meth:`SessionManager.join_session`. All of the multi-session/auth/shared
-participant machinery lives in the shared ``pyLib/gmWebServe`` library; this
module's only Tris-specific knowledge is the bootstrap command
(``gmTris.new_game``), the two role names, and the move-command field
(``player``) that must be bound server-side to whichever role the caller
actually holds (never trust a client-supplied mark — see :meth:`send_command`).
"""
from __future__ import annotations

import sys
from pathlib import Path

# ── Make the shared gmWebServe toolkit importable (pyLib is the common parent
#    of gmWebServe and gmGui) — see GAME/Eldhom/WebApp/PLAN.md, Phase 1, for
#    the extraction rationale (shared 1:1 with the Eldhom eng_serve).
_PYLIB_DIR = Path(__file__).resolve().parents[4] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))

from gmWebServe import (  # noqa: E402
    GameSession,
    SessionFullError,
    SessionLimitExceededError,
    SessionNotFoundError,
    SessionRegistry,
)

from .settings import Settings

__all__ = [
    "SessionManager",
    "GameSession",
    "SessionNotFoundError",
    "SessionLimitExceededError",
    "SessionFullError",
]

#: The two Tris seats — the session creator always fills "X" first;
#: a second (different) user fills "O" via :meth:`SessionManager.join_session`.
TRIS_ROLES = ("X", "O")

#: gmTris.move's mark field — bound server-side to the CALLER's own role in
#: :meth:`SessionManager.send_command`, never trusted verbatim from the
#: client, so one participant cannot forge a move on the other's behalf.
_MOVE_TYPE_ID = "gmTris.move"
_MOVE_PLAYER_FIELD = "player"


class SessionManager:
    """Owns the Tris `SessionRegistry` and knows the `gmTris.new_game` bootstrap."""

    def __init__(
        self,
        settings: Settings,
        max_sessions_per_user: int,
        idle_timeout_seconds: float,
    ) -> None:
        self._registry: SessionRegistry = SessionRegistry(
            executable=settings.engine_executable,
            event_host=settings.event_host,
            command_host=settings.command_host,
            connect_timeout_s=settings.connect_timeout_s,
            max_sessions_per_user=max_sessions_per_user,
            idle_timeout_seconds=idle_timeout_seconds,
        )

    @property
    def loop(self):
        """The asyncio loop used to fan out engine events to WebSocket subscribers."""
        return self._registry.loop

    @loop.setter
    def loop(self, value) -> None:
        self._registry.loop = value

    def create_session(self, owner: str, starter_mode: str) -> GameSession:
        """Boots a new `tris_engine` instance for *owner* and starts a match.

        *owner* fills the "X" seat; share the returned session's `join_code`
        with a second (different) user so they can fill "O" via
        :meth:`join_session` and play the SAME match.

        Raises:
            SessionLimitExceededError: If *owner* is already at their
                concurrent-session cap.
            gmWebServe.EngineProcessError: If the engine subprocess fails to
                start or its command port never becomes reachable.
        """
        return self._registry.create_session(
            owner,
            bootstrap=lambda sender: sender.send_command(
                "gmTris.new_game", {"starter_mode": starter_mode}
            ),
            roles=TRIS_ROLES,
        )

    def join_session(self, join_code: str, joining_user: str) -> GameSession:
        """Attaches *joining_user* to the "O" seat of the session for *join_code*.

        Idempotent if *joining_user* already holds a seat in that session
        (e.g. a page reload). See `gmWebServe.SessionRegistry.join_session`.

        Raises:
            SessionNotFoundError: If *join_code* matches no active session.
            SessionFullError: If both seats are already held by others.
            SessionLimitExceededError: If *joining_user* is already at their
                own concurrent-session cap.
        """
        return self._registry.join_session(join_code, joining_user)

    def list_sessions(self, owner: str) -> list[GameSession]:
        """Returns every active session belonging to *owner* (may be empty)."""
        return self._registry.list_sessions(owner)

    def get_session(self, session_id: str, owner: str) -> GameSession:
        """Returns the session if it exists and belongs to *owner*.

        Raises:
            SessionNotFoundError: If *session_id* is unknown, or belongs to a
                different owner.
        """
        return self._registry.get_session(session_id, owner)

    def send_command(self, session_id: str, owner: str, type_id: str, data: dict) -> None:
        """Forwards one command envelope to the running engine.

        For `gmTris.move`, the `player` field is ALWAYS overwritten
        server-side with the caller's own assigned seat ("X"/"O"), ignoring
        whatever value the client sent — this is what stops the "O" player
        from forging a move as "X" (or vice-versa) now that a session can
        have two different authenticated users (OWASP A01 guard).

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        if type_id == _MOVE_TYPE_ID:
            session = self._registry.get_session(session_id, owner)
            role = session.role_of(owner)
            if role is None:
                raise SessionNotFoundError(session_id)
            data = {**data, _MOVE_PLAYER_FIELD: role}
        self._registry.send_command(session_id, owner, type_id, data)

    def subscribe(self, session_id: str, owner: str):
        """Registers a new WebSocket subscriber, pre-filled with the last known state.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        return self._registry.subscribe(session_id, owner)

    def unsubscribe(self, session_id: str, owner: str, queue) -> None:
        """Removes a previously registered WebSocket subscriber, if still active."""
        self._registry.unsubscribe(session_id, owner, queue)

    def close_session(self, session_id: str, owner: str) -> None:
        """Stops and forgets one session, freeing its slot in the per-user cap.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        self._registry.close_session(session_id, owner)

    def reap_idle_sessions(self) -> list[str]:
        """Closes every session idle for longer than the configured timeout."""
        return self._registry.reap_idle_sessions()

    def shutdown(self) -> None:
        """Stops every active session (app shutdown)."""
        self._registry.shutdown()
