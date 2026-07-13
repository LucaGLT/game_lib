/**
 * @file tests/test_SequenceEngine.cpp
 * @brief Unit tests for SequenceEngine and related types.
 *
 * Covers:
 *   - can_play: all 10 (state × CardType) combinations from the spec table
 *   - advance: state transitions for every CardType
 *   - is_turn_ending: all five CardTypes
 *   - interrupt: fields set correctly
 *   - reset: returns default state
 *   - INSTANT invariance: does not change active or last_type
 *   - interrupted state blocks all can_play
 *   - cards_played counter increments for every type
 *
 * Build (from game_lib root):
 *   g++ -std=c++17 -I. gmAlea/SequenceEngine.cpp \
 *       gmAlea/tests/test_SequenceEngine.cpp -o test_SequenceEngine \
 *       && ./test_SequenceEngine
 */

#include "gmAlea/SequenceEngine.hpp"
#include "gmAlea/CardType.hpp"
#include "gmAlea/SequenceState.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace gmAlea;

// ── Helpers ───────────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " — " << reason << "\n";
	++g_fail;
}

// ── Tests: can_play — sequence NOT active ────────────────────────────────────

static void test_can_play_inactive()
{
	const std::string G = "can_play/inactive/";
	SequenceEngine engine;
	SequenceState  s;  // active == false, interrupted == false

	engine.can_play(CardType::SINGLE, s)
		? pass(G + "SINGLE=true")
		: fail(G + "SINGLE=true", "expected true");

	engine.can_play(CardType::SEQ_START, s)
		? pass(G + "SEQ_START=true")
		: fail(G + "SEQ_START=true", "expected true");

	!engine.can_play(CardType::SEQ_CONTINUE, s)
		? pass(G + "SEQ_CONTINUE=false")
		: fail(G + "SEQ_CONTINUE=false", "expected false");

	!engine.can_play(CardType::SEQ_END, s)
		? pass(G + "SEQ_END=false")
		: fail(G + "SEQ_END=false", "expected false");

	engine.can_play(CardType::INSTANT, s)
		? pass(G + "INSTANT=true")
		: fail(G + "INSTANT=true", "expected true");
}

// ── Tests: can_play — sequence ACTIVE ────────────────────────────────────────

static void test_can_play_active()
{
	const std::string G = "can_play/active/";
	SequenceEngine engine;
	SequenceState  s;
	s.active    = true;
	s.last_type = CardType::SEQ_START;

	!engine.can_play(CardType::SINGLE, s)
		? pass(G + "SINGLE=false")
		: fail(G + "SINGLE=false", "expected false");

	!engine.can_play(CardType::SEQ_START, s)
		? pass(G + "SEQ_START=false")
		: fail(G + "SEQ_START=false", "expected false");

	engine.can_play(CardType::SEQ_CONTINUE, s)
		? pass(G + "SEQ_CONTINUE=true")
		: fail(G + "SEQ_CONTINUE=true", "expected true");

	engine.can_play(CardType::SEQ_END, s)
		? pass(G + "SEQ_END=true")
		: fail(G + "SEQ_END=true", "expected true");

	engine.can_play(CardType::INSTANT, s)
		? pass(G + "INSTANT=true")
		: fail(G + "INSTANT=true", "expected true");
}

// ── Tests: can_play — INTERRUPTED blocks all ─────────────────────────────────

static void test_can_play_interrupted()
{
	const std::string G = "can_play/interrupted/";
	SequenceEngine engine;
	SequenceState  s;
	s.interrupted = true;

	bool any_ok = engine.can_play(CardType::SINGLE, s)
	           || engine.can_play(CardType::SEQ_START, s)
	           || engine.can_play(CardType::SEQ_CONTINUE, s)
	           || engine.can_play(CardType::SEQ_END, s)
	           || engine.can_play(CardType::INSTANT, s);

	!any_ok
		? pass(G + "all_blocked")
		: fail(G + "all_blocked", "expected all false when interrupted");
}

