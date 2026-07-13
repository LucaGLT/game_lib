"""Tests for GmCompDeckModule and ZoneList — Phase 6 implementation.

All tests run in offscreen mode (no display required).
A ``MockSender`` captures ``send_command`` calls so tests can assert on
the command stream without a real TCP connection.
"""
from __future__ import annotations

import os
import sys

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QAbstractItemView, QApplication

from gmGui.modules.gm_comp_deck_module import GmCompDeckModule
from gmGui.widgets.zone_list import ZoneList


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
def mod(qapp: QApplication) -> GmCompDeckModule:
    """Fresh GmCompDeckModule with widget built and MockSender injected."""
    m = GmCompDeckModule()
    m.widget()
    sender = MockSender()
    m.set_sender(sender)
    return m


def _sender(mod: GmCompDeckModule) -> MockSender:
    return mod._sender  # type: ignore[return-value]


# ── zone_changed tests ────────────────────────────────────────────────────────

def test_zone_changed_updates_counter(mod: GmCompDeckModule) -> None:
    """gmAlea.deck.zone_changed with 3 cards sets the counter label to '3 carte'."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "c1", "name": "Fireball"},
                {"card_id": "c2", "name": "Shield"},
                {"card_id": "c3", "name": "Arrow"},
            ],
        },
    })

    assert "3" in mod._counters["MainDeck"].text()


def test_zone_changed_populates_list(mod: GmCompDeckModule) -> None:
    """zone_changed populates the ZoneList with the correct number of items."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [
                {"card_id": "h1", "name": "Potion"},
                {"card_id": "h2", "name": "Sword"},
            ],
        },
    })

    assert mod._zone_lists["CardHand"].count() == 2


def test_zone_changed_stores_card_id_in_userrole(mod: GmCompDeckModule) -> None:
    """zone_changed items carry the card_id in Qt.ItemDataRole.UserRole."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "PlayArea",
            "cards": [{"card_id": "p_001", "name": "Dragon"}],
        },
    })

    item = mod._zone_lists["PlayArea"].item(0)
    assert item.data(Qt.ItemDataRole.UserRole) == "p_001"


def test_zone_changed_clears_previous_cards(mod: GmCompDeckModule) -> None:
    """A second zone_changed replaces all existing items in the zone."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [{"card_id": "old", "name": "Old"}],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "new1", "name": "New1"},
                {"card_id": "new2", "name": "New2"},
            ],
        },
    })

    assert mod._zone_lists["DiscardPile"].count() == 2
    assert mod._zone_lists["DiscardPile"].item(0).data(Qt.ItemDataRole.UserRole) == "new1"


# ── card_moved tests ──────────────────────────────────────────────────────────

def test_card_moved_updates_both_zone_counters(mod: GmCompDeckModule) -> None:
    """gmAlea.deck.card_moved from MainDeck to CardHand updates both counters."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [{"card_id": "m1", "name": "Fireball"}],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.card_moved",
        "data": {"card_id": "m1", "from_zone": "MainDeck", "to_zone": "CardHand"},
    })

    assert mod._zone_lists["MainDeck"].count() == 0
    assert mod._zone_lists["CardHand"].count() == 1
    assert "0" in mod._counters["MainDeck"].text()
    assert "1" in mod._counters["CardHand"].text()


def test_card_moved_item_is_in_dest_zone(mod: GmCompDeckModule) -> None:
    """After card_moved the card item exists in the destination zone."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [{"card_id": "fire_01", "name": "Fireball"}],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.card_moved",
        "data": {"card_id": "fire_01", "from_zone": "MainDeck", "to_zone": "PlayArea"},
    })

    found = GmCompDeckModule._find_item(mod._zone_lists["PlayArea"], "fire_01")
    assert found is not None


def test_card_moved_inserts_on_top(mod: GmCompDeckModule) -> None:
    """Moved card is inserted at index 0 (top of destination pile)."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [
                {"card_id": "h1", "name": "H1"},
                {"card_id": "h2", "name": "H2"},
            ],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [{"card_id": "m1", "name": "M1"}],
        },
    })

    mod.on_envelope({
        "typeId": "gmAlea.deck.card_moved",
        "data": {"card_id": "m1", "from_zone": "MainDeck", "to_zone": "CardHand"},
    })

    top_item = mod._zone_lists["CardHand"].item(0)
    assert top_item.data(Qt.ItemDataRole.UserRole) == "m1"


def test_card_moved_unknown_card_does_not_crash(mod: GmCompDeckModule) -> None:
    """card_moved with a card_id not in the source zone must not raise."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.card_moved",
        "data": {"card_id": "ghost_card", "from_zone": "MainDeck", "to_zone": "CardHand"},
    })
    # No assertion — just must not raise


