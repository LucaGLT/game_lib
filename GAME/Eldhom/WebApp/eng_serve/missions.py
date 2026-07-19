"""missions — server-side scan of the mission JSON files in ``data_dir``.

Replaces the desktop GUI's local-filesystem scan
(``GAME/Eldhom/GUI/app/mission_select_dialog.py::_scan_missions``), which
cannot run in the browser (the SPA has no access to the server's disk).

Reads each mission file's own ``mission_id``/``title``/``description``
fields rather than deriving ``mission_id`` from the filename: the engine's
``MissionLoader`` applies its own filename transform (e.g. mission_id
``"missione_sim_a"`` -> file ``mission_sim_a.json``), so trusting the file's
declared id avoids re-implementing that mapping here.

Also exposes each mission's ``pg_roster`` (hero_id/display_name/class_name)
so the frontend can offer a "pick your PG/hero" screen before/while creating
a shared multiplayer session (Shared Multiplayer feature) — see
:func:`hero_ids_for_mission`, used by ``routers/sessions.py`` to validate a
chosen ``hero_id`` and to build the session's ``roles`` tuple.
"""
from __future__ import annotations

import json
from pathlib import Path


def _read_mission_json(data_dir: Path, mission_id: str) -> dict | None:
    """Returns the raw parsed JSON of the mission file whose declared id matches, or None."""
    if not data_dir.is_dir():
        return None
    for path in sorted(data_dir.glob("mission_*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            continue
        if isinstance(data, dict) and str(data.get("mission_id", "")) == mission_id:
            return data
    return None


def _roster_summary(data: dict) -> list[dict]:
    """Extracts the ``{hero_id, display_name, class_name}`` summary from a mission's ``pg_roster``."""
    roster = data.get("pg_roster", [])
    if not isinstance(roster, list):
        return []
    result: list[dict] = []
    for entry in roster:
        if not isinstance(entry, dict) or "hero_id" not in entry:
            continue
        result.append(
            {
                "hero_id": str(entry["hero_id"]),
                "display_name": str(entry.get("display_name", entry["hero_id"])),
                "class_name": str(entry.get("class_name", "")),
            }
        )
    return result


def scan_missions(data_dir: Path) -> list[dict]:
    """Reads ``mission_*.json`` files under *data_dir* into summary dicts.

    Args:
        data_dir: Directory to scan (same one passed to the engine as its
            data-dir CLI argument).

    Returns:
        A list of ``{"mission_id", "title", "description", "pg_roster"}``
        dicts, one per readable mission file (unreadable/malformed files are
        skipped silently, same tolerance as the desktop dialog). ``pg_roster``
        is a list of ``{"hero_id", "display_name", "class_name"}``.
    """
    result: list[dict] = []
    if not data_dir.is_dir():
        return result
    for path in sorted(data_dir.glob("mission_*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            continue
        if not isinstance(data, dict):
            continue
        result.append(
            {
                "mission_id": str(data.get("mission_id", path.stem)),
                "title": str(data.get("title", path.name)),
                "description": str(data.get("description", "")),
                "pg_roster": _roster_summary(data),
            }
        )
    return result


def hero_ids_for_mission(data_dir: Path, mission_id: str) -> list[str] | None:
    """Returns the ordered ``hero_id`` list of *mission_id*'s ``pg_roster``, or None if not found.

    Used by ``routers/sessions.py`` to build a shared session's ``roles``
    tuple (one seat per PG/hero) and to validate a client-chosen ``hero_id``
    without trusting it blindly.
    """
    data = _read_mission_json(data_dir, mission_id)
    if data is None:
        return None
    roster = _roster_summary(data)
    return [entry["hero_id"] for entry in roster]