// ── Tests: advance transitions ────────────────────────────────────────────────

static void test_advance_single()
{
	const std::string G = "advance/SINGLE/";
	SequenceEngine engine;
	SequenceState  s;
	SequenceState  n = engine.advance(CardType::SINGLE, s);

	!n.active && n.last_type == CardType::SINGLE && n.cards_played == 1 && !n.interrupted
		? pass(G + "fields_correct")
		: fail(G + "fields_correct", "unexpected field values");
}

static void test_advance_seq_start()
{
	const std::string G = "advance/SEQ_START/";
	SequenceEngine engine;
	SequenceState  s;
	SequenceState  n = engine.advance(CardType::SEQ_START, s);

	n.active && n.last_type == CardType::SEQ_START && n.cards_played == 1 && !n.interrupted
		? pass(G + "fields_correct")
		: fail(G + "fields_correct", "unexpected field values");
}

static void test_advance_seq_continue()
{
	const std::string G = "advance/SEQ_CONTINUE/";
	SequenceEngine engine;
	SequenceState  s;
	s.active    = true;
	s.last_type = CardType::SEQ_START;
	s.cards_played = 1;

	SequenceState n = engine.advance(CardType::SEQ_CONTINUE, s);

	n.active && n.last_type == CardType::SEQ_CONTINUE && n.cards_played == 2
		? pass(G + "fields_correct")
		: fail(G + "fields_correct", "unexpected field values");
}

static void test_advance_seq_end()
{
	const std::string G = "advance/SEQ_END/";
	SequenceEngine engine;
	SequenceState  s;
	s.active       = true;
	s.last_type    = CardType::SEQ_CONTINUE;
	s.cards_played = 2;

	SequenceState n = engine.advance(CardType::SEQ_END, s);

	!n.active && n.last_type == CardType::SEQ_END && n.cards_played == 3
		? pass(G + "fields_correct")
		: fail(G + "fields_correct", "unexpected field values");
}

static void test_advance_instant_does_not_change_active()
{
	const std::string G = "advance/INSTANT/";
	SequenceEngine engine;

	// INSTANT inside an active sequence — active must not change
	SequenceState  s;
	s.active       = true;
	s.last_type    = CardType::SEQ_START;
	s.cards_played = 1;

	SequenceState n = engine.advance(CardType::INSTANT, s);

	n.active && n.last_type == CardType::SEQ_START && n.cards_played == 2
		? pass(G + "active_preserved")
		: fail(G + "active_preserved", "INSTANT must not change active or last_type");

	// INSTANT outside a sequence — active must stay false
	SequenceState  s2;
	SequenceState  n2 = engine.advance(CardType::INSTANT, s2);

	!n2.active && n2.last_type == CardType::SINGLE && n2.cards_played == 1
		? pass(G + "inactive_preserved")
		: fail(G + "inactive_preserved", "INSTANT must not activate sequence");
}

// ── Tests: is_turn_ending ─────────────────────────────────────────────────────

static void test_is_turn_ending()
{
	const std::string G = "is_turn_ending/";
	SequenceEngine engine;
	SequenceState  s;

	engine.is_turn_ending(CardType::SINGLE, s)
		? pass(G + "SINGLE=true")
		: fail(G + "SINGLE=true", "expected true");

	!engine.is_turn_ending(CardType::SEQ_START, s)
		? pass(G + "SEQ_START=false")
		: fail(G + "SEQ_START=false", "expected false");

	!engine.is_turn_ending(CardType::SEQ_CONTINUE, s)
		? pass(G + "SEQ_CONTINUE=false")
		: fail(G + "SEQ_CONTINUE=false", "expected false");

	engine.is_turn_ending(CardType::SEQ_END, s)
		? pass(G + "SEQ_END=true")
		: fail(G + "SEQ_END=true", "expected true");

	!engine.is_turn_ending(CardType::INSTANT, s)
		? pass(G + "INSTANT=false")
		: fail(G + "INSTANT=false", "expected false");
}

// ── Tests: interrupt ──────────────────────────────────────────────────────────

