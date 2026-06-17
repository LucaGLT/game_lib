"""Tests for GmDiceModule — Phase 7 implementation.

All tests run in offscreen mode (no display required).
A ``MockSender`` captures ``send_command`` calls so tests can assert on
the command stream without a real TCP connection.
"""
from __future__ import annotations

import json
import os
import sys

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_dice_module import GmDiceModule


# ── Shared QApplication ───────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── MockSender ────────────────────────────────────────────────────────────────

class MockSender:
    """Records send_command calls for assertion."""

    def __init__(self) -> None:
        self.calls: list[tuple[str, dict]] = []

    def send_command(self, type_id: str, data: dict) -> None:
        self.calls.append((type_id, data))

    def close(self) -> None:
        pass


# ── Per-test module fixture ───────────────────────────────────────────────────

@pytest.fixture
def mod(qapp: QApplication) -> GmDiceModule:
    """Fresh GmDiceModule with widget built and MockSender injected."""
    m = GmDiceModule()
    m.widget()
    sender = MockSender()
    m.set_sender(sender)
    return m


# ── Helper ────────────────────────────────────────────────────────────────────

def _roll_msg(dice: list[int], total: int) -> dict:
    return {
        "typeId": "gmAlea.dice.roll_result",
        "headers": {"data": json.dumps({"dice": dice, "total": total})},
    }


# ── Tests ─────────────────────────────────────────────────────────────────────

def test_roll_button_sends_command(mod: GmDiceModule) -> None:
    """Clicking [LANCIA] calls send_command('gmAlea.dice.roll_request', ...)."""
    mod._roll_btn.click()
    sender: MockSender = mod._sender  # type: ignore[assignment]
    assert len(sender.calls) == 1
    type_id, payload = sender.calls[0]
    assert type_id == "gmAlea.dice.roll_request"
    assert "count" in payload
    assert "faces" in payload


def test_roll_result_updates_labels(mod: GmDiceModule) -> None:
    """roll_result with dice=[3,5,2] sets total label to '10' and detail to '3 + 5 + 2'."""
    mod.on_envelope(_roll_msg([3, 5, 2], 10))
    assert mod._result_label.text() == "10"
    assert mod._detail_label.text() == "3 + 5 + 2"


def test_history_grows_after_roll(mod: GmDiceModule) -> None:
    """After one roll the history list contains exactly one entry."""
    mod._history_list.clear()
    mod.on_envelope(_roll_msg([4], 4))
    assert mod._history_list.count() == 1


def test_history_capped_at_ten(mod: GmDiceModule) -> None:
    """History list never exceeds 10 entries regardless of how many rolls occur."""
    mod._history_list.clear()
    for i in range(11):
        mod.on_envelope(_roll_msg([i + 1], i + 1))
    assert mod._history_list.count() == 10