# ── drawn tests ───────────────────────────────────────────────────────────────

def test_drawn_moves_card_from_main_to_hand(mod: GmCompDeckModule) -> None:
    """gmAlea.deck.drawn is equivalent to card_moved MainDeck→CardHand."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [{"card_id": "draw_me", "name": "Lucky Card"}],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.drawn",
        "data": {"card_id": "draw_me"},
    })

    assert mod._zone_lists["MainDeck"].count() == 0
    assert mod._zone_lists["CardHand"].count() == 1


# ── shuffled tests ────────────────────────────────────────────────────────────

def test_shuffled_preserves_card_count(mod: GmCompDeckModule) -> None:
    """gmAlea.deck.shuffled does not change the item count in the zone."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "s1", "name": "A"},
                {"card_id": "s2", "name": "B"},
            ],
        },
    })
    mod.on_envelope({
        "typeId": "gmAlea.deck.shuffled",
        "data": {"zone_name": "MainDeck"},
    })

    assert mod._zone_lists["MainDeck"].count() == 2


# ── command button tests ──────────────────────────────────────────────────────

def test_draw_button_sends_command(mod: GmCompDeckModule) -> None:
    """Blind draw sends gmAlea.deck.move_card(MainDeck -> CardHand) for first card."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "m1", "name": "A"},
                {"card_id": "m2", "name": "B"},
            ],
        },
    })
    _sender(mod).calls.clear()
    mod._btn_draw.click()

    assert len(_sender(mod).calls) == 1
    type_id, data = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.move_card"
    assert data["card_id"] == "m1"
    assert data["from"] == "MainDeck"
    assert data["to"] == "CardHand"


def test_draw_choose_mode_moves_selected_main_deck_card(mod: GmCompDeckModule) -> None:
    """After observe toggle, draw button becomes 'Scegli' and moves selected card."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "m1", "name": "A"},
                {"card_id": "m2", "name": "B"},
            ],
        },
    })
    mod._btn_observe_main.click()
    mod._zone_lists["MainDeck"].setCurrentRow(1)
    _sender(mod).calls.clear()

    mod._btn_draw.click()

    assert len(_sender(mod).calls) == 1
    type_id, data = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.move_card"
    assert data["card_id"] == "m2"
    assert data["from"] == "MainDeck"
    assert data["to"] == "CardHand"


def test_discard_pick_revealed_moves_selected_to_hand(mod: GmCompDeckModule) -> None:
    """Scarti in osservato mode uses pick button as 'Scegli'."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "d1", "name": "A"},
                {"card_id": "d2", "name": "B"},
            ],
        },
    })
    mod._btn_observe_discard.click()
    mod._zone_lists["DiscardPile"].setCurrentRow(1)
    _sender(mod).calls.clear()

    mod._btn_discard_pick.click()

    assert len(_sender(mod).calls) == 1
    type_id, data = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.move_card"
    assert data["card_id"] == "d2"
    assert data["from"] == "DiscardPile"
    assert data["to"] == "CardHand"


def test_discard_pick_hidden_moves_first_to_hand(mod: GmCompDeckModule) -> None:
    """Scarti in nascosto mode uses pick button as 'Prendi la Prima'."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "d1", "name": "A"},
                {"card_id": "d2", "name": "B"},
            ],
        },
    })
    _sender(mod).calls.clear()

    mod._btn_discard_pick.click()

    assert len(_sender(mod).calls) == 1
    type_id, data = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.move_card"
    assert data["card_id"] == "d1"
    assert data["from"] == "DiscardPile"
    assert data["to"] == "CardHand"


def test_discard_pick_label_changes_with_observe_toggle(mod: GmCompDeckModule) -> None:
    """Discard pick button label toggles between first-pick and choose."""
    assert mod._btn_discard_pick.text() == "Prendi la Prima"

    mod._btn_observe_discard.click()
    assert mod._btn_discard_pick.text() == "Scegli"

    mod._btn_observe_discard.click()
    assert mod._btn_discard_pick.text() == "Prendi la Prima"


