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
    """Runtime configuration for the eng_serve gateway (Phase 1: single session).

    Attributes:
        engine_executable:  Path to the compiled ``eldhom_engine`` executable.
        data_dir:           Data directory passed as the engine's first CLI
                             argument (mission/card/behavior JSON files) —
                             also scanned by ``GET /missions``.
        event_host:         Bind address for the event listener (engine -> eng_serve).
        event_port:         TCP port eng_serve listens on; the engine connects back to it.
        command_host:       Host of the engine's command server (eng_serve -> engine).
        command_port:       TCP port of the engine's command server.
        connect_timeout_s:  Max seconds to wait for the engine's command port at startup.
        default_mission_id: Mission started by ``POST /sessions`` when no
                             ``mission_id`` is given in the request body.
        cors_allow_origins: Origins allowed to call this API from a browser (Vite dev server).
    """

    engine_executable: Path = _default_engine_executable()
    data_dir: Path = _DATA_DIR

    # NOTE: eldhom_engine.exe has these ports HARD-CODED as C++ constexpr values
    # (GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp::ports::EVENTS/COMMANDS =
    # 9210/9211) — there is no argv/env override, so eng_serve MUST use the
    # same values. Consequence (same as Tris Phase 1): eng_serve and the
    # desktop GUI cannot both be connected to the SAME engine instance at once.
    event_host: str = "127.0.0.1"
    event_port: int = 9210
    command_host: str = "127.0.0.1"
    command_port: int = 9211

    connect_timeout_s: float = 10.0
    default_mission_id: str = "missione_sim_a"

    cors_allow_origins: list[str] = [
        "http://127.0.0.1:5173",
        "http://localhost:5173",
    ]

    model_config = SettingsConfigDict(
        env_prefix="ENGSERVE_", env_file=".env", extra="ignore"
    )
