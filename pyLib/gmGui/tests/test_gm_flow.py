"""Tests for GmFlowModule.

Full implementation: Phase 4.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 4: GmFlowModule not yet implemented")
def test_session_started_enables_buttons() -> None:
    """After gmFlow.session.started the PAUSE and STOP buttons become enabled."""


@pytest.mark.skip(reason="Phase 4: GmFlowModule not yet implemented")
def test_phase_entered_updates_label() -> None:
    """gmFlow.phase.entered updates the Phase label to the new phase_id."""


@pytest.mark.skip(reason="Phase 4: GmFlowModule not yet implemented")
def test_round_started_updates_round_label() -> None:
    """gmFlow.round.started updates the Round label with the 1-based index."""


@pytest.mark.skip(reason="Phase 4: GmFlowModule not yet implemented")
def test_timeline_actor_selected_highlights_block() -> None:
    """gmFlow.timeline.actor_selected calls TimelineScene.select_actor()."""


@pytest.mark.skip(reason="Phase 4: GmFlowModule not yet implemented")
def test_timeline_time_advanced_moves_cursor() -> None:
    """gmFlow.timeline.time_advanced calls TimelineScene.advance_time()."""
