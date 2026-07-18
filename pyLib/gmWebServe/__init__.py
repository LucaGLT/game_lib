"""gmWebServe — shared backend toolkit for exposing gmXxx game engines as web
services (FastAPI + WebSocket), reusing each engine's existing TCP/JSON wire
contract unchanged.

Provides the game-agnostic building blocks every ``eng_serve``-style gateway
needs:

- :class:`EngineEventListener` — Qt-free TCP server that accepts one engine
  connection and streams parsed envelopes to a callback (events: engine -> web).
- :class:`EngineProcess` — spawns and supervises one engine subprocess,
  waiting for its command port to become reachable (commands: web -> engine).
- :func:`find_free_port` — OS-assigned free TCP port allocation.
- :class:`SessionRegistry` — multi-session, multi-user registry (Phase 2):
  one engine subprocess per session, dynamically-allocated ports, a
  per-user concurrent-session cap, and idle-timeout eviction.
- :mod:`gmWebServe.auth` (:class:`AuthConfig`, :class:`AuthService`, ...) —
  pilot-grade username/password login issuing opaque bearer tokens.
- :mod:`gmWebServe.fastapi_deps` / :mod:`gmWebServe.auth_router` — ready-made
  FastAPI dependency + mountable ``/auth`` router built on top of ``auth``.

Both ``EngineEventListener``/``EngineProcess`` were extracted from
``GAME/Tic-Tac-Toe/WebApp/eng_serve`` (Phase 1) unchanged in behaviour and
reused as-is by ``GAME/Eldhom/WebApp/eng_serve`` — see
``GAME/Eldhom/WebApp/PLAN.md``, Phase 1, for the extraction rationale.
``SessionRegistry``/``auth`` generalise the single-session, no-auth
``session_manager.py`` every game previously duplicated (Phase 2).

Per-game specifics (executable path, bootstrap command, extra CLI args, auth
config file location) stay in each game's own ``eng_serve/settings.py`` and
``session_manager.py`` — this package is intentionally free of any
game-specific typeId or payload knowledge.
"""
from __future__ import annotations

from .auth import (
    AuthConfig,
    AuthService,
    AuthSession,
    InvalidCredentialsError,
    InvalidTokenError,
    TokenManager,
    UserAccount,
    hash_password,
    verify_password,
)
from .engine_listener import EngineEventListener, EngineSender
from .engine_process import EngineProcess, EngineProcessError
from .port_utils import find_free_port
from .session_registry import (
    GameSession,
    SessionFullError,
    SessionLimitExceededError,
    SessionNotFoundError,
    SessionRegistry,
)

__all__ = [
    "EngineEventListener",
    "EngineSender",
    "EngineProcess",
    "EngineProcessError",
    "find_free_port",
    "GameSession",
    "SessionRegistry",
    "SessionNotFoundError",
    "SessionLimitExceededError",
    "SessionFullError",
    "AuthConfig",
    "AuthService",
    "AuthSession",
    "TokenManager",
    "UserAccount",
    "InvalidCredentialsError",
    "InvalidTokenError",
    "hash_password",
    "verify_password",
]
