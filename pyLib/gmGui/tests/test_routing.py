"""Tests for MainWindow central routing table.

Full implementation: Phase 3.
Phase 1: all tests are skipped.
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 3: MainWindow routing not yet implemented")
def test_routing_table_built_from_subscribed_type_ids() -> None:
    """Every typeId declared by a module appears in MainWindow._routing."""


@pytest.mark.skip(reason="Phase 3: MainWindow routing not yet implemented")
def test_on_envelope_dispatches_to_correct_module() -> None:
    """_on_envelope routes a dict to exactly the module subscribed to its typeId."""


@pytest.mark.skip(reason="Phase 3: MainWindow routing not yet implemented")
def test_unknown_type_id_is_silently_ignored() -> None:
    """_on_envelope with an unregistered typeId does not raise."""


@pytest.mark.skip(reason="Phase 3: MainWindow routing not yet implemented")
def test_multi_module_same_type_id_all_receive() -> None:
    """If two modules subscribe to the same typeId, both receive the envelope."""