def test_hand_action_buttons_send_expected_zone_moves(mod: GmCompDeckModule) -> None:
    """Hand buttons map to discard/play/memory/banish for selected hand card."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [{"card_id": "h1", "name": "HandCard"}],
        },
    })
    mod._zone_lists["CardHand"].setCurrentRow(0)

    _sender(mod).calls.clear()
    mod._btn_hand_discard.click()
    mod._btn_hand_play.click()
    mod._btn_hand_memory.click()
    mod._btn_hand_banish.click()

    assert len(_sender(mod).calls) == 4
    sent = [call[1]["to"] for call in _sender(mod).calls]
    assert sent == ["DiscardPile", "PlayArea", "Memory", "BanishZone"]


def test_playarea_buttons_send_expected_zone_moves(mod: GmCompDeckModule) -> None:
    """PlayArea buttons map to discard and hand for selected played card."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "PlayArea",
            "cards": [{"card_id": "p1", "name": "Played"}],
        },
    })
    mod._zone_lists["PlayArea"].setCurrentRow(0)

    _sender(mod).calls.clear()
    mod._btn_play_discard.click()
    mod._btn_play_retake.click()

    assert len(_sender(mod).calls) == 2
    assert _sender(mod).calls[0][1]["to"] == "DiscardPile"
    assert _sender(mod).calls[1][1]["to"] == "CardHand"


def test_memory_riprendi_moves_selected_to_hand(mod: GmCompDeckModule) -> None:
    """Memoria/Riprendi moves selected memory card to hand."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "Memory",
            "cards": [{"card_id": "mem1", "name": "MemoryCard"}],
        },
    })
    mod._zone_lists["Memory"].setCurrentRow(0)

    _sender(mod).calls.clear()
    mod._btn_memory_retake.click()

    assert len(_sender(mod).calls) == 1
    assert _sender(mod).calls[0][0] == "gmAlea.deck.move_card"
    assert _sender(mod).calls[0][1]["from"] == "Memory"
    assert _sender(mod).calls[0][1]["to"] == "CardHand"


def test_discard_rimescola_button_sends_command(mod: GmCompDeckModule) -> None:
    """Scarti/Rimescola sends gmAlea.deck.recycle_discard command."""
    _sender(mod).calls.clear()
    mod._btn_discard_shuffle.click()

    assert len(_sender(mod).calls) == 1
    type_id, _ = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.recycle_discard"


# ── drag-drop signal tests ────────────────────────────────────────────────────

def test_card_dropped_signal_calls_send_command(mod: GmCompDeckModule) -> None:
    """Allowed drag from MainDeck to CardHand sends move command."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [{"card_id": "c_abc", "name": "First"}],
        },
    })
    _sender(mod).calls.clear()
    mod._zone_lists["MainDeck"].card_dropped.emit(
        "c_abc", "MainDeck", "CardHand"
    )

    assert len(_sender(mod).calls) == 1
    type_id, data = _sender(mod).calls[0]
    assert type_id == "gmAlea.deck.move_card"
    assert data["card_id"] == "c_abc"
    assert data["from"] == "MainDeck"
    assert data["to"] == "CardHand"


def test_drag_rejects_forbidden_source_zone(mod: GmCompDeckModule) -> None:
    """Drag from PlayArea is rejected by the policy matrix."""
    _sender(mod).calls.clear()
    mod._zone_lists["PlayArea"].card_dropped.emit(
        "c_xyz", "PlayArea", "DiscardPile"
    )

    assert len(_sender(mod).calls) == 0


def test_drag_hidden_main_deck_allows_only_first(mod: GmCompDeckModule) -> None:
    """When MainDeck is hidden, only the first card can be dragged to hand."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "MainDeck",
            "cards": [
                {"card_id": "m1", "name": "A"},
                {"card_id": "m2", "name": "B"},
            ],
        },
    })

    _sender(mod).calls.clear()
    mod._zone_lists["MainDeck"].card_dropped.emit("m2", "MainDeck", "CardHand")
    assert len(_sender(mod).calls) == 0

    mod._zone_lists["MainDeck"].card_dropped.emit("m1", "MainDeck", "CardHand")
    assert len(_sender(mod).calls) == 1


def test_drag_hidden_discard_allows_only_first(mod: GmCompDeckModule) -> None:
    """When DiscardPile is hidden, only first card may drag to hand/main deck."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "d1", "name": "A"},
                {"card_id": "d2", "name": "B"},
            ],
        },
    })

    _sender(mod).calls.clear()
    mod._zone_lists["DiscardPile"].card_dropped.emit("d2", "DiscardPile", "CardHand")
    assert len(_sender(mod).calls) == 0

    mod._zone_lists["DiscardPile"].card_dropped.emit("d1", "DiscardPile", "MainDeck")
    assert len(_sender(mod).calls) == 1


