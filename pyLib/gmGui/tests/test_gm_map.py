"""Tests for GmMapModule.

Full implementation: Phase 8.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 8: GmMapModule not yet implemented")
def test_map_loaded_creates_nodes_and_edges() -> None:
    """gmMap.map.loaded with 5 locations and 4 edges produces 5 nodes and 4 edges."""


@pytest.mark.skip(reason="Phase 8: GmMapModule not yet implemented")
def test_actor_moved_area_repositions_marker() -> None:
    """gmActor.actor.moved_area calls MapScene.move_actor with correct arguments."""


@pytest.mark.skip(reason="Phase 8: GmMapModule not yet implemented")
def test_metadata_changed_updates_node_colour() -> None:
    """gmMap.location.metadata_changed triggers MapScene.update_location()."""


@pytest.mark.skip(reason="Phase 8: GmMapModule not yet implemented")
def test_zoom_limits_respected() -> None:
    """QGraphicsView scale stays within [0.25, 4.0] after repeated zoom actions."""
