"""Le Pergamene di Eldhôm — actor panel adapter.

EldhomActorAdapter wraps the generic :class:`gmGui.modules.gm_actor_module.GmActorModule`
and translates ``eldhom.*`` events into the ``gmActor.*`` schema that module expects.

Only the event type IDs and field names differ from the Dungeon Crawler adapter;
all logic and rendering are fully delegated to GmActorModule without modification.
"""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QSizePolicy, QVBoxLayout, QWidget

from gmGui.modules.gm_actor_module import GmActorModule


class EldhomActorAdapter(QWidget):
    """Thin translation layer mapping ``eldhom.*`` events onto GmActorModule.

    Signals:
        actor_selected (str): forwarded from the embedded GmActorModule tree
            when the user clicks an actor row.
    """

    actor_selected: Signal = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        """Creates the adapter and embeds a GmActorModule."""
        super().__init__(parent)
        self.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding
        )
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        self._module: GmActorModule = GmActorModule()
        layout.addWidget(self._module.widget())
        self._module.on_actor_selected = self.actor_selected.emit
        self._cache: dict[str, dict] = {}

    # ── Public API ────────────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        """Receives an Eldhom engine envelope and forwards translated events.

        Args:
            msg: Decoded event dict with ``typeId`` and ``data`` keys.
        """
        type_id = str(msg.get("typeId", ""))
        data = msg.get("data", {})

        if type_id == "eldhom.state.full":
            self._on_state_full(data)
        elif type_id in ("eldhom.pg.ko", "eldhom.pg.healed"):
            self._on_hp_changed(data, ko=(type_id == "eldhom.pg.ko"))
        elif type_id == "eldhom.monster.damaged":
            self._on_monster_damaged(data)
        elif type_id == "eldhom.monster.defeated":
            self._on_monster_defeated(data)
        elif type_id == "eldhom.pg.moved":
            self._on_pg_moved(data)
        elif type_id == "eldhom.mission.time_advanced":
            self._on_time_advanced(data)

    def select_actor(self, actor_id: str) -> None:
        """Programmatically selects *actor_id* in the embedded actor tree.

        Args:
            actor_id: ID of the actor to highlight.
        """
        self._module.select_actor(actor_id)

    # ── Internal translators ──────────────────────────────────────────────────

    def _on_state_full(self, data: dict) -> None:
        actors: list[dict] = []
        self._cache.clear()
        for hero in data.get("heroes", []):
            entry = self._hero_to_actor(hero)
            if entry is not None:
                self._cache[entry["actor_id"]] = entry
                actors.append(entry)
        for group in data.get("groups", []):
            entry = self._group_to_actor(group)
            if entry is not None:
                self._cache[entry["actor_id"]] = entry
                actors.append(entry)
        self._module.on_envelope(
            {"typeId": "gmActor.snapshot", "data": {"actors": actors}}
        )

    def _on_hp_changed(self, data: dict, *, ko: bool) -> None:
        actor_id = str(data.get("actor_id", data.get("hero_id", "")))
        if not actor_id:
            return
        cached = self._cache.get(actor_id, {})
        new_hp = int(
            data.get("hp_after", data.get("hp", cached.get("current_hp", 0)))
        )
        max_hp = int(data.get("max_hp", cached.get("max_hp", 1)))
        if actor_id in self._cache:
            self._cache[actor_id]["current_hp"] = new_hp
        self._module.on_envelope(
            {
                "typeId": "gmActor.actor.hp_changed",
                "data": {"actor_id": actor_id, "new_hp": new_hp, "max_hp": max_hp},
            }
        )
        if ko:
            self._module.on_envelope(
                {
                    "typeId": "gmActor.actor.life_state_changed",
                    "data": {"actor_id": actor_id, "new_state": "KO"},
                }
            )

    def _on_monster_damaged(self, data: dict) -> None:
        actor_id = str(data.get("group_id", data.get("actor_id", "")))
        if not actor_id:
            return
        cached = self._cache.get(actor_id, {})
        new_hp = int(data.get("hp_after", cached.get("current_hp", 0)))
        max_hp = int(data.get("max_hp", cached.get("max_hp", 1)))
        if actor_id in self._cache:
            self._cache[actor_id]["current_hp"] = new_hp
        self._module.on_envelope(
            {
                "typeId": "gmActor.actor.hp_changed",
                "data": {"actor_id": actor_id, "new_hp": new_hp, "max_hp": max_hp},
            }
        )

    def _on_monster_defeated(self, data: dict) -> None:
        actor_id = str(data.get("group_id", data.get("actor_id", "")))
        if not actor_id:
            return
        self._cache.pop(actor_id, None)
        self._module.on_envelope(
            {"typeId": "gmActor.actor.removed", "data": {"actor_id": actor_id}}
        )

    def _on_pg_moved(self, data: dict) -> None:
        actor_id = str(data.get("hero_id", data.get("actor_id", "")))
        destination = str(
            data.get("payload", data.get("to", data.get("destination", "")))
        )
        if actor_id and destination:
            self._module.on_envelope(
                {
                    "typeId": "gmActor.actor.moved_area",
                    "data": {"actor_id": actor_id, "new_area_id": destination},
                }
            )

    def _on_time_advanced(self, data: dict) -> None:
        actor_id = str(data.get("actor_id", ""))
        timeline_val = int(data.get("payload", data.get("time", 0)))
        if actor_id:
            self._module.on_envelope(
                {
                    "typeId": "gmActor.actor.resource_changed",
                    "data": {
                        "actor_id": actor_id,
                        "resource_id": "timeline",
                        "new_value": timeline_val,
                    },
                }
            )

    # ── Static translators ────────────────────────────────────────────────────

    @staticmethod
    def _hero_to_actor(hero: dict) -> dict | None:
        actor_id = str(hero.get("id", ""))
        if not actor_id:
            return None
        hp = int(hero.get("hp", 0))
        max_hp = max(int(hero.get("max_hp", 6)), 1)
        return {
            "actor_id":   actor_id,
            "faction_id": "heroes",
            "name":       str(hero.get("name", actor_id)),
            "current_hp": hp,
            "max_hp":     max_hp,
            "life_state": "KO" if hp <= 0 else "ALIVE",
            "statuses":   {s: 1 for s in hero.get("statuses", [])},
            "equipment":  EldhomActorAdapter._equipment_from_list(
                hero.get("equipment", [])
            ),
            "area_id":    str(hero.get("location", "")),
            "resources":  {"timeline": int(hero.get("timeline", 0))},
        }

    @staticmethod
    def _group_to_actor(group: dict) -> dict | None:
        actor_id = str(group.get("id", ""))
        if not actor_id:
            return None
        hp = int(group.get("hp", 0))
        max_hp = max(int(group.get("max_hp", 1)), 1)
        return {
            "actor_id":   actor_id,
            "faction_id": "enemies",
            "name":       str(group.get("name", actor_id)),
            "current_hp": hp,
            "max_hp":     max_hp,
            "life_state": "DEAD" if hp <= 0 else "ALIVE",
            "statuses":   {},
            "equipment":  {},
            "area_id":    str(group.get("location", "")),
            "resources":  {"timeline": int(group.get("timeline", 0))},
        }

    @staticmethod
    def _equipment_from_list(items: list) -> dict[str, str]:
        result: dict[str, str] = {}
        for item in items:
            if isinstance(item, dict):
                slot = str(item.get("slot", "item"))
                item_id = str(item.get("item_id", item.get("id", "")))
                if item_id:
                    result[slot] = item_id
            elif isinstance(item, str) and item:
                result[item] = item
        return result
