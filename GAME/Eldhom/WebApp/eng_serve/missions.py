"""missions — server-side scan of the mission JSON files in ``data_dir``.

Replaces the desktop GUI's local-filesystem scan
(``GAME/Eldhom/GUI/app/mission_select_dialog.py::_scan_missions``), which
cannot run in the browser (the SPA has no access to the server's disk).

Reads each mission file's own ``mission_id``/``title``/``description``
fields rather than deriving ``mission_id`` from the filename: the engine's
``MissionLoader`` applies its own filename transform (e.g. mission_id
``"missione_sim_a"`` -> file ``mission_sim_a.json``), so trusting the file's
declared id avoids re-implementing that mapping here.
"""
from __future__ import annotations

import json
from pathlib import Path


def scan_missions(data_dir: Path) -> list[dict]:
    """Reads ``mission_*.json`` files under *data_dir* into summary dicts.

    Args:
        data_dir: Directory to scan (same one passed to the engine as its
            data-dir CLI argument).

    Returns:
        A list of ``{"mission_id", "title", "description"}`` dicts, one per
        readable mission file (unreadable/malformed files are skipped
        silently, same tolerance as the desktop dialog).
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
            }
        )
    return result