static void test_interrupt()
{
	const std::string G = "interrupt/";
	SequenceEngine engine;
	SequenceState  s;
	s.active       = true;
	s.last_type    = CardType::SEQ_CONTINUE;
	s.cards_played = 3;

	SequenceState n = engine.interrupt(s);

	!n.active && n.interrupted && n.cards_played == 3
	&& n.last_type == CardType::SEQ_CONTINUE
		? pass(G + "fields_correct")
		: fail(G + "fields_correct", "interrupt did not set fields correctly");
}

// ── Tests: reset ──────────────────────────────────────────────────────────────

static void test_reset()
{
	const std::string G = "reset/";
	SequenceEngine engine;
	SequenceState  dirty;
	dirty.active       = true;
	dirty.interrupted  = true;
	dirty.cards_played = 99;

	SequenceState n = engine.reset();

	!n.active && !n.interrupted && n.cards_played == 0
		? pass(G + "returns_default")
		: fail(G + "returns_default", "reset did not return default state");
}

// ── Tests: full sequence round-trip ──────────────────────────────────────────

static void test_full_sequence_roundtrip()
{
	const std::string G = "roundtrip/";
	SequenceEngine engine;
	SequenceState  s = engine.reset();

	// Play: SEQ_START → SEQ_CONTINUE → SEQ_CONTINUE → SEQ_END
	s = engine.advance(CardType::SEQ_START, s);
	s = engine.advance(CardType::SEQ_CONTINUE, s);
	s = engine.advance(CardType::SEQ_CONTINUE, s);

	engine.can_play(CardType::SEQ_END, s)
		? pass(G + "can_play_seq_end_after_2_continues")
		: fail(G + "can_play_seq_end_after_2_continues", "expected true");

	s = engine.advance(CardType::SEQ_END, s);

	!s.active && s.cards_played == 4 && s.last_type == CardType::SEQ_END
		? pass(G + "final_state_correct")
		: fail(G + "final_state_correct", "unexpected final state");

	engine.is_turn_ending(CardType::SEQ_END, s)
		? pass(G + "turn_ends_on_seq_end")
		: fail(G + "turn_ends_on_seq_end", "expected true");
}

// ── Tests: advance clears interrupted flag ────────────────────────────────────

static void test_advance_clears_interrupted()
{
	const std::string G = "advance/clears_interrupted/";
	SequenceEngine engine;
	SequenceState  s;
	s.interrupted = true;

	// advance() does not validate legality — it clears interrupted
	SequenceState n = engine.advance(CardType::SINGLE, s);

	!n.interrupted
		? pass(G + "cleared")
		: fail(G + "cleared", "advance should clear interrupted flag");
}

// ── Tests: card_type_name ─────────────────────────────────────────────────────

static void test_card_type_name()
{
	const std::string G = "card_type_name/";
	bool ok = std::string(card_type_name(CardType::SINGLE))       == "SINGLE"
	       && std::string(card_type_name(CardType::SEQ_START))    == "SEQ_START"
	       && std::string(card_type_name(CardType::SEQ_CONTINUE)) == "SEQ_CONTINUE"
	       && std::string(card_type_name(CardType::SEQ_END))      == "SEQ_END"
	       && std::string(card_type_name(CardType::INSTANT))      == "INSTANT";

	ok ? pass(G + "all_names") : fail(G + "all_names", "name mismatch");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== SequenceEngine unit tests ===\n";

	test_can_play_inactive();
	test_can_play_active();
	test_can_play_interrupted();
	test_advance_single();
	test_advance_seq_start();
	test_advance_seq_continue();
	test_advance_seq_end();
	test_advance_instant_does_not_change_active();
	test_is_turn_ending();
	test_interrupt();
	test_reset();
	test_full_sequence_roundtrip();
	test_advance_clears_interrupted();
	test_card_type_name();

	std::cout << "\n=== Results: " << g_pass << " passed, " << g_fail << " failed ===\n";
	return g_fail == 0 ? 0 : 1;
}
