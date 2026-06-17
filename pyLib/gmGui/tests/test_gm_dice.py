"""Tests for GmDiceModule.

Full implementation: Phase 7.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 7: GmDiceModule not yet implemented")
def test_roll_button_sends_command() -> None:
    """Clicking [LANCIA] calls send_command('gmAlea.dice.roll_request', ...)."""


@pytest.mark.skip(reason="Phase 7: GmDiceModule not yet implemented")
def test_roll_result_updates_labels() -> None:
    """roll_result with dice=[3,5,2] sets total label to '10' and detail to '3 + 5 + 2'."""


@pytest.mark.skip(reason="Phase 7: GmDiceModule not yet implemented")
def test_history_grows_after_roll() -> None:
    """After one roll the history list contains exactly one entry."""


@pytest.mark.skip(reason="Phase 7: GmDiceModule not yet implemented")
def test_history_capped_at_ten() -> None:
    """History list never exceeds 10 entries regardless of how many rolls occur."""
