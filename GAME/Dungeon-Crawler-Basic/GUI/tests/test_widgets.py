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

    w.set_active_hero("hero")
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


# ── Adjacency tests ───────────────────────────────────────────────────────────

def _make_board_with_rooms(rooms: list[dict]) -> DungeonBoardWidget:
    """Helper: create a board widget with the given room list."""
    w = DungeonBoardWidget()
    w.on_envelope({"typeId": "dungeon.map.snapshot", "data": {"rooms": rooms}})
    return w


def _place_hero(w: DungeonBoardWidget, hero_id: str, location: str) -> None:
    w.on_envelope({
        "typeId": "dungeon.actor.snapshot",
        "data": {"actors": [
            {"id": hero_id, "kind": "HERO", "hp": 10, "max_hp": 10,
             "location": location, "tags": [], "statuses": []},
        ]},
    })


def test_board_non_adjacent_room_returns_empty():
    """Selecting a room 2 hops away does not qualify as a valid move destination."""
    w = _make_board_with_rooms([
        {"id": "A", "tags": ["start"], "adjacent": ["B"]},
        {"id": "B", "tags": [],        "adjacent": ["A", "C"]},
        {"id": "C", "tags": [],        "adjacent": ["B"]},
    ])
    _place_hero(w, "hero_1", "A")
    w.set_active_hero("hero_1")
    # C is two hops away from A — not adjacent
    node = w._module._map_scene._nodes[w._room_index_by_id["C"]]
    node.setSelected(True)
    assert w.move_destination() == "", "Non-adjacent room C should return ''"
    w._module._map_scene.clearSelection()
    print("  [OK] test_board_non_adjacent_room_returns_empty")


def test_board_current_room_not_valid_destination():
    """The hero's own room is not a valid move destination."""
    w = _make_board_with_rooms([
        {"id": "A", "tags": ["start"], "adjacent": ["B"]},
        {"id": "B", "tags": [],        "adjacent": ["A"]},
    ])
    _place_hero(w, "hero_1", "A")
    w.set_active_hero("hero_1")
    node = w._module._map_scene._nodes[w._room_index_by_id["A"]]
    node.setSelected(True)
    assert w.move_destination() == "", "Hero's own room should return ''"
    w._module._map_scene.clearSelection()
    print("  [OK] test_board_current_room_not_valid_destination")


def test_board_adjacent_room_valid_after_move():
    """After actor.moved, adjacency is recomputed from the new position."""
    w = _make_board_with_rooms([
        {"id": "A", "tags": ["start"], "adjacent": ["B"]},
        {"id": "B", "tags": [],        "adjacent": ["A", "C"]},
        {"id": "C", "tags": [],        "adjacent": ["B"]},
    ])
    _place_hero(w, "hero_1", "A")
    w.set_active_hero("hero_1")
    # Hero moves A → B
    w.on_envelope({"typeId": "dungeon.actor.moved", "data": {"actor_id": "hero_1", "to": "B"}})
    # Now C is adjacent to B
    scene = w._module._map_scene
    scene.clearSelection()
    node_c = scene._nodes[w._room_index_by_id["C"]]
    node_c.setSelected(True)
    assert w.move_destination() == "C", "After moving to B, C should be reachable"
    # A is also adjacent to B
    scene.clearSelection()
    node_a = scene._nodes[w._room_index_by_id["A"]]
    node_a.setSelected(True)
    assert w.move_destination() == "A", "After moving to B, A should be reachable"
    scene.clearSelection()
    print("  [OK] test_board_adjacent_room_valid_after_move")


def test_board_adjacency_bidirectional():
    """Adjacency is symmetric: A→B implies B→A."""
    w = _make_board_with_rooms([
        {"id": "X", "tags": [], "adjacent": ["Y"]},
        {"id": "Y", "tags": [], "adjacent": ["X"]},
    ])
    _place_hero(w, "hero_1", "Y")
    w.set_active_hero("hero_1")
    node = w._module._map_scene._nodes[w._room_index_by_id["X"]]
    node.setSelected(True)
    assert w.move_destination() == "X", "From Y, X should be a valid destination"
    w._module._map_scene.clearSelection()
    print("  [OK] test_board_adjacency_bidirectional")


def test_board_two_heroes_active_hero_determines_adjacency():
    """With multiple heroes, move_destination uses the *active* hero's room."""
    w = _make_board_with_rooms([
        {"id": "A", "tags": [], "adjacent": ["B"]},
        {"id": "B", "tags": [], "adjacent": ["A", "C"]},
        {"id": "C", "tags": [], "adjacent": ["B"]},
    ])
    w.on_envelope({
        "typeId": "dungeon.actor.snapshot",
        "data": {"actors": [
            {"id": "hero_1", "kind": "HERO", "hp": 10, "max_hp": 10,
             "location": "A", "tags": [], "statuses": []},
            {"id": "hero_2", "kind": "HERO", "hp": 8, "max_hp": 8,
             "location": "C", "tags": [], "statuses": []},
        ]},
    })
    # hero_1 turn: C is NOT adjacent to A
    w.set_active_hero("hero_1")
    scene = w._module._map_scene
    node_c = scene._nodes[w._room_index_by_id["C"]]
    node_b = scene._nodes[w._room_index_by_id["B"]]
    node_a = scene._nodes[w._room_index_by_id["A"]]
    scene.clearSelection()
    node_c.setSelected(True)
    assert w.move_destination() == "", "C is not adjacent to hero_1 at A"
    # B IS adjacent to A
    scene.clearSelection()
    node_b.setSelected(True)
    assert w.move_destination() == "B", "B is adjacent to hero_1 at A"
    # hero_2 turn at C: B is adjacent to C
    w.set_active_hero("hero_2")
    scene.clearSelection()
    node_b.setSelected(True)
    assert w.move_destination() == "B", "B is adjacent to hero_2 at C"
    # A is NOT adjacent to C
    scene.clearSelection()
    node_a.setSelected(True)
    assert w.move_destination() == "", "A is not adjacent to hero_2 at C"
    scene.clearSelection()
    print("  [OK] test_board_two_heroes_active_hero_determines_adjacency")


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
    test_board_non_adjacent_room_returns_empty()
    test_board_current_room_not_valid_destination()
    test_board_adjacent_room_valid_after_move()
    test_board_adjacency_bidirectional()
    test_board_two_heroes_active_hero_determines_adjacency()
    print("All widget tests PASSED.")
