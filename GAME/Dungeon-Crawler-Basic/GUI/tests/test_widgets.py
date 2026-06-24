"""Unit tests for GUI widgets (headless, no Qt display)."""
from __future__ import annotations

import sys
from pathlib import Path

_GUI_DIR  = Path(__file__).resolve().parents[2] / "GUI"
_PYLIB    = Path(__file__).resolve().parents[4] / "pyLib"

sys.path.insert(0, str(_GUI_DIR))
sys.path.insert(0, str(_PYLIB))

# Headless Qt
import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication  # noqa: E402

_app = QApplication.instance() or QApplication(sys.argv)

from widgets.log_widget import LogWidget            # noqa: E402
from widgets.error_bar_widget import ErrorBarWidget  # noqa: E402
from widgets.action_panel_widget import ActionPanelWidget  # noqa: E402
from widgets.hero_panel_widget import HeroPanelWidget  # noqa: E402
from widgets.dungeon_board_widget import DungeonBoardWidget  # noqa: E402


def test_log_widget_append_and_clear():
    w = LogWidget()
    w.append_entry("hello")
    assert w._list.count() == 1
    w.clear()
    assert w._list.count() == 0
    print("  [OK] test_log_widget_append_and_clear")


def test_log_widget_on_envelope_session_started():
    w = LogWidget()
    w.on_envelope({"typeId": "dungeon.session.started", "data": {"round": 1}})
    assert w._list.count() == 1
    print("  [OK] test_log_widget_on_envelope_session_started")


def test_log_widget_unknown_event_ignored():
    w = LogWidget()
    w.on_envelope({"typeId": "dungeon.unknown.event", "data": {}})
    assert w._list.count() == 0
    print("  [OK] test_log_widget_unknown_event_ignored")


def test_error_bar_show_and_clear():
    w = ErrorBarWidget()
    w.show_error("test error")
    assert "test error" in w._label.text()
    w.clear()
    assert w._label.text() == ""
    print("  [OK] test_error_bar_show_and_clear")


def test_error_bar_on_envelope_rejected():
    w = ErrorBarWidget()
    w.on_envelope({
        "typeId": "dungeon.action.rejected",
        "data": {"reason": "No potion available.", "command": "dungeon.heal"}
    })
    assert "No potion" in w._label.text()
    print("  [OK] test_error_bar_on_envelope_rejected")


def test_error_bar_ignores_other_events():
    w = ErrorBarWidget()
    w.on_envelope({"typeId": "dungeon.actor.moved", "data": {}})
    assert w._label.text() == ""
    print("  [OK] test_error_bar_ignores_other_events")


def test_action_panel_initial_state():
    w = ActionPanelWidget()
    assert not w._btn_heal.isEnabled()
    assert not w._btn_equip.isEnabled()
    print("  [OK] test_action_panel_initial_state")


def test_action_panel_enables_on_snapshot_with_potion():
    w = ActionPanelWidget()
    w.on_envelope({
        "typeId": "dungeon.actor.snapshot",
        "data": {
            "actors": [{
                "id": "hero_1", "kind": "HERO", "hp": 10, "max_hp": 10,
                "tags": ["has_potion", "bigword_available"], "statuses": []
            }]
        }
    })
    # After snapshot the actor data is cached; turn must also start to enable buttons.
    state = w._actors_state.get("hero_1", {})
    assert state.get("has_potion") is True
    assert state.get("has_item") is True
    print("  [OK] test_action_panel_enables_on_snapshot_with_potion")


def test_action_panel_reset():
    w = ActionPanelWidget()
    w._active_actor_id = "hero_1"
    w.reset()
    assert w._active_actor_id == ""
    assert not w._btn_heal.isEnabled()
    assert not w._btn_equip.isEnabled()
    print("  [OK] test_action_panel_reset")


def test_hero_panel_adapter_snapshot():
    w = HeroPanelWidget()
    w.on_envelope({
        "typeId": "dungeon.actor.snapshot",
        "data": {
            "actors": [{
                "id": "hero",
                "kind": "HERO",
                "hp": 10,
                "max_hp": 10,
                "location": "room_1",
                "tags": ["has_potion"],
                "statuses": []
            }]
        }
    })
    assert w._module._actor_data["hero"]["faction_id"] == "heroes"
    assert w._module._actor_data["hero"]["area_id"] == "room_1"
    print("  [OK] test_hero_panel_adapter_snapshot")


def test_dungeon_board_adapter_move_request():
    w = DungeonBoardWidget()
    moves: list[tuple[str, str]] = []
    areas: list[str] = []
    w.move_requested.connect(lambda hero_id, destination: moves.append((hero_id, destination)))
    w.area_selected.connect(lambda area_id: areas.append(area_id))

    w.on_envelope({
        "typeId": "dungeon.map.snapshot",
        "data": {
            "rooms": [
                {"id": "room_1", "tags": ["start"], "adjacent": ["room_2"]},
                {"id": "room_2", "tags": [], "adjacent": ["room_1"]},
            ]
        }
    })
    w.on_envelope({
        "typeId": "dungeon.actor.snapshot",
        "data": {
            "actors": [{
                "id": "hero",
                "kind": "HERO",
                "hp": 10,
                "max_hp": 10,
                "location": "room_1",
                "tags": [],
                "statuses": []
            }]
        }
    })

    node = w._module._map_scene._nodes[w._room_index_by_id["room_2"]]
    node.setSelected(True)
    # A click is view-only: it selects the area, never auto-moves.
    assert areas == ["room_2"]
    assert moves == []
    assert w.move_destination() == "room_2"
    # The explicit Move action emits move_requested for the selected adjacent room.
    w.request_move()
    assert moves == [("hero", "room_2")]
    print("  [OK] test_dungeon_board_adapter_move_request")


if __name__ == "__main__":
    print("=== Widget unit tests ===")
    test_log_widget_append_and_clear()
    test_log_widget_on_envelope_session_started()
    test_log_widget_unknown_event_ignored()
    test_error_bar_show_and_clear()
    test_error_bar_on_envelope_rejected()
    test_error_bar_ignores_other_events()
    test_action_panel_initial_state()
    test_action_panel_enables_on_snapshot_with_potion()
    test_action_panel_reset()
    test_hero_panel_adapter_snapshot()
    test_dungeon_board_adapter_move_request()
    print("All widget tests PASSED.")
