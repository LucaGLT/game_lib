"""cards — server-side scan of the card catalog JSON files in ``data_dir``.

Replaces the desktop GUI's local-filesystem scan
(``GAME/Eldhom/GUI/app/eldhom_main_window.py::_load_card_catalog``), which
cannot run in the browser. Returns the raw card dicts as-is (same shape
already used by ``cards_base.json``/``cards_mission_*.json``) — eng_serve
does not interpret or reshape card fields, matching its pass-through design
(see GAME/Eldhom/WebApp/PLAN.md, Key Design Decision 6).
"""
from __future__ import annotations

import json
from pathlib import Path


def scan_cards(data_dir: Path) -> list[dict]:
    """Reads every ``cards_*.json`` file under *data_dir* into one flat list.

    Args:
        data_dir: Directory to scan (same one passed to the engine as its
            data-dir CLI argument).

    Returns:
        A list of raw card dicts, deduplicated by ``card_id`` (first file
        wins) — same tolerance as the desktop's ``_load_card_catalog``.
    """
    seen: set[str] = set()
    result: list[dict] = []
    if not data_dir.is_dir():
        return result
    for path in sorted(data_dir.glob("cards_*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            continue
        if not isinstance(data, list):
            continue
        for card in data:
            if not isinstance(card, dict):
                continue
            card_id = str(card.get("card_id", ""))
            if card_id == "" or card_id in seen:
                continue
            seen.add(card_id)
            result.append(card)
    return result
