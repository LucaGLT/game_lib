"""settings — eng_serve configuration via pydantic-settings.

Values can be overridden with ``ENGSERVE_*`` environment variables or a
``.env`` file placed next to this package (e.g. ``ENGSERVE_EVENT_PORT=9400``).
"""
from __future__ import annotations

from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

_REPO_ROOT = Path(__file__).resolve().parents[4]
_ENGINE_BUILD_DIR = _REPO_ROOT / "build" / "GAME" / "Tic-Tac-Toe" / "CoreEngine"


def _default_engine_executable() -> Path:
    """Locates the built ``tris_engine`` executable (Debug preferred, then Release).

    Falls back to the Debug path even when it does not exist yet, so that a
    later failure message (see :class:`engine_process.EngineProcessError`)
    points at the expected location instead of an empty string.
    """
    for candidate in (
        _ENGINE_BUILD_DIR / "Debug" / "tris_engine.exe",
        _ENGINE_BUILD_DIR / "Release" / "tris_engine.exe",
        _ENGINE_BUILD_DIR / "tris_engine.exe",
        _ENGINE_BUILD_DIR / "tris_engine",
    ):
        if candidate.exists():
            return candidate
    return _ENGINE_BUILD_DIR / "Debug" / "tris_engine.exe"


class Settings(BaseSettings):
    """Runtime configuration for the eng_serve gateway (Phase 1: single session).

    Attributes:
        engine_executable:   Path to the compiled ``tris_engine`` executable.
        event_host:          Bind address for the event listener (engine -> eng_serve).
        event_port:          TCP port the engine connects back to for events.
        command_host:        Host of the engine's command server (eng_serve -> engine).
        command_port:        TCP port of the engine's command server.
        connect_timeout_s:   Max seconds to wait for the engine's command port at startup.
        cors_allow_origins:  Origins allowed to call this API from a browser (Vite dev server).
    """

    engine_executable: Path = _default_engine_executable()

    # NOTE: tris_engine.exe has these ports HARD-CODED as C++ constexpr values
    # (GAME/Tic-Tac-Toe/CoreEngine/engine/TrisTypes.hpp: EVENTS=9100, COMMANDS=9001)
    # — there is no argv/env override, so eng_serve MUST use the same values.
    # Consequence for Phase 1: eng_serve and the desktop GUI cannot both be
    # connected to the SAME engine instance at once (only one process can
    # bind port 9100). Phase 2's per-session dynamic ports will require a
    # small, scoped C++ change to make these ports configurable.
    event_host: str = "127.0.0.1"
    event_port: int = 9100
    command_host: str = "127.0.0.1"
    command_port: int = 9001

    connect_timeout_s: float = 10.0

    cors_allow_origins: list[str] = [
        "http://127.0.0.1:5173",
        "http://localhost:5173",
    ]

    model_config = SettingsConfigDict(
        env_prefix="ENGSERVE_", env_file=".env", extra="ignore"
    )
