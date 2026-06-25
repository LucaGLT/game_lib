"""Tests for GmFlowModule — Phase 4 implementation.

All tests run in offscreen mode (no display required).
The ``mod`` fixture builds a fresh ``GmFlowModule`` with its widget
already constructed, isolating each test from state leakage.
"""
from __future__ import annotations

import os
import sys

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_flow_module import GmFlowModule
from gmGui.widgets.timeline_scene import TimelineScene


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    """Single QApplication shared across all tests in this module."""
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Per-test module fixture ───────────────────────────────────────────────────

@pytest.fixture
def mod(qapp: QApplication) -> GmFlowModule:
    """Fresh GmFlowModule with widget already built (triggers _build_widget)."""
    m = GmFlowModule()
    m.widget()
    return m


# ── Session events ────────────────────────────────────────────────────────────

def test_session_started_enables_buttons(mod: GmFlowModule) -> None:
    """After gmFlow.session.started PAUSE and STOP become enabled; RESUME stays off."""
    mod.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "s1"}})

    assert mod._btn_pause.isEnabled()
    assert mod._btn_stop.isEnabled()
    assert not mod._btn_resume.isEnabled()


def test_session_started_updates_session_label(mod: GmFlowModule) -> None:
    """gmFlow.session.started updates the Session label with the session_id."""
    mod.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "sess_42"}})

    assert "sess_42" in mod._lbl_session.text()


def test_session_paused_swaps_buttons(mod: GmFlowModule) -> None:
    """After paused: PAUSE is disabled, RESUME is enabled."""
    mod.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "s1"}})
    mod.on_envelope({"typeId": "gmFlow.session.paused", "data": {}})

    assert not mod._btn_pause.isEnabled()
    assert mod._btn_resume.isEnabled()


def test_session_completed_disables_all_buttons(mod: GmFlowModule) -> None:
    """gmFlow.session.completed disables all three control buttons."""
    mod.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "s1"}})
    mod.on_envelope({"typeId": "gmFlow.session.completed", "data": {}})

    assert not mod._btn_pause.isEnabled()
    assert not mod._btn_resume.isEnabled()
    assert not mod._btn_stop.isEnabled()


# ── Phase events ──────────────────────────────────────────────────────────────

def test_phase_entered_updates_label(mod: GmFlowModule) -> None:
    """gmFlow.phase.entered updates the Phase label to the new phase_id."""
    mod.on_envelope({
        "typeId": "gmFlow.phase.entered",
        "data": {"phase_id": "combat", "previous_id": ""},
    })

    assert "combat" in mod._lbl_phase.text()


def test_phase_entered_appends_log(mod: GmFlowModule) -> None:
    """gmFlow.phase.entered adds an entry at row 0 of the event log."""
    mod.on_envelope({
        "typeId": "gmFlow.phase.entered",
        "data": {"phase_id": "exploration", "previous_id": ""},
    })

    assert mod._log.count() > 0
    assert "exploration" in mod._log.item(0).text()


# ── Round events ──────────────────────────────────────────────────────────────

def test_round_started_updates_round_label(mod: GmFlowModule) -> None:
    """gmFlow.round.started updates the Round label with the 1-based index."""
    mod.on_envelope({
        "typeId": "gmFlow.round.started",
        "data": {"round_id": "r1", "index": 3},
    })

    assert "3" in mod._lbl_round.text()


# ── Turn events ───────────────────────────────────────────────────────────────

def test_turn_started_updates_turn_label(mod: GmFlowModule) -> None:
    """gmFlow.turn.started updates the Turn label with the turn_id."""
    mod.on_envelope({
        "typeId": "gmFlow.turn.started",
        "data": {"turn_id": "t_hero", "active_actors": []},
    })

    assert "t_hero" in mod._lbl_turn.text()


def test_turn_started_selects_first_active_actor(mod: GmFlowModule) -> None:
    """gmFlow.turn.started calls select_actor for the first actor in active_actors."""
    mod._timeline_scene.set_actors([
        {"actor_id": "hero_1", "timeline_position": 0},
        {"actor_id": "hero_2", "timeline_position": 5},
    ])
    mod.on_envelope({
        "typeId": "gmFlow.turn.started",
        "data": {"turn_id": "t1", "active_actors": ["hero_1"]},
    })

    assert mod._timeline_scene._selected_id == "hero_1"


def test_turn_started_empty_active_actors_does_not_crash(mod: GmFlowModule) -> None:
    """gmFlow.turn.started with empty active_actors must not raise."""
    mod.on_envelope({
        "typeId": "gmFlow.turn.started",
        "data": {"turn_id": "t_empty", "active_actors": []},
    })

    assert "t_empty" in mod._lbl_turn.text()


