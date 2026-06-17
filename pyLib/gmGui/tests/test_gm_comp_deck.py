"""Tests for GmCompDeckModule.

Full implementation: Phase 6.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 6: GmCompDeckModule not yet implemented")
def test_zone_changed_updates_counter() -> None:
    """gmAlea.deck.zone_changed with 3 cards sets the counter label to '3'."""


@pytest.mark.skip(reason="Phase 6: GmCompDeckModule not yet implemented")
def test_card_moved_updates_both_zone_counters() -> None:
    """gmAlea.deck.card_moved from MainDeck to CardHand updates both counters."""


@pytest.mark.skip(reason="Phase 6: GmCompDeckModule not yet implemented")
def test_drag_drop_emits_send_command() -> None:
    """A simulated drag-and-drop triggers send_command with correct parameters."""


@pytest.mark.skip(reason="Phase 6: GmCompDeckModule not yet implemented")
def test_banish_zone_rejects_drop() -> None:
    """BanishZone has DragDropMode=NoDragDrop; dropping onto it is not allowed."""
