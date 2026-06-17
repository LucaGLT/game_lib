"""Tests for GmActorModule.

Full implementation: Phase 5.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 5: GmActorModule not yet implemented")
def test_hp_changed_updates_hp_bar() -> None:
    """gmActor.actor.hp_changed updates the HpBar to the new current/max values."""


@pytest.mark.skip(reason="Phase 5: GmActorModule not yet implemented")
def test_hp_below_20_percent_shows_red() -> None:
    """HpBar colour is red when current HP is below 20 % of max."""


@pytest.mark.skip(reason="Phase 5: GmActorModule not yet implemented")
def test_status_added_appears_in_list() -> None:
    """gmActor.actor.status_added adds an entry to the status list."""


@pytest.mark.skip(reason="Phase 5: GmActorModule not yet implemented")
def test_life_state_dying_greys_row() -> None:
    """DYING life state applies the red colour to the actor's tree row."""
