"""Dungeon Crawler Basic — hero/actor panel adapter.

HeroPanelWidget keeps the public Dungeon Crawler API but delegates rendering to
the shared :class:`gmGui.modules.gm_actor_module.GmActorModule` widget so the
actor panel reuses the existing GameLib GUI look and data presentation.
"""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QSizePolicy, QVBoxLayout, QWidget

from gmGui.modules.gm_actor_module import GmActorModule


class HeroPanelWidget(QWidget):
    """Adapter widget that maps dungeon actor events onto gmActor events.

    Signals:
        actor_selected(actor_id): forwarded from the embedded GmActorModule tree.
    """

    actor_selected: Signal = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self._layout = QVBoxLayout(self)
        self._layout.setContentsMargins(4, 4, 4, 4)
        self._layout.setSpacing(4)
        self._module: GmActorModule = GmActorModule()
        self._module_widget = self._module.widget()
        self._layout.addWidget(self._module_widget)
        self._actor_cache: dict[str, dict] = {}
        self._module.on_actor_selected = self.actor_selected.emit

    def on_envelope(self, msg: dict) -> None:
        """Receives a dungeon envelope and forwards the translated actor event."""
        type_id = str(msg.get("typeId", ""))
        data = msg.get("data", {})

        if type_id == "dungeon.session.started":
            self.reset()
            return

        if type_id == "dungeon.actor.snapshot":
            self._module.on_envelope(
                {"typeId": "gmActor.snapshot", "data": {"actors": self._translate_snapshot(data)}}
            )
        elif type_id == "dungeon.actor.hp_changed":
            self._module.on_envelope(self._translate_hp_changed(data))
        elif type_id == "dungeon.actor.status_changed":
            translated = self._translate_status_changed(data)
            if translated is not None:
                self._module.on_envelope(translated)
        elif type_id == "dungeon.actor.equipped":
            translated = self._translate_equipped(data)
            if translated is not None:
                self._module.on_envelope(translated)
        elif type_id == "dungeon.turn.started":
            actor_id = str(data.get("actor_id", ""))
            actions_remaining = int(data.get("actions_remaining", 2))
            if actor_id:
                self._current_turn_actor: str = actor_id
                self._emit_actions_resource(actor_id, actions_remaining)
        elif type_id == "dungeon.actor.moved":
            translated = self._translate_moved(data)
            if translated is not None:
                self._module.on_envelope(translated)

    def select_actor(self, actor_id: str) -> None:
        """Programmatically selects *actor_id* in the embedded actor tree."""
        self._module.select_actor(actor_id)

    def update_actions_remaining(self, actor_id: str, remaining: int) -> None:
        """Updates the 'azioni' resource for *actor_id* in the Risorse section."""
        self._emit_actions_resource(actor_id, remaining)

    def _emit_actions_resource(self, actor_id: str, remaining: int) -> None:
        self._module.on_envelope({
            "typeId": "gmActor.actor.resource_changed",
            "data": {"actor_id": actor_id, "resource_id": "azioni",
                     "new_value": remaining},
        })

    def reset(self) -> None:
        """Resets the embedded module to a clean actor state."""
        self._actor_cache.clear()
        self._module = GmActorModule()
        self._layout.removeWidget(self._module_widget)
        self._module_widget.setParent(None)
        self._module_widget.deleteLater()
        self._module_widget = self._module.widget()
        self._layout.addWidget(self._module_widget)
        self._module.on_actor_selected = self.actor_selected.emit

    def _translate_snapshot(self, data: dict) -> list[dict]:
        actors: list[dict] = []
        for actor in data.get("actors", []):
            actor_id = str(actor.get("id", ""))
            if not actor_id:
                continue
            current_hp = int(actor.get("hp", 0))
            max_hp = int(actor.get("max_hp", 1))
            tags = [str(tag) for tag in actor.get("tags", [])]
            status_map: dict[str, int] = {}
            for status_id in actor.get("statuses", []):
                status_map[str(status_id)] = 1

            entry = {
                "actor_id": actor_id,
                "faction_id": self._faction_for_kind(str(actor.get("kind", ""))),
                "name": actor_id,
                "current_hp": current_hp,
                "max_hp": max_hp,
                "life_state": "DEAD" if current_hp <= 0 else "ALIVE",
                "statuses": status_map,
                "equipment": self._equipment_from_tags(tags),
                "area_id": str(actor.get("location", "")),
            }
            self._actor_cache[actor_id] = entry
            actors.append(entry)
        return actors

    def _translate_hp_changed(self, data: dict) -> dict:
        actor_id = str(data.get("actor_id", ""))
        cached = self._actor_cache.get(actor_id, {})
        return {
            "typeId": "gmActor.actor.hp_changed",
            "data": {
                "actor_id": actor_id,
                "new_hp": int(data.get("hp_after", data.get("new_hp", cached.get("current_hp", 0)))),
                "max_hp": int(data.get("max_hp", cached.get("max_hp", 1))),
            },
        }

    def _translate_status_changed(self, data: dict) -> dict | None:
        actor_id = str(data.get("actor_id", ""))
        status_id = str(data.get("status_id", ""))
        if not actor_id or not status_id:
            return None
        added = bool(data.get("added", True))
        return {
            "typeId": "gmActor.actor.status_added" if added else "gmActor.actor.status_removed",
            "data": {"actor_id": actor_id, "status_id": status_id, "stacks": 1},
        }

    def _translate_equipped(self, data: dict) -> dict | None:
        actor_id = str(data.get("actor_id", ""))
        item_tag = str(data.get("item_tag", ""))
        if not actor_id or not item_tag:
            return None
        return {
            "typeId": "gmActor.actor.item_equipped",
            "data": {"actor_id": actor_id, "slot": "weapon", "item_instance_id": item_tag},
        }

    def _translate_moved(self, data: dict) -> dict | None:
        actor_id = str(data.get("actor_id", ""))
        destination = str(data.get("to", ""))
        if not actor_id or not destination:
            return None
        return {
            "typeId": "gmActor.actor.moved_area",
            "data": {"actor_id": actor_id, "new_area_id": destination},
        }

    @staticmethod
    def _faction_for_kind(kind: str) -> str:
        kind = kind.upper()
        if kind == "HERO":
            return "heroes"
        if kind in ("MONSTER", "MONSTER_ELITE", "BOSS_MONSTER"):
            return "enemies"
        return "neutral"

    @staticmethod
    def _equipment_from_tags(tags: list[str]) -> dict[str, str]:
        equipment: dict[str, str] = {}
        if "bigword_available" in tags:
            equipment["weapon"] = "bigword_available"
        return equipment
