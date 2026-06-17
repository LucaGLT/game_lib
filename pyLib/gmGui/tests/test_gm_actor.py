"""Tests for GmActorModule and HpBar — Phase 5 implementation.

All tests run in offscreen mode (no display required).
The ``mod`` fixture builds a fresh ``GmActorModule`` with its widget
constructed, isolating each test from state leakage.
"""
from __future__ import annotations

import os
import sys

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_actor_module import GmActorModule
from gmGui.widgets.hp_bar import HpBar


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Per-test module fixture ───────────────────────────────────────────────────

@pytest.fixture
def mod(qapp: QApplication) -> GmActorModule:
    """Fresh GmActorModule with widget already built."""
    m = GmActorModule()
    m.widget()
    return m


# ── Test helpers ──────────────────────────────────────────────────────────────

def _snapshot(
    actor_id: str = "hero_1",
    name: str = "Hero",
    faction_id: str = "Eroi",
    current_hp: int = 100,
    max_hp: int = 100,
    life_state: str = "ALIVE",
    statuses: dict | None = None,
    equipment: dict | None = None,
) -> dict:
    return {
        "typeId": "gmActor.snapshot",
        "data": {
            "actors": [{
                "actor_id": actor_id,
                "name": name,
                "faction_id": faction_id,
                "current_hp": current_hp,
                "max_hp": max_hp,
                "life_state": life_state,
                "statuses": statuses or {},
                "equipment": equipment or {},
                "area_id": "",
            }]
        },
    }


def _select(mod: GmActorModule, actor_id: str) -> None:
    """Programmatically selects the tree item for *actor_id*."""
    mod._tree.setCurrentItem(mod._actor_items[actor_id])


# ── Snapshot tests ────────────────────────────────────────────────────────────

def test_snapshot_creates_actor_items(mod: GmActorModule) -> None:
    """gmActor.snapshot populates _actor_items for every actor in the payload."""
    mod.on_envelope(_snapshot("hero_1", "Aragorn"))

    assert "hero_1" in mod._actor_items


def test_snapshot_creates_faction_groups(mod: GmActorModule) -> None:
    """gmActor.snapshot creates a faction root item in the tree."""
    mod.on_envelope(_snapshot("hero_1", faction_id="Eroi"))

    assert "Eroi" in mod._faction_items


def test_snapshot_populates_tree_columns(mod: GmActorModule) -> None:
    """gmActor.snapshot fills Name, HP and State columns for each actor."""
    mod.on_envelope(_snapshot("hero_1", name="Gandalf", current_hp=80, max_hp=100))

    item = mod._actor_items["hero_1"]
    assert item.text(0) == "Gandalf"
    assert "80" in item.text(1)
    assert "100" in item.text(1)
    assert item.text(2) == "ALIVE"


def test_snapshot_adds_faction_to_filter_combo(mod: GmActorModule) -> None:
    """gmActor.snapshot adds each new faction_id to the filter QComboBox."""
    mod.on_envelope(_snapshot("hero_1", faction_id="Mostri"))

    texts = [mod._filter_combo.itemText(i) for i in range(mod._filter_combo.count())]
    assert "Mostri" in texts


# ── HP events ─────────────────────────────────────────────────────────────────

def test_hp_changed_updates_tree_column(mod: GmActorModule) -> None:
    """gmActor.actor.hp_changed updates the HP column in the tree."""
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.hp_changed",
        "data": {"actor_id": "hero_1", "old_hp": 100, "new_hp": 40, "max_hp": 100},
    })

    assert "40" in mod._actor_items["hero_1"].text(1)


def test_hp_changed_updates_hp_bar(mod: GmActorModule) -> None:
    """gmActor.actor.hp_changed updates HpBar when the actor is selected."""
    mod.on_envelope(_snapshot("hero_1"))
    _select(mod, "hero_1")
    mod.on_envelope({
        "typeId": "gmActor.actor.hp_changed",
        "data": {"actor_id": "hero_1", "old_hp": 100, "new_hp": 40, "max_hp": 100},
    })

    assert mod._hp_bar._current_hp == 40
    assert mod._hp_bar._max_hp == 100


def test_hp_changed_unselected_actor_no_bar_update(mod: GmActorModule) -> None:
    """hp_changed for a non-selected actor must NOT change the HpBar."""
    mod.on_envelope(_snapshot("hero_1", current_hp=100, max_hp=100))
    # hero_1 is not selected; the HpBar was last set to 0/1 (empty detail)
    initial_hp = mod._hp_bar._current_hp
    mod.on_envelope({
        "typeId": "gmActor.actor.hp_changed",
        "data": {"actor_id": "hero_1", "old_hp": 100, "new_hp": 55, "max_hp": 100},
    })

    assert mod._hp_bar._current_hp == initial_hp


