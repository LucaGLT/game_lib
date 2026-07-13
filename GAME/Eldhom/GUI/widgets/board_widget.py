"""Le Pergamene di Eldhôm — board widget.

EldhomBoardWidget wraps :class:`~gmGui.modules.gm_map_module.GmMapModule`
and translates Eldhôm-specific events into the generic gmMap / gmActor events
that the shared module understands.

Public API
----------
on_state_full(msg)      Build / rebuild the entire map from a full state snapshot.
on_pg_moved(msg)        Move a hero token to its new location.
on_monster_defeated(msg) Grey-out a defeated monster token on the map.

Signal
------
area_selected(str)      Emitted when the player clicks a location node.
                        Payload: location id string.
"""
from __future__ import annotations

import json as _json
import re as _re
import sys
from pathlib import Path

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QSizePolicy, QVBoxLayout, QWidget

_GUI_DIR   = Path(__file__).resolve().parent.parent / "app"
_PYLIB_DIR = Path(__file__).resolve().parents[3] / "pyLib"
_DATA_DIR  = Path(__file__).resolve().parents[2] / "data"
for _p in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from gmGui.modules.gm_map_module import GmMapModule       # noqa: E402
from gmGui.widgets.map_scene import LocationNode          # noqa: E402


def _zone_from_loc_id(lid: str) -> str:
    """Zone prefix = location ID stripped of trailing digits.

    Examples: ``"S1"``→``"S"``, ``"C3"``→``"C"``, ``"IN"``→``"IN"``.
    """
    return _re.sub(r'\d+$', '', lid) or lid


def _monster_prefix(monster_type: str) -> str:
    """Derives a short display prefix from a monster type string.

    Examples: ``"guardiano_base"``→``"G"``, ``"guardiano_elite"``→``"GE"``.
    """
    words = [w for w in monster_type.lower().split("_") if w]
    if not words:
        return "M"
    first = words[0][0].upper()
    if "elite" in words or "boss" in words:
        return first + "E"
    return first


def _extract_locked_pairs_from_special_objects(sobjs: list[dict]) -> set[tuple[str, str]]:
    """Extracts locked adjacency pairs from special object payloads.

    Accepts all known key variants used in mission JSONs:
    - ``locked_adjacency`` (runtime full-state payload)
    - ``on_interact.adjacency``
    - ``on_interact.adjacency_unlock``
    - ``on_interact.unlock_adjacency``
    """
    locked_pairs: set[tuple[str, str]] = set()
    for sobj in sobjs:
        pair_sources: list = []
        raw_locked = sobj.get("locked_adjacency", [])
        if isinstance(raw_locked, list):
            pair_sources.append(raw_locked)

        on_interact = sobj.get("on_interact", {})
        if isinstance(on_interact, dict):
            for key in ("adjacency", "adjacency_unlock", "unlock_adjacency"):
                raw = on_interact.get(key, [])
                if isinstance(raw, list):
                    pair_sources.append(raw)

        for src in pair_sources:
            for pair in src:
                if isinstance(pair, list) and len(pair) == 2:
                    la, lb = str(pair[0]), str(pair[1])
                    if la and lb:
                        locked_pairs.add((la, lb))
                        locked_pairs.add((lb, la))
    return locked_pairs


def _load_locked_pairs_from_mission_file(mission_id: str) -> set[tuple[str, str]]:
    """Loads locked adjacency from the mission JSON as GUI fallback.

    This is used when older engine binaries do not yet expose
    ``state["special_objects"]`` in ``eldhom.state.full``.
    """
    if not mission_id:
        return set()

    candidate_files: list[Path] = [
        _DATA_DIR / f"{mission_id}.json",
        _DATA_DIR / f"{mission_id.replace('missione_', 'mission_')}.json",
    ]

    for path in candidate_files:
        if not path.exists():
            continue
        try:
            payload = _json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            continue
        if isinstance(payload, dict):
            sobjs = payload.get("special_objects", [])
            if isinstance(sobjs, list):
                return _extract_locked_pairs_from_special_objects(sobjs)
    return set()


def _monster_label_prefix_from_payload(group_payload: dict, inst_id: str) -> str:
    """Builds a short monster prefix using best available metadata.

    Priority:
    1) ``monster_type`` from the group payload (preferred)
    2) Heuristics from ids/names: contains "elite" -> GE, "guard" -> G
    3) Generic fallback "M"
    """
    mtype = str(group_payload.get("monster_type", group_payload.get("type", "")))
    prefix = _monster_prefix(mtype)
    if prefix != "M":
        return prefix

    probe = " ".join([
        inst_id,
        str(group_payload.get("id", "")),
        str(group_payload.get("name", "")),
    ]).lower()
    if "elite" in probe or "boss" in probe:
        return "GE"
    if "guard" in probe or "guardiano" in probe:
        return "G"
    return "M"