# ── Timeline events ───────────────────────────────────────────────────────────

def test_timeline_actor_selected_highlights_block(mod: GmFlowModule) -> None:
    """gmFlow.timeline.actor_selected calls TimelineScene.select_actor()."""
    mod._timeline_scene.set_actors([
        {"actor_id": "hero_1", "timeline_position": 0},
        {"actor_id": "villain", "timeline_position": 3},
    ])
    mod.on_envelope({
        "typeId": "gmFlow.timeline.actor_selected",
        "data": {"actor_id": "hero_1", "timeline_position": 0},
    })

    assert mod._timeline_scene._selected_id == "hero_1"


def test_timeline_time_advanced_moves_cursor(mod: GmFlowModule) -> None:
    """gmFlow.timeline.time_advanced calls TimelineScene.advance_time()."""
    mod.on_envelope({
        "typeId": "gmFlow.timeline.time_advanced",
        "data": {"old_time": 0, "new_time": 10},
    })

    assert mod._timeline_scene._current_time == 10


# ── Event log behaviour ───────────────────────────────────────────────────────

def test_log_inserts_most_recent_first(mod: GmFlowModule) -> None:
    """Event log shows the most-recent entry at row 0."""
    mod.on_envelope({
        "typeId": "gmFlow.phase.entered",
        "data": {"phase_id": "first"},
    })
    mod.on_envelope({
        "typeId": "gmFlow.phase.entered",
        "data": {"phase_id": "second"},
    })

    assert "second" in mod._log.item(0).text()


def test_log_never_exceeds_max_entries(mod: GmFlowModule) -> None:
    """Event log never holds more than 20 entries regardless of how many arrive."""
    for i in range(25):
        mod.on_envelope({
            "typeId": "gmFlow.phase.entered",
            "data": {"phase_id": f"phase_{i}"},
        })

    assert mod._log.count() <= 20


# ── TimelineScene standalone tests ────────────────────────────────────────────

def test_scene_set_actors_populates_rects(qapp: QApplication) -> None:
    """set_actors() creates exactly one QGraphicsRectItem per actor."""
    scene = TimelineScene()
    scene.set_actors([
        {"actor_id": "a", "timeline_position": 0},
        {"actor_id": "b", "timeline_position": 2},
    ])

    assert len(scene._actor_rects) == 2
    assert "a" in scene._actor_rects
    assert "b" in scene._actor_rects


def test_scene_set_actors_clears_previous(qapp: QApplication) -> None:
    """Calling set_actors() twice replaces the previous actor set completely."""
    scene = TimelineScene()
    scene.set_actors([{"actor_id": "old", "timeline_position": 0}])
    scene.set_actors([
        {"actor_id": "new_a", "timeline_position": 0},
        {"actor_id": "new_b", "timeline_position": 1},
    ])

    assert "old" not in scene._actor_rects
    assert len(scene._actor_rects) == 2


def test_scene_select_actor_sets_selected_id(qapp: QApplication) -> None:
    """select_actor() updates _selected_id even when the scene is empty."""
    scene = TimelineScene()
    scene.select_actor("hero_1")

    assert scene._selected_id == "hero_1"


def test_scene_select_actor_changes_selection(qapp: QApplication) -> None:
    """Selecting a second actor deselects the first one."""
    scene = TimelineScene()
    scene.set_actors([
        {"actor_id": "a", "timeline_position": 0},
        {"actor_id": "b", "timeline_position": 2},
    ])
    scene.select_actor("a")
    scene.select_actor("b")

    assert scene._selected_id == "b"


def test_scene_advance_time_updates_current_time(qapp: QApplication) -> None:
    """advance_time() updates _current_time."""
    scene = TimelineScene()
    scene.advance_time(7)

    assert scene._current_time == 7


def test_scene_advance_time_before_set_actors_does_not_crash(
    qapp: QApplication,
) -> None:
    """advance_time() before set_actors() must not raise (no time line yet)."""
    scene = TimelineScene()
    scene.advance_time(5)

    assert scene._current_time == 5


def test_scene_set_actors_preserves_selection(qapp: QApplication) -> None:
    """set_actors() re-highlights the previously selected actor if still present."""
    from PySide6.QtCore import Qt

    scene = TimelineScene()
    scene.set_actors([
        {"actor_id": "hero", "timeline_position": 0},
    ])
    scene.select_actor("hero")

    # Re-populate with the same actor.
    scene.set_actors([
        {"actor_id": "hero", "timeline_position": 2},
    ])

    assert scene._selected_id == "hero"
    # The rect should have the active (yellow) pen, not the inactive one.
    pen = scene._actor_rects["hero"].pen()
    assert pen.color() == Qt.GlobalColor.yellow
