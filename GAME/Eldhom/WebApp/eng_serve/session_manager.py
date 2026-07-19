"""session_manager — Eldhôm-specific thin wrapper over gmWebServe.SessionRegistry.

Shared Multiplayer: real multi-session, multi-user registry (dynamic ports,
per-user concurrent-session cap, idle-timeout cleanup, ownership-checked
access) — mirrors GAME/Tic-Tac-Toe/WebApp/eng_serve/session_manager.py
exactly, generalised for Eldhôm's differences:

- A session's *roles* are not a fixed pair like Tris' ``("X", "O")`` — they
  are the chosen mission's ``pg_roster`` hero_id list (2-5 PGs), resolved by
  the router from ``GET /missions`` before calling :meth:`create_session`.
- Both the creator AND the joiner explicitly CHOOSE which PG/hero to play
  (``owner_hero_id``/``hero_id``), instead of Tris' automatic "X first, O
  second" assignment — see :meth:`create_session`/:meth:`join_session`.
- Anti-spoofing (OWASP A01) is per-HERO, not per-move: any command that acts
  on behalf of one specific hero (``eldhom.simple_action``, ``play_card``,
  ``stop_sequence``, ``declare_attack``, the deck GM-override commands, and
  ``react_defense``'s ``defender_id``) has that field overwritten server-side
  with the caller's OWN assigned hero_id — see :meth:`send_command`. Party
  -wide decision points (``resolve_formation``, ``play_instants``,
  ``play_reactive_instants``) are deliberately NOT restricted to one hero:
  Eldhôm is a cooperative game (not zero-sum like Tris' X-vs-O), so any
  participant may resolve a shared dialog on the party's behalf — equivalent
  to any player being able to move a shared token at a physical table.
- **No participant controls monsters** (see :meth:`_auto_resolve_monster_events`):
  a reaction window whose ``defender_id`` is not one of the session's PGs, or
  a formation dialog none of whose ``actors`` are PGs, would otherwise wait
  forever for a human who does not exist. The server auto-answers these on
  the monsters' behalf (always "Subisci"/TAKE for reactions — these monsters
  have no real parry/dodge capability yet; empty backline for formation,
  i.e. "all to frontline" — both deliberately simple placeholders, revisit
  if/when monster AI gains real reaction choices).
"""
from __future__ import annotations

import sys
from pathlib import Path

# ── Make the shared gmWebServe toolkit importable (pyLib is the common parent
#    of gmWebServe and gmGui) — see GAME/Eldhom/WebApp/PLAN.md, Phase 1, for
#    the extraction rationale (shared 1:1 with the Tris eng_serve).
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

#: typeId -> payload field holding the acting hero_id, for commands that act
#: on behalf of exactly ONE hero. Overwritten server-side in :meth:`SessionManager.
#: send_command` with the caller's own assigned role — never trusted verbatim
#: from the client, so one participant cannot puppet another's PG.
_HERO_OWNED_COMMAND_FIELDS: dict[str, str] = {
    "eldhom.simple_action": "hero_id",
    "eldhom.play_card": "hero_id",
    "eldhom.stop_sequence": "hero_id",
    "eldhom.declare_attack": "hero_id",
    "eldhom.react_defense": "defender_id",
    "eldhom.deck.draw": "hero_id",
    "eldhom.deck.discard": "hero_id",
    "eldhom.deck.take_discard": "hero_id",
    "eldhom.deck.reshuffle": "hero_id",
}

_EVT_REACTION_WINDOW_OPENED = "eldhom.reaction.window_opened"
_EVT_FORMATION_DIALOG_NEEDED = "eldhom.formation.dialog_needed"
_CMD_REACT_DEFENSE = "eldhom.react_defense"
_CMD_RESOLVE_FORMATION = "eldhom.resolve_formation"