# ── HpBar colour tests ────────────────────────────────────────────────────────

def test_hp_bar_color_green_above_50_percent(qapp: QApplication) -> None:
    """HpBar.bar_color() is green when HP > 50 % of max."""
    bar = HpBar()
    bar.set_hp(60, 100)

    assert bar.bar_color() == QColor(Qt.GlobalColor.green)


def test_hp_bar_color_yellow_between_20_and_50(qapp: QApplication) -> None:
    """HpBar.bar_color() is yellow when HP is between 20 % and 50 % of max."""
    bar = HpBar()
    bar.set_hp(30, 100)

    assert bar.bar_color() == QColor(Qt.GlobalColor.yellow)


def test_hp_below_20_percent_shows_red(qapp: QApplication) -> None:
    """HpBar.bar_color() is red when HP is below 20 % of max."""
    bar = HpBar()
    bar.set_hp(10, 100)

    assert bar.bar_color() == QColor(Qt.GlobalColor.red)


def test_hp_bar_ratio_at_zero(qapp: QApplication) -> None:
    """HpBar.ratio() returns 0.0 when current HP is 0."""
    bar = HpBar()
    bar.set_hp(0, 100)

    assert bar.ratio() == 0.0


def test_hp_bar_ratio_at_full(qapp: QApplication) -> None:
    """HpBar.ratio() returns 1.0 when current HP equals max HP."""
    bar = HpBar()
    bar.set_hp(100, 100)

    assert bar.ratio() == 1.0


def test_hp_bar_set_hp_stores_values(qapp: QApplication) -> None:
    """set_hp() stores both current and max values correctly."""
    bar = HpBar()
    bar.set_hp(73, 200)

    assert bar._current_hp == 73
    assert bar._max_hp == 200


# ── Status events ─────────────────────────────────────────────────────────────

def test_status_added_appears_in_list(mod: GmActorModule) -> None:
    """gmActor.actor.status_added adds the status to the list when actor selected."""
    mod.on_envelope(_snapshot("hero_1"))
    _select(mod, "hero_1")
    mod.on_envelope({
        "typeId": "gmActor.actor.status_added",
        "data": {"actor_id": "hero_1", "status_id": "Avvelenato", "stacks": 2},
    })

    texts = [mod._status_list.item(i).text() for i in range(mod._status_list.count())]
    assert any("Avvelenato" in t for t in texts)


def test_status_added_updates_state_column(mod: GmActorModule) -> None:
    """gmActor.actor.status_added reflects the status count in the State column."""
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.status_added",
        "data": {"actor_id": "hero_1", "status_id": "Avvelenato", "stacks": 1},
    })

    assert "1" in mod._actor_items["hero_1"].text(2)


def test_status_removed_leaves_list(mod: GmActorModule) -> None:
    """gmActor.actor.status_removed removes the entry from the status list."""
    mod.on_envelope(_snapshot("hero_1"))
    _select(mod, "hero_1")
    mod.on_envelope({
        "typeId": "gmActor.actor.status_added",
        "data": {"actor_id": "hero_1", "status_id": "Avvelenato", "stacks": 1},
    })
    mod.on_envelope({
        "typeId": "gmActor.actor.status_removed",
        "data": {"actor_id": "hero_1", "status_id": "Avvelenato"},
    })

    texts = [mod._status_list.item(i).text() for i in range(mod._status_list.count())]
    assert not any("Avvelenato" in t for t in texts)


# ── Area events ───────────────────────────────────────────────────────────────

def test_moved_area_updates_tooltip(mod: GmActorModule) -> None:
    """gmActor.actor.moved_area sets the tooltip on the actor's tree item."""
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.moved_area",
        "data": {"actor_id": "hero_1", "old_area": "", "new_area": "forest_1"},
    })

    assert "forest_1" in mod._actor_items["hero_1"].toolTip(0)


# ── Life-state events ─────────────────────────────────────────────────────────

def test_life_state_dying_greys_row(mod: GmActorModule) -> None:
    """DYING life state applies red foreground to the actor's tree row.

    (Test name kept for compatibility with original stub; DYING maps to red
    per the plan: DEAD → grigio, DYING → rosso.)
    """
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.life_state_changed",
        "data": {"actor_id": "hero_1", "old_state": "ALIVE", "new_state": "DYING"},
    })

    color = mod._actor_items["hero_1"].foreground(0).color()
    assert color == QColor(Qt.GlobalColor.red)


