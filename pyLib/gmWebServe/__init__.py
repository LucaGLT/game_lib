"""gmWebServe — shared backend toolkit for exposing gmXxx game engines as web
services (FastAPI + WebSocket), reusing each engine's existing TCP/JSON wire
contract unchanged.

Provides the two game-agnostic building blocks every ``eng_serve``-style
gateway needs:

- :class:`EngineEventListener` — Qt-free TCP server that accepts one engine
  connection and streams parsed envelopes to a callback (events: engine -> web).
- :class:`EngineProcess` — spawns and supervises one engine subprocess,
  waiting for its command port to become reachable (commands: web -> engine).

Both classes were extracted from ``GAME/Tic-Tac-Toe/WebApp/eng_serve`` (Phase 1)
unchanged in behaviour and reused as-is by ``GAME/Eldhom/WebApp/eng_serve`` —
see ``GAME/Eldhom/WebApp/PLAN.md``, Phase 1, for the extraction rationale.

Per-game specifics (executable path, ports, bootstrap command, session
lifecycle) stay in each game's own ``eng_serve/settings.py`` and
``session_manager.py`` — this package is intentionally free of any
game-specific typeId or payload knowledge.
"""
from __future__ import annotations

from .engine_listener import EngineEventListener, EngineSender
from .engine_process import EngineProcess, EngineProcessError

__all__ = [
    "EngineEventListener",
    "EngineSender",
    "EngineProcess",
    "EngineProcessError",
]
