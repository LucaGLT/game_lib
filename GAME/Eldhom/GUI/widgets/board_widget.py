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
import sys
from pathlib import Path

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QSizePolicy, QVBoxLayout, QWidget

_GUI_DIR   = Path(__file__).resolve().parent.parent / "app"
_PYLIB_DIR = Path(__file__).resolve().parents[3] / "pyLib"
for _p in (str(_GUI_DIR), str(_PYLIB_DIR)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from gmGui.modules.gm_map_module import GmMapModule       # noqa: E402
from gmGui.widgets.map_scene import LocationNode          # noqa: E402


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
            edges_out: list[list[int]] = []
            seen_edges: set[tuple[int, int]] = set()

            for loc in data.get("locations", []):
                lid  = str(loc.get("id", ""))
                if not lid:
                    continue
                idx  = self._loc_idx(lid)
                name = loc.get("name", lid)
                for adj in loc.get("adjacent", []):
                    a_idx = self._loc_idx(str(adj))
                    edge  = tuple(sorted((idx, a_idx)))
                    if edge not in seen_edges:
                        edges_out.append([edge[0], edge[1]])
                        seen_edges.add(edge)
                locations_out.append({
                    "location_id": idx,
                    "tags":        ["location"],
                    "metadata":    {
                        "terrain": "stone",
                        "items":   [name],
                    },
                })

            map_data: dict = {"locations": locations_out, "edges": edges_out}
            self._module.on_envelope({
                "typeId":  "gmMap.map.loaded",
                "headers": {"data": _json.dumps(map_data)},
                "data":    map_data,
            })
            self._map_built = True

        # ── Register actor labels ────────────────────────────────────────────
        label_map: dict[str, str] = {}
        for hero in data.get("heroes", []):
            label_map[str(hero["id"])] = hero.get("name", str(hero["id"]))
        for grp in data.get("groups", []):
            for inst in grp.get("instances", []):
                label_map[str(inst["id"])] = str(inst.get("id", "?"))
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
