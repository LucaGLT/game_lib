"""settings — eng_serve configuration via pydantic-settings (Eldhôm).

Values can be overridden with ``ENGSERVE_*`` environment variables or a
``.env`` file placed next to this package (e.g. ``ENGSERVE_EVENT_PORT=9400``).
"""
from __future__ import annotations

from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

_REPO_ROOT = Path(__file__).resolve().parents[4]
_ENGINE_BUILD_DIR = _REPO_ROOT / "build" / "GAME" / "Eldhom" / "CoreEngine"
_DATA_DIR = _REPO_ROOT / "GAME" / "Eldhom" / "data"


def _default_engine_executable() -> Path:
    """Locates the built ``eldhom_engine`` executable (Debug preferred, then Release).

    Falls back to the Debug path even when it does not exist yet, so that a
    later failure message (see :class:`gmWebServe.EngineProcessError`) points
    at the expected location instead of an empty string.
    """
    for candidate in (
        _ENGINE_BUILD_DIR / "Debug" / "eldhom_engine.exe",
        _ENGINE_BUILD_DIR / "Release" / "eldhom_engine.exe",
        _ENGINE_BUILD_DIR / "eldhom_engine.exe",
        _ENGINE_BUILD_DIR / "eldhom_engine",
    ):
        if candidate.exists():
            return candidate
    return _ENGINE_BUILD_DIR / "Debug" / "eldhom_engine.exe"


class Settings(BaseSettings):
    """Runtime configuration for the eng_serve gateway (Shared Multiplayer: multi-session + auth).

    Attributes:
        engine_executable:  Path to the compiled ``eldhom_engine`` executable.
        data_dir:           Data directory passed as the engine's first CLI
                             argument (mission/card/behavior JSON files) —
                             also scanned by ``GET /missions``/``GET /cards``.
        event_host:         Bind address for each session's event listener.
        command_host:       Bind address the engine's command server listens on.
        connect_timeout_s:  Max seconds to wait for the engine's command port at startup.
        default_mission_id: Mission started by ``POST /sessions`` when no
                             ``mission_id`` is given in the request body.
        auth_config_path:   Path to the pilot-grade users/limits JSON file (see
                             ``gmWebServe.tools.manage_users`` to create/update it).
        cors_allow_origins: Origins allowed to call this API from a browser (Vite dev server).
    """

    engine_executable: Path = _default_engine_executable()
    data_dir: Path = _DATA_DIR

    # NOTE: eldhom_engine.exe's events/commands ports are now dynamically
    # allocated PER SESSION by gmWebServe.SessionRegistry (see
    # session_manager.py) and passed to each engine instance via
    # --events-port/--commands-port CLI arguments (see
    # GAME/Eldhom/CoreEngine/main.cpp) — there is no longer a single fixed
    # port here. eldhom::ports::EVENTS/COMMANDS (9210/9211) remain the
    # engine's own compiled-in defaults when launched with no port
    # arguments at all (the desktop GUI's direct-connect flow, unchanged).
    event_host: str = "127.0.0.1"
    command_host: str = "127.0.0.1"

    connect_timeout_s: float = 10.0
    default_mission_id: str = "missione_sim_a"

    auth_config_path: Path = Path(__file__).resolve().parent / "auth_config.json"

    cors_allow_origins: list[str] = [
        "http://127.0.0.1:5173",
        "http://localhost:5173",
    ]

    model_config = SettingsConfigDict(
        env_prefix="ENGSERVE_", env_file=".env", extra="ignore"
    )
