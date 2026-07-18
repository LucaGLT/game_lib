"""eng_serve — FastAPI gateway for the Tic-Tac-Toe WebApp (Phase 1: single session, no auth).

Bridges the browser (REST + WebSocket) to the existing ``tris_engine``
executable, reusing its unchanged TCP/JSON wire contract via
``pyLib/gmGui/engine_bridge``. See GAME/Tic-Tac-Toe/WebApp/PLAN.md for the
full architecture and roadmap.

Run with (from the ``WebApp`` folder, so ``eng_serve`` resolves as a package)::

    cd GAME/Tic-Tac-Toe/WebApp
    uvicorn eng_serve.main:app --reload --port 8000
"""
from __future__ import annotations

import asyncio
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from eng_serve.routers import sessions as sessions_router
from eng_serve.session_manager import SessionManager
from eng_serve.settings import Settings


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncIterator[None]:
    """Creates the SessionManager on startup and shuts it down on exit."""
    settings = Settings()
    manager = SessionManager(settings)
    manager.loop = asyncio.get_running_loop()
    app.state.settings = settings
    app.state.session_manager = manager
    try:
        yield
    finally:
        manager.shutdown()


app = FastAPI(title="eng_serve", version="0.1.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://127.0.0.1:5173", "http://localhost:5173"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(sessions_router.router)


@app.get("/health")
def health() -> dict:
    """Liveness probe — does not touch the engine subprocess."""
    return {"status": "ok"}
