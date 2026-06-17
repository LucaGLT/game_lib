"""Tests for Phase 9 — Layout Persistence & Module State.

Covers:
- Per-module ``save_state()`` / ``restore_state()`` round-trips.
- ``settings.save_layout()`` / ``settings.restore_layout()`` integration
  (uses QSettings with a unique test-scoped application name to avoid
  colliding with real settings files).

All tests run in offscreen mode (no display required).
"""
from __future__ import annotations

import os
import sys
from unittest.mock import patch

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_comp_deck_module import GmCompDeckModule
from gmGui.modules.gm_flow_module import GmFlowModule
from gmGui.modules.gm_map_module import GmMapModule


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Per-test module fixtures ──────────────────────────────────────────────────

@pytest.fixture
def flow_mod(qapp: QApplication) -> GmFlowModule:
    m = GmFlowModule()
    m.widget()
    return m


@pytest.fixture
def map_mod(qapp: QApplication) -> GmMapModule:
    m = GmMapModule()
    m.widget()
    return m


@pytest.fixture
def deck_mod(qapp: QApplication) -> GmCompDeckModule:
    m = GmCompDeckModule()
    m.widget()
    return m


# ── GmFlowModule ─────────────────────────────────────────────────────────────

def test_flow_save_state_contains_pixels_per_unit(flow_mod: GmFlowModule) -> None:
    """save_state() returns a dict with 'pixels_per_unit' key."""
    state = flow_mod.save_state()
    assert "pixels_per_unit" in state
    assert isinstance(state["pixels_per_unit"], int)
    assert state["pixels_per_unit"] > 0


def test_flow_restore_state_updates_pixels_per_unit(flow_mod: GmFlowModule) -> None:
    """restore_state({'pixels_per_unit': 16}) changes the timeline scale."""
    flow_mod.restore_state({"pixels_per_unit": 16})
    assert flow_mod._timeline_scene._pixels_per_unit == 16


def test_flow_save_restore_roundtrip(flow_mod: GmFlowModule) -> None:
    """save_state() followed by restore_state() preserves pixels_per_unit."""
    flow_mod._timeline_scene._pixels_per_unit = 12
    state = flow_mod.save_state()
    flow_mod._timeline_scene._pixels_per_unit = 8  # reset to default
    flow_mod.restore_state(state)
    assert flow_mod._timeline_scene._pixels_per_unit == 12


def test_flow_restore_state_ignores_invalid_value(flow_mod: GmFlowModule) -> None:
    """restore_state() with non-positive pixels_per_unit leaves value unchanged."""
    flow_mod._timeline_scene._pixels_per_unit = 8
    flow_mod.restore_state({"pixels_per_unit": 0})
    assert flow_mod._timeline_scene._pixels_per_unit == 8
    flow_mod.restore_state({"pixels_per_unit": -4})
    assert flow_mod._timeline_scene._pixels_per_unit == 8


# ── GmMapModule ───────────────────────────────────────────────────────────────

def test_map_save_state_contains_zoom_and_layer(map_mod: GmMapModule) -> None:
    """save_state() returns dict with 'zoom_level' (float) and 'layer' (str)."""
    state = map_mod.save_state()
    assert "zoom_level" in state
    assert "layer" in state
    assert isinstance(state["zoom_level"], float)
    assert isinstance(state["layer"], str)


def test_map_restore_state_updates_zoom_level(map_mod: GmMapModule) -> None:
    """restore_state({'zoom_level': 2.0, 'layer': ...}) sets _zoom_level to 2.0."""
    map_mod.restore_state({"zoom_level": 2.0, "layer": "terrain"})
    assert abs(map_mod._zoom_level - 2.0) < 0.01


def test_map_restore_state_updates_layer(map_mod: GmMapModule) -> None:
    """restore_state updates the layer QComboBox to 'items'."""
    map_mod.restore_state({"zoom_level": 1.0, "layer": "items"})
    assert map_mod._layer_combo.currentText() == "items"


def test_map_save_restore_roundtrip(map_mod: GmMapModule) -> None:
    """save_state() followed by restore_state() preserves zoom and layer."""
    map_mod._zoom(2.0)
    idx = map_mod._layer_combo.findText("actors")
    if idx >= 0:
        map_mod._layer_combo.setCurrentIndex(idx)
    state = map_mod.save_state()

    # Reset to defaults.
    map_mod._map_view.resetTransform()
    map_mod._zoom_level = 1.0
    first_idx = map_mod._layer_combo.findText("terrain")
    if first_idx >= 0:
        map_mod._layer_combo.setCurrentIndex(first_idx)

    map_mod.restore_state(state)
    assert abs(map_mod._zoom_level - state["zoom_level"]) < 0.01
    assert map_mod._layer_combo.currentText() == "actors"


def test_map_zoom_clamped_after_restore(map_mod: GmMapModule) -> None:
    """restore_state clamps an out-of-range zoom_level to [0.25, 4.0]."""
    map_mod.restore_state({"zoom_level": 99.0})
    assert map_mod._zoom_level <= 4.0
    map_mod.restore_state({"zoom_level": 0.001})
    assert map_mod._zoom_level >= 0.25


# ── GmCompDeckModule ──────────────────────────────────────────────────────────

def test_deck_save_state_contains_deck_name(deck_mod: GmCompDeckModule) -> None:
    """save_state() returns a dict with 'deck' key (str)."""
    state = deck_mod.save_state()
    assert "deck" in state
    assert isinstance(state["deck"], str)


def test_deck_restore_state_updates_combo(deck_mod: GmCompDeckModule) -> None:
    """restore_state({'deck': 'Deck 1'}) selects that item in the QComboBox."""
    deck_mod.restore_state({"deck": "Deck 1"})
    assert deck_mod._deck_combo.currentText() == "Deck 1"


def test_deck_restore_state_unknown_name_ignored(deck_mod: GmCompDeckModule) -> None:
    """restore_state with an unknown deck name leaves the combo unchanged."""
    original = deck_mod._deck_combo.currentText()
    deck_mod.restore_state({"deck": "NonExistentDeck"})
    assert deck_mod._deck_combo.currentText() == original


def test_deck_save_restore_roundtrip(deck_mod: GmCompDeckModule) -> None:
    """save_state() followed by restore_state() preserves deck selection."""
    # Add a second deck so we can test a non-default selection.
    deck_mod._deck_combo.addItem("Deck 2")
    idx = deck_mod._deck_combo.findText("Deck 2")
    deck_mod._deck_combo.setCurrentIndex(idx)
    state = deck_mod.save_state()

    # Reset to first deck.
    deck_mod._deck_combo.setCurrentIndex(0)

    deck_mod.restore_state(state)
    assert deck_mod._deck_combo.currentText() == "Deck 2"


# ── save_layout / restore_layout integration ─────────────────────────────────

def test_save_restore_layout_smoke(qapp: QApplication) -> None:
    """save_layout + restore_layout complete without raising exceptions.

    Uses a unique QSettings scope ('gmGui_test') to avoid writing to the
    real application settings file.
    """
    from unittest.mock import MagicMock, patch

    from gmGui import settings
    from gmGui.engine_bridge.receiver import EngineReceiver

    with patch.object(EngineReceiver, "start"):
        from gmGui.main_window import MainWindow

        win = MainWindow()
        try:
            # Patch QSettings org/app so we don't pollute real settings.
            with patch("gmGui.settings._ORG", "GameLib_test"), \
                 patch("gmGui.settings._APP", "gmGui_test"):
                settings.save_layout(win)
                settings.restore_layout(win)
        finally:
            win._receiver.stop()
            win._sender.close()
            win.close()
