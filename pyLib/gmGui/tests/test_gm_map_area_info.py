"""Tests for the shared area-info contract and related module changes.

Covers:
- :class:`GmMapAreaInfoModule` rendering of actors and interactables.
- :class:`GmMapModule` selection sending an area-info request (no gameplay action).
- :class:`GmActorModule` detail panel always visible, actor tree hidden by default.

All tests run in offscreen mode (no display required).
"""
from __future__ import annotations

import os
import sys

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.message_ids import AREA_INFO_REQUEST, AREA_INFO_RESPONSE, AREA_SELECTED
from gmGui.modules.gm_actor_module import GmActorModule
from gmGui.modules.gm_map_area_info_module import GmMapAreaInfoModule
from gmGui.modules.gm_map_module import GmMapModule
from gmGui.widgets.map_scene import LocationNode


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Fake sender capturing outgoing commands ───────────────────────────────────

class _FakeSender:
    def __init__(self) -> None:
        self.sent: list[tuple[str, dict]] = []

    def send_command(self, type_id: str, data: dict) -> None:
        self.sent.append((type_id, data))


# ── GmMapAreaInfoModule ───────────────────────────────────────────────────────

def test_area_info_populates_both_lists(qapp: QApplication) -> None:
    mod = GmMapAreaInfoModule()
    mod.widget()
    msg = {
        "typeId": AREA_INFO_RESPONSE,
        "data": {
            "area_id": "room_2",
            "actors": [
                {"id": "hero_1", "name": "Hero", "faction": "HERO", "state": "ALIVE"},
                {"id": "orc_1", "name": "Orc", "faction": "MONSTER"},
            ],
            "interactables": [
                {"id": "chest", "name": "Chest", "type": "container"},
            ],
        },
    }
    mod.on_envelope(msg)
    assert mod._actors_list.count() == 2
    assert mod._interactables_list.count() == 1
    assert "Hero" in mod._actors_list.item(0).text()
    assert "Chest" in mod._interactables_list.item(0).text()
    assert "room_2" in mod._area_label.text()


def test_area_info_reads_headers_data(qapp: QApplication) -> None:
    import json

    mod = GmMapAreaInfoModule()
    mod.widget()
    payload = {"area_id": "room_5", "actors": [], "interactables": []}
    msg = {"typeId": AREA_INFO_RESPONSE, "headers": {"data": json.dumps(payload)}}
    mod.on_envelope(msg)
    assert "room_5" in mod._area_label.text()


def test_area_info_subscribes_to_response(qapp: QApplication) -> None:
    mod = GmMapAreaInfoModule()
    assert AREA_INFO_RESPONSE in mod.subscribed_type_ids()


# ── GmMapModule: click => request, not action ─────────────────────────────────

def test_map_selection_sends_area_info_request(qapp: QApplication) -> None:
    mod = GmMapModule()
    mod.widget()
    sender = _FakeSender()
    mod.set_sender(sender)

    mod._request_area_info("3")

    type_ids = [t for t, _ in sender.sent]
    assert AREA_SELECTED in type_ids
    assert AREA_INFO_REQUEST in type_ids
    request = next(d for t, d in sender.sent if t == AREA_INFO_REQUEST)
    assert request["area_id"] == "3"


def test_map_empty_area_id_sends_nothing(qapp: QApplication) -> None:
    mod = GmMapModule()
    mod.widget()
    sender = _FakeSender()
    mod.set_sender(sender)

    mod._request_area_info("")

    assert sender.sent == []


# ── GmActorModule: tree hidden by default, details always visible ─────────────

def test_actor_tree_hidden_by_default(qapp: QApplication) -> None:
    mod = GmActorModule()
    root = mod.widget()
    assert mod._tree_visible is False
    # The whole left pane (filters + tree) is collapsed; only the detail panel shows.
    assert mod._left_panel.isVisibleTo(root) is False
    assert mod._splitter.sizes()[0] == 0
    assert mod._right_panel is not None


def test_actor_tree_toggle_shows_and_hides(qapp: QApplication) -> None:
    mod = GmActorModule()
    root = mod.widget()
    mod._toggle_actor_tree()
    assert mod._tree_visible is True
    assert mod._left_panel.isVisibleTo(root) is True
    assert mod._splitter.sizes()[0] > 0
    mod._toggle_actor_tree()
    assert mod._tree_visible is False
    assert mod._left_panel.isVisibleTo(root) is False
    assert mod._splitter.sizes()[0] == 0


def test_actor_tree_state_persisted(qapp: QApplication) -> None:
    mod = GmActorModule()
    mod.widget()
    mod._toggle_actor_tree()  # now visible
    state = mod.save_state()
    assert state["tree_visible"] is True

    other = GmActorModule()
    other.widget()
    other.restore_state(state)
    assert other._tree_visible is True


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
