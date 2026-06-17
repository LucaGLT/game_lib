"""Tests for MainWindow central envelope routing.

All tests run in offscreen mode (no display required).
A ``MockModule`` replaces real modules to isolate routing logic from widget
construction.
"""
from __future__ import annotations

import os
import sys

import pytest

# Force Qt offscreen platform before any PySide6 import.
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication, QLabel

from gmGui.modules.base_module import BaseModule


# ── Shared QApplication fixture ───────────────────────────────────────────────

@pytest.fixture(scope="module")
def qapp():
    """Single QApplication instance shared across all tests in this module."""
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


# ── Mock module ───────────────────────────────────────────────────────────────

class MockModule(BaseModule):
    """Minimal module that records received envelopes for assertion."""

    def __init__(self, mid: str, type_ids: list[str]) -> None:
        super().__init__()
        self._module_id = mid
        self._type_ids = type_ids
        self.received: list[dict] = []

    @property
    def module_id(self) -> str:
        return self._module_id

    @property
    def title(self) -> str:
        return f"Mock:{self._module_id}"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.LeftDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return list(self._type_ids)

    def _build_widget(self) -> QLabel:
        return QLabel(self._module_id)

    def on_envelope(self, msg: dict) -> None:
        self.received.append(msg)


# ── Helpers ───────────────────────────────────────────────────────────────────

def _make_window_with_mocks(
    qapp: QApplication,
    *mock_modules: MockModule,
) -> "MainWindowWithMocks":
    """Returns a MainWindow whose module list is replaced by *mock_modules*.

    Bypasses ``_register_modules`` to avoid creating real module widgets.
    """
    from gmGui.main_window import MainWindow

    win = MainWindow.__new__(MainWindow)
    # Minimal QMainWindow initialisation (no bridge, no receiver.start()).
    from PySide6.QtWidgets import QMainWindow
    QMainWindow.__init__(win)
    win.setDockNestingEnabled(True)
    win._receiver = _DummyReceiver()
    win._sender = _DummySender()
    win._modules = []
    win._routing = {}
    win._docks = {}

    central = QLabel("test")
    win.setCentralWidget(central)
    win._conn_label = QLabel("test-status")

    for mod in mock_modules:
        mod.set_sender(win._sender)
        for tid in mod.subscribed_type_ids():
            win._routing.setdefault(tid, []).append(mod)
        win._add_dock(mod)
        win._modules.append(mod)

    return win


class _DummyReceiver:
    """Minimal stand-in that prevents MainWindow from starting a real thread."""
    def stop(self) -> None: pass


class _DummySender:
    """Minimal stand-in for EngineSender."""
    def send_command(self, type_id: str, data: dict) -> None: pass
    def close(self) -> None: pass


# ── Tests ─────────────────────────────────────────────────────────────────────

def test_routing_table_built_from_subscribed_type_ids(qapp: QApplication) -> None:
    """Every typeId declared by a module appears in _routing."""
    m = MockModule("m1", ["lib.event_a", "lib.event_b"])
    win = _make_window_with_mocks(qapp, m)

    assert "lib.event_a" in win._routing
    assert "lib.event_b" in win._routing
    assert win._routing["lib.event_a"] == [m]
    assert win._routing["lib.event_b"] == [m]


def test_on_envelope_dispatches_to_correct_module(qapp: QApplication) -> None:
    """_on_envelope routes a dict to exactly the module subscribed to its typeId."""
    m1 = MockModule("m1", ["lib.ping"])
    m2 = MockModule("m2", ["lib.pong"])
    win = _make_window_with_mocks(qapp, m1, m2)

    win._on_envelope({"typeId": "lib.ping", "data": {}})

    assert len(m1.received) == 1
    assert m1.received[0]["typeId"] == "lib.ping"
    assert len(m2.received) == 0


def test_unknown_type_id_is_silently_ignored(qapp: QApplication) -> None:
    """_on_envelope with an unregistered typeId does not raise."""
    m = MockModule("m1", ["lib.known"])
    win = _make_window_with_mocks(qapp, m)

    # Must not raise.
    win._on_envelope({"typeId": "lib.unknown_event", "data": {}})
    assert len(m.received) == 0


def test_multi_module_same_type_id_all_receive(qapp: QApplication) -> None:
    """If two modules subscribe to the same typeId, both receive the envelope."""
    m1 = MockModule("m1", ["shared.event"])
    m2 = MockModule("m2", ["shared.event"])
    win = _make_window_with_mocks(qapp, m1, m2)

    msg = {"typeId": "shared.event", "data": {"val": 42}}
    win._on_envelope(msg)

    assert len(m1.received) == 1
    assert len(m2.received) == 1
    assert m1.received[0] is msg
    assert m2.received[0] is msg


def test_envelope_delivered_unmodified(qapp: QApplication) -> None:
    """The exact dict passed to _on_envelope reaches the module unchanged."""
    m = MockModule("m1", ["lib.evt"])
    win = _make_window_with_mocks(qapp, m)

    original = {"typeId": "lib.evt", "source": "CoreEngine", "data": {"x": 1}}
    win._on_envelope(original)

    assert m.received[0] is original


def test_status_bar_updates_to_connected_on_first_envelope(
    qapp: QApplication,
) -> None:
    """_on_envelope sets the connection label to 'Connesso' on first call."""
    from gmGui.main_window import _STATUS_CONNECTED, _STATUS_DISCONNECTED

    m = MockModule("m1", ["lib.evt"])
    win = _make_window_with_mocks(qapp, m)
    win._conn_label.setText(_STATUS_DISCONNECTED)

    win._on_envelope({"typeId": "lib.evt", "data": {}})

    assert win._conn_label.text() == _STATUS_CONNECTED


def test_on_connection_lost_resets_status_bar(qapp: QApplication) -> None:
    """_on_connection_lost sets the connection label back to 'Disconnesso'."""
    from gmGui.main_window import _STATUS_CONNECTED, _STATUS_DISCONNECTED

    m = MockModule("m1", ["lib.evt"])
    win = _make_window_with_mocks(qapp, m)
    win._conn_label.setText(_STATUS_CONNECTED)

    win._on_connection_lost()

    assert win._conn_label.text() == _STATUS_DISCONNECTED


def test_five_docks_created_in_full_window(qapp: QApplication) -> None:
    """MainWindow creates exactly 5 QDockWidgets (one per real module)."""
    from unittest.mock import patch
    from gmGui.main_window import MainWindow
    from gmGui.engine_bridge.receiver import EngineReceiver

    with patch.object(EngineReceiver, 'start'):
        win = MainWindow()
    try:
        assert set(win._docks.keys()) == {
            "gm_flow", "gm_map", "gm_actor", "gm_comp_deck", "gm_dice"
        }
    finally:
        win.close()


def test_view_menu_has_toggle_action_per_dock(qapp: QApplication) -> None:
    """The View menu contains one action for each registered dock widget."""
    from unittest.mock import patch
    from gmGui.main_window import MainWindow
    from gmGui.engine_bridge.receiver import EngineReceiver

    with patch.object(EngineReceiver, 'start'):
        win = MainWindow()
    try:
        # Access _view_menu directly: QAction::menu() returns a temporary
        # Python wrapper that PySide6 may GC before use (shiboken ownership bug).
        view_menu = win._view_menu
        assert view_menu is not None
        action_texts = {a.text() for a in view_menu.actions()}
        # Each dock's toggleViewAction uses the dock title as action text.
        for dock in win._docks.values():
            assert dock.windowTitle() in action_texts
    finally:
        win.close()