class SessionManager:
    """Owns the Eldhôm `SessionRegistry` and knows the `eldhom.start_mission` bootstrap."""

    def __init__(
        self,
        settings: Settings,
        max_sessions_per_user: int,
        idle_timeout_seconds: float,
    ) -> None:
        self._settings = settings
        self._registry: SessionRegistry = SessionRegistry(
            executable=settings.engine_executable,
            event_host=settings.event_host,
            command_host=settings.command_host,
            connect_timeout_s=settings.connect_timeout_s,
            max_sessions_per_user=max_sessions_per_user,
            idle_timeout_seconds=idle_timeout_seconds,
        )
        # Eldhôm-only bookkeeping (NOT part of the generic GameSession): which
        # mission a session is running, so the frontend can resolve hero
        # display names for a `peek_session_by_code` preview. Cleaned up
        # whenever a session is closed (explicitly or reaped idle).
        self._mission_id_by_session: dict[str, str] = {}

    @property
    def loop(self):
        """The asyncio loop used to fan out engine events to WebSocket subscribers."""
        return self._registry.loop

    @loop.setter
    def loop(self, value) -> None:
        self._registry.loop = value

    def create_session(
        self, owner: str, mission_id: str, hero_ids: list[str], owner_hero_id: str
    ) -> GameSession:
        """Boots a new `eldhom_engine` instance for *owner* and starts *mission_id*.

        *owner* fills the *owner_hero_id* seat; share the returned session's
        `join_code` with a second (different) user so they can fill one of
        the remaining seats via :meth:`join_session` and play the SAME
        mission together.

        Args:
            owner: Username creating the session (from the auth token).
            mission_id: Mission to start (validated by the caller — see
                ``routers/sessions.py``, which resolves *hero_ids* from
                ``GET /missions`` before calling this method).
            hero_ids: The mission's full ``pg_roster`` hero_id list — becomes
                this session's `roles`.
            owner_hero_id: Which of *hero_ids* the creator chose to play.

        Raises:
            ValueError: If *owner_hero_id* is not one of *hero_ids*.
            SessionLimitExceededError: If *owner* is already at their
                concurrent-session cap.
            gmWebServe.EngineProcessError: If the engine subprocess fails to
                start or its command port never becomes reachable.
        """
        session = self._registry.create_session(
            owner,
            bootstrap=lambda sender: sender.send_command(
                "eldhom.start_mission", {"mission_id": mission_id}
            ),
            extra_args=[str(self._settings.data_dir)],
            roles=hero_ids,
            owner_role=owner_hero_id,
            on_event=self._auto_resolve_monster_events,
        )
        self._mission_id_by_session[session.session_id] = mission_id
        return session

    def join_session(
        self, join_code: str, joining_user: str, hero_id: str
    ) -> GameSession:
        """Attaches *joining_user* to the *hero_id* seat of the session for *join_code*.

        Idempotent if *joining_user* already holds a seat in that session
        (e.g. a page reload). See `gmWebServe.SessionRegistry.join_session`.

        Raises:
            SessionNotFoundError: If *join_code* matches no active session.
            ValueError: If *hero_id* is not one of the session's roles.
            SessionFullError: If *hero_id* is already held by someone else.
            SessionLimitExceededError: If *joining_user* is already at their
                own concurrent-session cap.
        """
        return self._registry.join_session(join_code, joining_user, requested_role=hero_id)

    def peek_session_by_code(self, join_code: str) -> tuple[GameSession, str | None]:
        """Looks up a session by *join_code* WITHOUT joining it.

        Returns the session (to read `roles`/`join_code`) and its
        `mission_id` (or None if untracked, e.g. server restarted since
        creation), so the frontend can show a "pick your remaining PG/hero"
        screen listing only the unclaimed seats before actually joining.

        Raises:
            SessionNotFoundError: If *join_code* matches no active session.
        """
        session = self._registry.peek_session_by_code(join_code)
        return session, self._mission_id_by_session.get(session.session_id)

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

    def mission_id_of(self, session_id: str) -> str | None:
        """Returns the mission_id *session_id* was created with, or None if untracked."""
        return self._mission_id_by_session.get(session_id)

    def send_command(self, session_id: str, owner: str, type_id: str, data: dict) -> None:
        """Forwards one command envelope to the running engine.

        For any command in :data:`_HERO_OWNED_COMMAND_FIELDS`, the relevant
        hero-id field is ALWAYS overwritten server-side with the caller's own
        assigned seat, ignoring whatever value the client sent — this is what
        stops one participant from puppeting another's PG now that a session
        can have several different authenticated users (OWASP A01 guard).
        Party-wide dialogs (formation/instant-window resolution) are
        deliberately left untouched — see the module docstring.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        hero_field = _HERO_OWNED_COMMAND_FIELDS.get(type_id)
        if hero_field is not None:
            session = self._registry.get_session(session_id, owner)
            role = session.role_of(owner)
            if role is None:
                raise SessionNotFoundError(session_id)
            data = {**data, hero_field: role}
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
        """Closes one of *owner*'s own sessions, freeing its slot in the per-user cap.

        Raises:
            SessionNotFoundError: If *session_id* does not belong to *owner*.
        """
        self._registry.close_session(session_id, owner)
        self._mission_id_by_session.pop(session_id, None)

    def reap_idle_sessions(self) -> list[str]:
        """Closes every session idle too long, also forgetting their mission_id."""
        closed_ids = self._registry.reap_idle_sessions()
        for session_id in closed_ids:
            self._mission_id_by_session.pop(session_id, None)
        return closed_ids

    def shutdown(self) -> None:
        """Stops every active engine subprocess (app shutdown)."""
        self._registry.shutdown()
        self._mission_id_by_session.clear()

    @staticmethod
    def _auto_resolve_monster_events(session: GameSession, envelope: dict) -> None:
        """Server-side `on_event` hook: answers dialogs no human participant can.

        A session's `roles` are exactly its mission's PG/hero ids (see
        :meth:`create_session`) — anything NOT in there is a monster. Runs on
        the engine listener's background thread (same as the generic fan-out
        that calls it); uses `session.sender` directly rather than going
        through :meth:`send_command`, since there is no *owner* here to
        authenticate against.

        Handles:
          - `eldhom.reaction.window_opened`: if `defender_id` is not a PG,
            always answers "TAKE" (Subisci) — these monsters have no real
            parry/dodge capability yet.
          - `eldhom.formation.dialog_needed`: if none of the listed `actors`
            are PGs (an all-monster formation dialog), answers with an empty
            backline (everyone to Prima Linea) — always a valid (RG<=PL)
            choice, and the simplest sensible default until monster AI needs
            a smarter one.
        """
        type_id = envelope.get("typeId")
        data = envelope.get("data", {})

        if type_id == _EVT_REACTION_WINDOW_OPENED:
            defender_id = data.get("defender_id", "")
            if defender_id and defender_id not in session.roles:
                session.sender.send_command(
                    _CMD_REACT_DEFENSE, {"defender_id": defender_id, "reaction": "TAKE"}
                )
            return

        if type_id == _EVT_FORMATION_DIALOG_NEEDED:
            actors = data.get("actors", [])
            actor_ids = [a.get("actor_id", "") for a in actors if isinstance(a, dict)]
            if actor_ids and not any(actor_id in session.roles for actor_id in actor_ids):
                session.sender.send_command(
                    _CMD_RESOLVE_FORMATION,
                    {
                        "faction_id": data.get("faction_id", ""),
                        "location_id": data.get("location_id", ""),
                        "backline": [],
                    },
                )
            return