def test_drag_discard_to_main_allowed_when_visible(mod: GmCompDeckModule) -> None:
    """Visible DiscardPile allows dragging any selected card to MainDeck."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "d1", "name": "A"},
                {"card_id": "d2", "name": "B"},
            ],
        },
    })
    mod._btn_observe_discard.click()

    _sender(mod).calls.clear()
    mod._zone_lists["DiscardPile"].card_dropped.emit("d2", "DiscardPile", "MainDeck")

    assert len(_sender(mod).calls) == 1
    _, data = _sender(mod).calls[0]
    assert data["from"] == "DiscardPile"
    assert data["to"] == "MainDeck"


def test_drag_from_hand_allows_same_targets_as_hand_buttons(mod: GmCompDeckModule) -> None:
    """Hand drag targets match button actions: discard/play/memory/banish."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [{"card_id": "h1", "name": "H"}],
        },
    })

    _sender(mod).calls.clear()
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "DiscardPile")
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "PlayArea")
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "Memory")
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "BanishZone")

    assert len(_sender(mod).calls) == 4
    targets = [call[1]["to"] for call in _sender(mod).calls]
    assert targets == ["DiscardPile", "PlayArea", "Memory", "BanishZone"]


def test_drag_from_hand_rejects_non_button_targets(mod: GmCompDeckModule) -> None:
    """Hand drag rejects targets not available from hand buttons."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [{"card_id": "h1", "name": "H"}],
        },
    })

    _sender(mod).calls.clear()
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "MainDeck")
    mod._zone_lists["CardHand"].card_dropped.emit("h1", "CardHand", "CardHand")

    assert len(_sender(mod).calls) == 0


def test_discard_first_card_always_visible_when_hidden(mod: GmCompDeckModule) -> None:
    """In hidden discard mode, first card keeps real name while others are masked."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "DiscardPile",
            "cards": [
                {"card_id": "d1", "name": "FirstVisible"},
                {"card_id": "d2", "name": "SecondHidden"},
            ],
        },
    })

    assert mod._zone_lists["DiscardPile"].item(0).text() == "FirstVisible"
    assert mod._zone_lists["DiscardPile"].item(1).text() == "Carta_2"


# ── BanishZone policy tests ───────────────────────────────────────────────────

def test_banish_zone_rejects_drop(mod: GmCompDeckModule) -> None:
    """BanishZone has DragDropMode=NoDragDrop; dropping onto it is not allowed."""
    banish = mod._zone_lists["BanishZone"]

    assert banish.dragDropMode() == QAbstractItemView.DragDropMode.NoDragDrop


def test_other_zones_accept_drag_drop(mod: GmCompDeckModule) -> None:
    """All zones except BanishZone have DragDrop mode enabled."""
    for zone_name in ("MainDeck", "CardHand", "PlayArea", "DiscardPile"):
        mode = mod._zone_lists[zone_name].dragDropMode()
        assert mode == QAbstractItemView.DragDropMode.DragDrop, (
            f"{zone_name} should have DragDrop mode, got {mode}"
        )


# ── ZoneList standalone tests ─────────────────────────────────────────────────

def test_zone_list_zone_name_stored(qapp: QApplication) -> None:
    """ZoneList._zone_name matches the name passed to the constructor."""
    zl = ZoneList("TestZone")

    assert zl._zone_name == "TestZone"


def test_zone_list_card_dropped_signal_emitted(qapp: QApplication) -> None:
    """card_dropped signal carries (card_id, from_zone, to_zone) correctly."""
    received: list[tuple[str, str, str]] = []
    zl = ZoneList("TargetZone")
    zl.card_dropped.connect(lambda a, b, c: received.append((a, b, c)))

    zl.card_dropped.emit("card_99", "SourceZone", "TargetZone")

    assert received == [("card_99", "SourceZone", "TargetZone")]


def test_zone_list_default_mode_is_dragdrop(qapp: QApplication) -> None:
    """ZoneList default DragDropMode is DragDrop."""
    zl = ZoneList("AnyZone")

    assert zl.dragDropMode() == QAbstractItemView.DragDropMode.DragDrop


# ── Counter edge cases ────────────────────────────────────────────────────────

def test_counter_singular_for_one_card(mod: GmCompDeckModule) -> None:
    """Counter shows '1 carta' (singular) when the zone has exactly one card."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {
            "zone_name": "CardHand",
            "cards": [{"card_id": "solo", "name": "Lone Card"}],
        },
    })

    assert mod._counters["CardHand"].text() == "1 carta"


def test_counter_plural_for_zero_cards(mod: GmCompDeckModule) -> None:
    """Counter shows '0 carte' (plural) when the zone is empty."""
    mod.on_envelope({
        "typeId": "gmAlea.deck.zone_changed",
        "data": {"zone_name": "PlayArea", "cards": []},
    })

    assert mod._counters["PlayArea"].text() == "0 carte"
