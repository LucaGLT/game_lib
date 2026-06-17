"""Tests for GmMapModule and MapScene — Phase 8 implementation.

All tests run in offscreen mode (no display required).
The ``mod`` fixture builds a fresh ``GmMapModule`` with its widget
constructed, isolating each test from state leakage.
"""
from __future__ import annotations

import json
import os
import sys
from unittest.mock import patch

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_map_module import GmMapModule


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Per-test module fixture ───────────────────────────────────────────────────

@pytest.fixture
def mod(qapp: QApplication) -> GmMapModule:
    """Fresh GmMapModule with widget already built."""
    m = GmMapModule()
    m.widget()
    return m


# ── Helpers ───────────────────────────────────────────────────────────────────

def _map_loaded_msg(n_locations: int, n_edges: int) -> dict:
    """Builds a gmMap.map.loaded envelope with sequential location IDs and edges."""
    locations = [{"location_id": i} for i in range(n_locations)]
    edges = [[i, i + 1] for i in range(n_edges)]
    return {
        "typeId": "gmMap.map.loaded",
        "headers": {"data": json.dumps({"locations": locations, "edges": edges})},
    }


# ── Tests ─────────────────────────────────────────────────────────────────────

def test_map_loaded_creates_nodes_and_edges(mod: GmMapModule) -> None:
    """gmMap.map.loaded with 5 locations and 4 edges produces 5 nodes and 4 edges."""
    mod.on_envelope(_map_loaded_msg(5, 4))
    assert mod._map_scene.node_count() == 5
    assert mod._map_scene.edge_count() == 4


def test_actor_moved_area_repositions_marker(mod: GmMapModule) -> None:
    """gmActor.actor.moved_area moves the actor marker to the correct location."""
    mod.on_envelope(_map_loaded_msg(5, 4))
    msg = {
        "typeId": "gmActor.actor.moved_area",
        "headers": {"data": json.dumps({"actor_id": "hero_1", "new_area_id": 2})},
    }
    mod.on_envelope(msg)
    assert mod._map_scene.marker_location("hero_1") == 2


def test_metadata_changed_updates_node_colour(mod: GmMapModule) -> None:
    """gmMap.location.metadata_changed calls MapScene.update_location()."""
    mod.on_envelope(_map_loaded_msg(5, 4))
    with patch.object(mod._map_scene, "update_location") as mock_ul:
        msg = {
            "typeId": "gmMap.location.metadata_changed",
            "headers": {
                "data": json.dumps(
                    {"location_id": 3, "metadata": {"terrain": "water"}}
                )
            },
        }
        mod.on_envelope(msg)
        mock_ul.assert_called_once_with(3, {"terrain": "water"})


def test_zoom_limits_respected(mod: GmMapModule) -> None:
    """Zoom level stays within [0.25, 4.0] after repeated zoom in and out."""
    for _ in range(20):
        mod._zoom(1.25)
    assert mod._zoom_level <= 4.0

    for _ in range(20):
        mod._zoom(0.8)
    assert mod._zoom_level >= 0.25

