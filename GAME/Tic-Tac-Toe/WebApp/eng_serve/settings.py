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
    """Runtime configuration for the eng_serve gateway (Phase 2: multi-session + auth).

    Attributes:
        engine_executable:   Path to the compiled ``tris_engine`` executable.
        event_host:          Bind address for each session's event listener.
        command_host:        Bind address the engine's command server listens on.
        connect_timeout_s:   Max seconds to wait for the engine's command port at startup.
        auth_config_path:    Path to the pilot-grade users/limits JSON file (see
                              ``gmWebServe.tools.manage_users`` to create/update it).
        cors_allow_origins:  Origins allowed to call this API from a browser (Vite dev server).
    """

    engine_executable: Path = _default_engine_executable()

    # NOTE: tris_engine.exe's events/commands ports are now dynamically
    # allocated PER SESSION by gmWebServe.SessionRegistry (see
    # session_manager.py) and passed to each engine instance via
    # --events-port/--commands-port CLI arguments (see
    # GAME/Tic-Tac-Toe/CoreEngine/main.cpp) — there is no longer a single
    # fixed port here. gmTris::ports::EVENTS/COMMANDS (9100/9001) remain the
    # engine's own compiled-in defaults when launched with no arguments at
    # all (the desktop GUI's direct-connect flow, unchanged).
    event_host: str = "127.0.0.1"
    command_host: str = "127.0.0.1"

    connect_timeout_s: float = 10.0

    auth_config_path: Path = Path(__file__).resolve().parent / "auth_config.json"

    cors_allow_origins: list[str] = [
        "http://127.0.0.1:5173",
        "http://localhost:5173",
    ]

    model_config = SettingsConfigDict(
        env_prefix="ENGSERVE_", env_file=".env", extra="ignore"
    )
