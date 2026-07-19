"""eng_serve — FastAPI gateway for Le Pergamene di Eldhôm WebApp (Shared Multiplayer: multi-session + auth).

Bridges the browser (REST + WebSocket) to the existing ``eldhom_engine``
executable, reusing its unchanged TCP/JSON wire contract via
``pyLib/gmWebServe`` (shared with the Tic-Tac-Toe eng_serve). See
GAME/Eldhom/WebApp/PLAN.md for the full architecture and roadmap.

Run with (from the ``WebApp`` folder, so ``eng_serve`` resolves as a package)::

    cd GAME/Eldhom/WebApp
    uvicorn eng_serve.main:app --reload --port 8100

(port 8100, not 8000, so both games' dev servers can run side by side).
"""
from __future__ import annotations

import asyncio
import contextlib
import sys
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

_PYLIB_DIR = Path(__file__).resolve().parents[4] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))

from gmWebServe.auth import AuthConfig, AuthService  # noqa: E402
from gmWebServe.auth_router import router as auth_router  # noqa: E402

from eng_serve.routers import sessions as sessions_router
from eng_serve.session_manager import SessionManager
from eng_serve.settings import Settings

_REAP_INTERVAL_S = 60.0


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncIterator[None]:
    """Creates the auth service + SessionManager on startup, tears both down on exit."""
    settings = Settings()
    auth_config = AuthConfig.load(settings.auth_config_path)
    auth_service = AuthService(auth_config)
    manager = SessionManager(
        settings,
        max_sessions_per_user=auth_config.max_sessions_per_user,
        idle_timeout_seconds=auth_config.session_idle_timeout_seconds,
    )
    manager.loop = asyncio.get_running_loop()
    app.state.settings = settings
    app.state.auth_service = auth_service
    app.state.session_manager = manager

    reaper_task = asyncio.create_task(_reap_idle_sessions_periodically(manager))
    try:
        yield
    finally:
        reaper_task.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            await reaper_task
        manager.shutdown()


async def _reap_idle_sessions_periodically(manager: SessionManager) -> None:
    """Background task: evicts idle sessions every ``_REAP_INTERVAL_S`` seconds.

    Runs ``reap_idle_sessions`` in a worker thread since it may stop one or
    more engine subprocesses (blocking I/O), which must not stall the
    asyncio event loop.
    """
    loop = asyncio.get_running_loop()
    while True:
        await asyncio.sleep(_REAP_INTERVAL_S)
        await loop.run_in_executor(None, manager.reap_idle_sessions)


app = FastAPI(title="eng_serve (Eldhôm)", version="0.2.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://127.0.0.1:5173", "http://localhost:5173"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(auth_router)
app.include_router(sessions_router.router)


@app.get("/health")
def health() -> dict:
    """Liveness probe — does not touch the engine subprocess."""
    return {"status": "ok"}