def test_life_state_dead_sets_gray(mod: GmActorModule) -> None:
    """DEAD life state applies gray foreground to the actor's tree row."""
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.life_state_changed",
        "data": {"actor_id": "hero_1", "old_state": "ALIVE", "new_state": "DEAD"},
    })

    color = mod._actor_items["hero_1"].foreground(0).color()
    assert color == QColor(Qt.GlobalColor.gray)


def test_life_state_alive_resets_color(mod: GmActorModule) -> None:
    """Returning to ALIVE resets the row foreground to the default (non-red) color."""
    mod.on_envelope(_snapshot("hero_1"))
    mod.on_envelope({
        "typeId": "gmActor.actor.life_state_changed",
        "data": {"actor_id": "hero_1", "old_state": "ALIVE", "new_state": "DYING"},
    })
    mod.on_envelope({
        "typeId": "gmActor.actor.life_state_changed",
        "data": {"actor_id": "hero_1", "old_state": "DYING", "new_state": "ALIVE"},
    })

    color = mod._actor_items["hero_1"].foreground(0).color()
    assert color != QColor(Qt.GlobalColor.red)
    assert color != QColor(Qt.GlobalColor.gray)


# ── Equipment events ──────────────────────────────────────────────────────────

def test_item_equipped_appears_in_detail(mod: GmActorModule) -> None:
    """gmActor.actor.item_equipped adds the item to the equipment list."""
    mod.on_envelope(_snapshot("hero_1"))
    _select(mod, "hero_1")
    mod.on_envelope({
        "typeId": "gmActor.actor.item_equipped",
        "data": {"actor_id": "hero_1", "item_instance_id": "sword_01", "slot": "main_hand"},
    })

    texts = [mod._equip_list.item(i).text() for i in range(mod._equip_list.count())]
    assert any("sword_01" in t for t in texts)


def test_item_unequipped_removes_from_detail(mod: GmActorModule) -> None:
    """gmActor.actor.item_unequipped removes the item from the equipment list."""
    mod.on_envelope(_snapshot("hero_1"))
    _select(mod, "hero_1")
    mod.on_envelope({
        "typeId": "gmActor.actor.item_equipped",
        "data": {"actor_id": "hero_1", "item_instance_id": "sword_01", "slot": "main_hand"},
    })
    mod.on_envelope({
        "typeId": "gmActor.actor.item_unequipped",
        "data": {"actor_id": "hero_1", "item_instance_id": "sword_01", "slot": "main_hand"},
    })

    texts = [mod._equip_list.item(i).text() for i in range(mod._equip_list.count())]
    assert not any("sword_01" in t for t in texts)


# ── Filter tests ──────────────────────────────────────────────────────────────

def test_filter_combo_hides_other_factions(mod: GmActorModule) -> None:
    """Selecting a faction hides all other faction groups in the tree."""
    mod.on_envelope(_snapshot("hero_1", faction_id="Eroi"))
    mod.on_envelope({
        "typeId": "gmActor.snapshot",
        "data": {
            "actors": [{
                "actor_id": "villain_1",
                "name": "Sauron",
                "faction_id": "Mostri",
                "current_hp": 200,
                "max_hp": 200,
                "life_state": "ALIVE",
                "statuses": {},
                "equipment": {},
                "area_id": "",
            }]
        },
    })
    mod._filter_combo.setCurrentText("Eroi")

    assert not mod._faction_items["Eroi"].isHidden()
    assert mod._faction_items["Mostri"].isHidden()


def test_filter_tutti_shows_all_factions(mod: GmActorModule) -> None:
    """Selecting 'Tutti' reveals all faction groups."""
    mod.on_envelope(_snapshot("hero_1", faction_id="Eroi"))
    mod._filter_combo.setCurrentText("Eroi")
    mod._filter_combo.setCurrentText("Tutti")

    assert not mod._faction_items["Eroi"].isHidden()


# ── Selection / detail panel ──────────────────────────────────────────────────

def test_selection_updates_detail_name(mod: GmActorModule) -> None:
    """Selecting an actor in the tree updates the detail name label."""
    mod.on_envelope(_snapshot("hero_1", name="Legolas"))
    _select(mod, "hero_1")

    assert mod._detail_name.text() == "Legolas"


def test_selection_populates_hp_bar(mod: GmActorModule) -> None:
    """Selecting an actor populates the HpBar with the actor's stored HP."""
    mod.on_envelope(_snapshot("hero_1", current_hp=70, max_hp=100))
    _select(mod, "hero_1")

    assert mod._hp_bar._current_hp == 70
    assert mod._hp_bar._max_hp == 100