class EldhomBoardWidget(QWidget):
    """Adapter widget that maps Eldhôm map events onto :class:`GmMapModule`.

    Eldhôm uses *string* location IDs (e.g. ``"foresta"``, ``"villaggio"``).
    ``GmMapModule`` / ``MapScene`` require integer indices.  This class
    maintains a ``str → int`` mapping and converts transparently.
    """

    area_selected: Signal = Signal(str)   # location_id string

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self._outer_layout = QVBoxLayout(self)
        self._outer_layout.setContentsMargins(0, 0, 0, 0)
        self._outer_layout.setSpacing(0)

        self._loc_index: dict[str, int] = {}      # loc_id  → integer index
        self._actor_locs: dict[str, str] = {}     # actor_id → loc_id
        self._map_built: bool = False             # map structure built once

        self._module: GmMapModule = GmMapModule()
        self._module_widget: QWidget = self._module.widget()
        self._outer_layout.addWidget(self._module_widget)

        # Override the module's built-in selection handler so we can emit our own signal.
        try:
            self._module._map_scene.selectionChanged.disconnect(
                self._module._on_selection_changed
            )
        except Exception:
            pass
        self._module._map_scene.selectionChanged.connect(self._on_scene_selection)

    # ── Public API ─────────────────────────────────────────────────────────────

    def on_state_full(self, msg: dict) -> None:
        """Rebuilds the map and places all actor tokens from a full state snapshot.

        The map structure (locations + edges) is built only once so that the
        force-directed layout is not reset on every refresh; subsequent
        snapshots only re-place the actor tokens at their current locations.
        """
        data = _extract_data(msg)

        incoming_locs = {
            str(loc.get("id", ""))
            for loc in data.get("locations", [])
            if loc.get("id", "")
        }
        # Rebuild the structure only when the map actually changes (first load
        # or a new mission), to avoid resetting the force-directed layout.
        if not self._map_built or incoming_locs != set(self._loc_index.keys()):
            self._loc_index.clear()
            self._map_built = False

        if not self._map_built:

            # ── Build location + edge lists ──────────────────────────────────
            locations_out: list[dict] = []
            edges_out: list[list] = []     # each: [idx_a, idx_b, edge_type]
            seen_edges: set[tuple[int, int]] = set()

            # Pre-compute zone for each location (strip trailing digits)
            loc_zones: dict[str, str] = {
                str(loc.get("id", "")): _zone_from_loc_id(str(loc.get("id", "")))
                for loc in data.get("locations", [])
                if loc.get("id", "")
            }

            # Collect locked-adjacency pairs from runtime payload; fallback to
            # mission file for older engines that do not expose special_objects.
            locked_pairs = _extract_locked_pairs_from_special_objects(
                list(data.get("special_objects", [])))
            if not locked_pairs:
                locked_pairs = _load_locked_pairs_from_mission_file(
                    str(data.get("mission_id", "")))

            for loc in data.get("locations", []):
                lid  = str(loc.get("id", ""))
                if not lid:
                    continue
                idx  = self._loc_idx(lid)
                zone = loc_zones.get(lid, "")
                region = "A" if lid == "IN" else ("B" if lid == "OUT" else "C")
                for adj in loc.get("adjacent", []):
                    a_lid = str(adj)
                    a_idx = self._loc_idx(a_lid)
                    edge  = tuple(sorted((idx, a_idx)))
                    if edge not in seen_edges:
                        seen_edges.add(edge)
                        adj_zone  = loc_zones.get(a_lid, "")
                        edge_type = "FREE" if zone == adj_zone else "CLOSED_DOOR"
                        edges_out.append([edge[0], edge[1], edge_type])
                locations_out.append({
                    "location_id": idx,
                    "label":       lid,
                    "zone_id":     zone,
                    "region_id":   region,
                    "tags":        ["location"],
                    "metadata":    {"terrain": "stone", "items": []},
                })

            # Add LOCKED_DOOR edges from special_objects (not in regular adjacency)
            for la, lb in locked_pairs:
                if la > lb:   # process each undirected pair once
                    continue
                # Ensure both locations are registered
                idx_a = self._loc_idx(la)
                idx_b = self._loc_idx(lb)
                edge  = tuple(sorted((idx_a, idx_b)))
                if edge not in seen_edges:
                    seen_edges.add(edge)
                    edges_out.append([edge[0], edge[1], "LOCKED_DOOR"])

            map_data: dict = {"locations": locations_out, "edges": edges_out}
            self._module.on_envelope({
                "typeId":  "gmMap.map.loaded",
                "headers": {"data": _json.dumps(map_data)},
                "data":    map_data,
            })
            self._map_built = True

        # ── Register actor labels ────────────────────────────────────────────
        label_map: dict[str, str] = {}
        pg_counter: int = 1
        for hero in data.get("heroes", []):
            label_map[str(hero["id"])] = f"PG{pg_counter}"
            pg_counter += 1
        monster_prefix_counters: dict[str, int] = {}
        for grp in data.get("groups", []):
            prefix = _monster_label_prefix_from_payload(grp, "")
            for inst in grp.get("instances", []):
                inst_id = str(inst.get("id", ""))
                inst_prefix = _monster_label_prefix_from_payload(grp, inst_id)

                # Preserve explicit numeric suffix when present in instance id.
                suffix_match = _re.search(r"(\d+)$", inst_id)
                if suffix_match:
                    label_map[inst_id] = f"{inst_prefix}{suffix_match.group(1)}"
                    continue

                monster_prefix_counters[inst_prefix] = (
                    monster_prefix_counters.get(inst_prefix, 0) + 1)
                label_map[inst_id] = (
                    f"{inst_prefix}{monster_prefix_counters[inst_prefix]}")
        if label_map:
            self._module._map_scene.register_actor_labels(label_map)

        # ── Place actors at their current locations ──────────────────────────
        for hero in data.get("heroes", []):
            self._place_actor(str(hero["id"]), str(hero.get("location", "")))
        for grp in data.get("groups", []):
            for inst in grp.get("instances", []):
                self._place_actor(str(inst["id"]), str(inst.get("location", "")))

    def on_pg_moved(self, msg: dict) -> None:
        """Moves a hero token to its new location when ``eldhom.pg.moved`` arrives.

        The engine forwards the destination location id in the ``payload`` field.
        """
        data        = _extract_data(msg)
        actor_id    = str(data.get("actor_id", ""))
        destination = str(
            data.get("payload", data.get("destination", data.get("to", "")))
        )
        if actor_id and destination:
            self._actor_locs[actor_id] = destination
            if destination in self._loc_index:
                self._move_on_map(actor_id, destination)

    def on_monster_defeated(self, msg: dict) -> None:
        """No visual removal (GmMapModule has no remove API); token stays in place."""
        pass

    # ── Internal helpers ───────────────────────────────────────────────────────

    def _loc_idx(self, loc_id: str) -> int:
        """Returns (creating if needed) an integer index for *loc_id*."""
        if loc_id not in self._loc_index:
            self._loc_index[loc_id] = len(self._loc_index)
        return self._loc_index[loc_id]

    def _place_actor(self, actor_id: str, loc_id: str) -> None:
        """Sends a position_changed event to the map module for *actor_id*."""
        if not actor_id or not loc_id:
            return
        idx = self._loc_idx(loc_id)
        self._actor_locs[actor_id] = loc_id
        payload: dict = {"actor_id": actor_id, "new_location_id": idx}
        self._module.on_envelope({
            "typeId":  "gmActor.actor.position_changed",
            "headers": {"data": _json.dumps(payload)},
            "data":    payload,
        })

    def _move_on_map(self, actor_id: str, loc_id: str) -> None:
        """Sends a moved_area event to the map module."""
        idx: int = self._loc_idx(loc_id)
        payload: dict = {"actor_id": actor_id, "new_area_id": idx}
        self._module.on_envelope({
            "typeId":  "gmActor.actor.moved_area",
            "headers": {"data": _json.dumps(payload)},
            "data":    payload,
        })

    def _on_scene_selection(self) -> None:
        """Translates a MapScene selection back to a string location ID."""
        items = self._module._map_scene.selectedItems()
        for item in items:
            if isinstance(item, LocationNode):
                # Reverse-lookup integer index → string id
                for lid, idx in self._loc_index.items():
                    if idx == item.loc_id:
                        self.area_selected.emit(lid)
                        return


def _extract_data(msg: dict) -> dict:
    """Returns the payload dict from an event envelope."""
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    try:
        return _json.loads(raw)
    except Exception:
        return msg.get("data", {})
