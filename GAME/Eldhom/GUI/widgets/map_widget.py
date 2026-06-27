"""Le Pergamene di Eldhom — map widget.

EldhomMapWidget shows the 3 mission locations arranged horizontally.
Each location column lists the actors present (PG and monster instances)
with simple labels showing HP, position (PL/RG) and life state.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QFrame,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt


class _ActorToken(QLabel):
    """Single-line label representing one actor in a location column."""

    HERO_STYLE      = "color:#c8a060; font-weight:bold; padding:2px 4px;"
    MONSTER_STYLE   = "color:#e05050; padding:2px 4px;"
    KO_STYLE        = "color:#888888; font-style:italic; padding:2px 4px;"

    def __init__(self, actor_id: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._actor_id = actor_id
        self.setWordWrap(False)
        self.update_display("?", 0, 0, "FRONTLINE", True, False)

    def update_display(
        self,
        name: str,
        hp: int,
        max_hp: int,
        position: str,
        alive: bool,
        is_hero: bool,
    ) -> None:
        """Refreshes the label text and style."""
        pos_tag = "PL" if position == "FRONTLINE" else "RG"
        if not alive:
            self.setText(f"✗ {name} [{pos_tag}]")
            self.setStyleSheet(self.KO_STYLE)
        else:
            self.setText(f"{'★' if is_hero else '•'} {name} [{pos_tag}] {hp}/{max_hp}❤")
            self.setStyleSheet(self.HERO_STYLE if is_hero else self.MONSTER_STYLE)


class _LocationColumn(QGroupBox):
    """One vertical column for a single location."""

    def __init__(self, loc_id: str, loc_name: str, parent: QWidget | None = None) -> None:
        super().__init__(loc_name, parent)
        self._loc_id   = loc_id
        self._tokens: dict[str, _ActorToken] = {}
        self._layout   = QVBoxLayout(self)
        self._layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self._layout.setSpacing(2)
        self.setMinimumWidth(180)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setStyleSheet(
            "QGroupBox { border:1px solid #555; border-radius:4px;"
            " margin-top:8px; padding:4px; }"
            " QGroupBox::title { color:#aaa; subcontrol-origin:margin;"
            " subcontrol-position:top left; padding:0 4px; }"
        )

    def set_actors(self, actors: list[dict]) -> None:
        """Rebuilds the column from a list of actor dicts."""
        # Remove existing tokens not in actors
        ids_present = {a["id"] for a in actors}
        for aid in list(self._tokens.keys()):
            if aid not in ids_present:
                self._tokens[aid].deleteLater()
                del self._tokens[aid]

        for actor in actors:
            aid       = actor["id"]
            name      = actor.get("name", aid)
            hp        = actor.get("hp", 0)
            max_hp    = actor.get("max_hp", 1)
            position  = actor.get("position", "FRONTLINE")
            alive     = actor.get("alive", True)
            is_hero   = actor.get("is_hero", False)

            if aid not in self._tokens:
                token = _ActorToken(aid, self)
                self._tokens[aid] = token
                self._layout.addWidget(token)

            self._tokens[aid].update_display(name, hp, max_hp, position, alive, is_hero)


class EldhomMapWidget(QFrame):
    """Displays all mission locations and their actor tokens.

    Updated by calling :meth:`on_state_full` whenever a full state snapshot
    arrives, or by individual event handlers.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._columns: dict[str, _LocationColumn] = {}
        self._location_order: list[str] = []
        self._actor_index: dict[str, dict] = {}   # actor_id → actor info

        layout = QHBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(4, 4, 4, 4)
        self._layout = layout
        self.setFrameShape(QFrame.Shape.StyledPanel)

    # ── Public API ─────────────────────────────────────────────────────────────

    def on_state_full(self, msg: dict) -> None:
        """Rebuilds the entire map from a full state snapshot."""
        data = _extract_data(msg)

        # Rebuild columns from location list
        locations = data.get("locations", [])
        new_ids   = [loc["id"] for loc in locations]

        # Remove stale columns
        for lid in list(self._columns.keys()):
            if lid not in new_ids:
                self._columns[lid].deleteLater()
                del self._columns[lid]

        # Add new / reorder
        self._location_order = new_ids
        for loc in locations:
            lid = loc["id"]
            if lid not in self._columns:
                col = _LocationColumn(lid, loc.get("name", lid), self)
                self._columns[lid] = col
                self._layout.addWidget(col)

        # Build actor index
        self._actor_index.clear()
        for hero in data.get("heroes", []):
            self._actor_index[hero["id"]] = {**hero, "is_hero": True}
        for grp in data.get("groups", []):
            for inst in grp.get("instances", []):
                self._actor_index[inst["id"]] = {**inst, "is_hero": False,
                                                 "name": inst.get("id", "?")}

        self._refresh_columns()

    def on_hero_update(self, msg: dict) -> None:
        """Updates a single hero's token."""
        data = _extract_data(msg)
        hero_id = data.get("actor_id", "")
        if hero_id in self._actor_index:
            for key in ("hp", "max_hp", "position", "location", "timeline"):
                if key in data:
                    self._actor_index[hero_id][key] = data[key]
            self._refresh_columns()

    def on_instance_update(self, msg: dict) -> None:
        """Updates a monster instance token."""
        data = _extract_data(msg)
        inst_id = data.get("actor_id", "")
        if inst_id in self._actor_index:
            payload = data.get("payload", {})
            if isinstance(payload, dict):
                for key in ("hp", "max_hp", "position", "location"):
                    if key in payload:
                        self._actor_index[inst_id][key] = payload[key]
            self._refresh_columns()

    def on_monster_defeated(self, msg: dict) -> None:
        """Marks a monster instance as dead."""
        data    = _extract_data(msg)
        inst_id = data.get("actor_id", "")
        if inst_id in self._actor_index:
            self._actor_index[inst_id]["alive"] = False
            self._refresh_columns()

    # ── Internal ──────────────────────────────────────────────────────────────

    def _refresh_columns(self) -> None:
        """Distributes actors to their location column."""
        # Build per-location actor lists
        by_location: dict[str, list[dict]] = {lid: [] for lid in self._location_order}
        for actor in self._actor_index.values():
            loc = actor.get("location", "")
            if loc in by_location:
                by_location[loc].append(actor)

        for lid, col in self._columns.items():
            col.set_actors(by_location.get(lid, []))


def _extract_data(msg: dict) -> dict:
    """Returns the data payload from an event dict."""
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    import json
    try:
        return json.loads(raw)
    except Exception:
        return {}
