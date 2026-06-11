/**
 * @file tests/test_gmCompDeck.cpp
 * @brief Integration tests for gmCompDeck — cross-zone moves and invariant enforcement.
 *
 * Build (from game_lib root):
 *   g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/gmCompDeck.cpp \
 *       gmDeck/tests/test_gmCompDeck.cpp -o test_gmCompDeck && ./test_gmCompDeck
 */

#include "gmDeck/gmCompDeck.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace gmFate;

// ── Helpers ───────────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name) {
    std::cout << "[PASS] " << name << "\n";
    ++g_pass;
}

static void fail(const std::string& name, const std::string& reason) {
    std::cout << "[FAIL] " << name << " — " << reason << "\n";
    ++g_fail;
}

// Helper: verify total_count equals expected
static bool check_total(const gmCompDeck& deck, int expected) {
    return deck.total_count() == expected;
}

// ── Test 1: Construction — main deck loaded, others empty ────────────────────

static void test_construction() {
    const std::string T = "construction";
    try {
        std::vector<uint32_t> cards = {101, 102, 103, 104, 105};
        gmCompDeck player("Alice", cards, /*seed=*/42);

        bool ok = player.count_in(ZoneId::MAIN_DECK) == 5
               && player.count_in(ZoneId::HAND)      == 0
               && player.count_in(ZoneId::PLAY_AREA) == 0
               && player.count_in(ZoneId::MEMORY)    == 0
               && player.count_in(ZoneId::DISCARD)   == 0
               && player.count_in(ZoneId::BANISHED)  == 0
               && player.total_count()               == 5
               && player.owner_name()                == "Alice";

        ok ? pass(T) : fail(T, "unexpected zone counts after construction");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 2: draw_to_hand — main deck → hand ───────────────────────────────────

static void test_draw_to_hand() {
    const std::string T = "draw_to_hand";
    try {
        gmCompDeck player("Bob", {1, 2, 3, 4, 5}, 0);
        player.draw_to_hand(3);

        bool ok = player.count_in(ZoneId::MAIN_DECK) == 2
               && player.count_in(ZoneId::HAND)      == 3
               && player.total_count()               == 5;

        ok ? pass(T) : fail(T, "wrong counts after draw_to_hand(3)");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 3: draw_to_hand — DeckEmptyError ────────────────────────────────────

static void test_draw_to_hand_empty() {
    const std::string T = "draw_to_hand_empty";
    try {
        gmCompDeck player("Carol", {1, 2}, 0);
        player.draw_to_hand(3);  // only 2 cards — must throw
        fail(T, "no exception thrown");
    } catch (const DeckEmptyError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 4: play_card and resolve_card ───────────────────────────────────────

static void test_play_and_resolve() {
    const std::string T = "play_and_resolve";
    try {
        gmCompDeck player("Dan", {10, 20, 30}, 1);
        player.draw_to_hand(2);

        uint32_t played = player.hand().peek_all().front();
        player.play_card(played);

        // Verify zones
        if (player.locate(played) != ZoneId::PLAY_AREA) {
            fail(T, "played card not in PLAY_AREA");
            return;
        }

        player.resolve_card(played);
        if (player.locate(played) != ZoneId::DISCARD) {
            fail(T, "resolved card not in DISCARD");
            return;
        }

        if (player.total_count() == 3) {
            pass(T);
        } else {
            fail(T, "total_count changed unexpectedly");
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 5: discard_from_hand ────────────────────────────────────────────────

static void test_discard_from_hand() {
    const std::string T = "discard_from_hand";
    try {
        gmCompDeck player("Eve", {1, 2, 3}, 2);
        player.draw_to_hand(3);
        uint32_t discarded = player.hand().peek_all().front();
        player.discard_from_hand(discarded);

        bool ok = player.count_in(ZoneId::HAND)    == 2
               && player.count_in(ZoneId::DISCARD) == 1
               && player.locate(discarded)         == ZoneId::DISCARD;

        ok ? pass(T) : fail(T, "wrong state after discard_from_hand");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 6: take_from_discard ────────────────────────────────────────────────

static void test_take_from_discard() {
    const std::string T = "take_from_discard";
    try {
        gmCompDeck player("Frank", {1, 2, 3}, 3);
        player.draw_to_hand(3);
        uint32_t card = player.hand().peek_all().front();
        player.discard_from_hand(card);
        player.take_from_discard(card);

        bool ok = player.count_in(ZoneId::DISCARD) == 0
               && player.count_in(ZoneId::HAND)    == 3
               && player.locate(card)              == ZoneId::HAND;

        ok ? pass(T) : fail(T, "wrong state after take_from_discard");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 7: banish from hand ──────────────────────────────────────────────────

static void test_banish_from_hand() {
    const std::string T = "banish_from_hand";
    try {
        gmCompDeck player("Grace", {10, 20, 30}, 4);
        player.draw_to_hand(2);
        uint32_t card = player.hand().peek_all().front();
        player.banish(card);

        bool ok = player.count_in(ZoneId::BANISHED) == 1
               && player.locate(card)               == ZoneId::BANISHED
               && player.total_count()              == 3;

        ok ? pass(T) : fail(T, "wrong state after banish");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 8: banish already-banished token → error ────────────────────────────

static void test_banish_already_banished() {
    const std::string T = "banish_already_banished";
    try {
        gmCompDeck player("Hank", {5, 6, 7}, 5);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.banish(card);
        player.banish(card);  // must throw
        fail(T, "no exception for double banish");
    } catch (const TokenNotFoundError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 9: reshuffle_discard_into_deck ───────────────────────────────────────

static void test_reshuffle_discard() {
    const std::string T = "reshuffle_discard_into_deck";
    try {
        gmCompDeck player("Iris", {1, 2, 3, 4, 5}, 6);
        player.draw_to_hand(5);

        // Discard all hand cards
        std::vector<uint32_t> hand_cards = player.hand().peek_all();
        for (uint32_t id : hand_cards) {
            player.discard_from_hand(id);
        }

        if (player.count_in(ZoneId::DISCARD) != 5) {
            fail(T, "expected 5 cards in discard before reshuffle");
            return;
        }

        player.reshuffle_discard_into_deck();

        bool ok = player.count_in(ZoneId::MAIN_DECK) == 5
               && player.count_in(ZoneId::DISCARD)   == 0
               && player.total_count()               == 5;

        ok ? pass(T) : fail(T, "wrong counts after reshuffle_discard_into_deck");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 10: draw_specific_to_hand ───────────────────────────────────────────

static void test_draw_specific_to_hand() {
    const std::string T = "draw_specific_to_hand";
    try {
        // Use auto_shuffle=false on base gmDeck — but gmCompDeck always shuffles.
        // Use a known seed and just verify the chosen token moves zones correctly.
        gmCompDeck player("Jack", {100, 200, 300, 400, 500}, 7);

        // Pick a token we know is in the main deck
        uint32_t target = player.main_deck().peek_all().back();  // last token
        player.draw_specific_to_hand(target);

        bool ok = player.locate(target)              == ZoneId::HAND
               && player.count_in(ZoneId::HAND)      == 1
               && player.count_in(ZoneId::MAIN_DECK) == 4;

        ok ? pass(T) : fail(T, "wrong state after draw_specific_to_hand");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 11: locate on unknown token → NOT_FOUND ─────────────────────────────

static void test_locate_not_found() {
    const std::string T = "locate_not_found";
    try {
        gmCompDeck player("Kate", {1, 2, 3}, 8);
        ZoneId loc = player.locate(999);  // 999 not in deck

        if (loc == ZoneId::NOT_FOUND) {
            pass(T);
        } else {
            fail(T, "expected NOT_FOUND, got " + zone_name(loc));
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 12: return_from_discard_to_deck ─────────────────────────────────────

static void test_return_from_discard_to_deck() {
    const std::string T = "return_from_discard_to_deck";
    try {
        gmCompDeck player("Leo", {1, 2, 3}, 9);
        player.draw_to_hand(3);
        player.discard_from_hand(2);
        player.return_from_discard_to_deck(2);

        bool ok = player.count_in(ZoneId::DISCARD)   == 0
               && player.count_in(ZoneId::MAIN_DECK) == 1
               && player.locate(2)                   == ZoneId::MAIN_DECK;

        ok ? pass(T) : fail(T, "wrong state after return_from_discard_to_deck");
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 13: remember_from_hand — Hand → Memory ────────────────────────────────

static void test_remember_from_hand() {
    const std::string T = "remember_from_hand";
    try {
        gmCompDeck player("Mia", {10, 20, 30}, 10);
        player.draw_to_hand(2);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);

        bool ok = player.locate(card)              == ZoneId::MEMORY
               && player.count_in(ZoneId::HAND)    == 1
               && player.count_in(ZoneId::MEMORY)  == 1
               && player.memory_size()             == 1
               && player.is_in_memory(card)
               && player.total_count()             == 3;

        ok ? pass(T) : fail(T, "wrong state after remember_from_hand");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 14: remember_from_play_area — Play Area → Memory ───────────────────

static void test_remember_from_play_area() {
    const std::string T = "remember_from_play_area";
    try {
        gmCompDeck player("Nora", {10, 20, 30}, 11);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.play_card(card);
        player.remember_from_play_area(card);

        bool ok = player.locate(card)              == ZoneId::MEMORY
               && player.count_in(ZoneId::PLAY_AREA) == 0
               && player.is_in_memory(card)
               && player.total_count()             == 3;

        ok ? pass(T) : fail(T, "wrong state after remember_from_play_area");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 15: remember_from_discard — Discard → Memory ──────────────────────

static void test_remember_from_discard() {
    const std::string T = "remember_from_discard";
    try {
        gmCompDeck player("Omar", {10, 20, 30}, 12);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.discard_from_hand(card);
        player.remember_from_discard(card);

        bool ok = player.locate(card)              == ZoneId::MEMORY
               && player.count_in(ZoneId::DISCARD) == 0
               && player.is_in_memory(card);

        ok ? pass(T) : fail(T, "wrong state after remember_from_discard");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 16: play_from_memory — Memory → Play Area ─────────────────────────

static void test_play_from_memory() {
    const std::string T = "play_from_memory";
    try {
        gmCompDeck player("Pia", {10, 20, 30}, 13);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);
        player.play_from_memory(card);

        bool ok = player.locate(card)                == ZoneId::PLAY_AREA
               && player.count_in(ZoneId::MEMORY)   == 0
               && player.count_in(ZoneId::PLAY_AREA) == 1;

        ok ? pass(T) : fail(T, "wrong state after play_from_memory");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 17: return_memory_to_hand — Memory → Hand ──────────────────────────

static void test_return_memory_to_hand() {
    const std::string T = "return_memory_to_hand";
    try {
        gmCompDeck player("Quinn", {10, 20, 30}, 14);
        player.draw_to_hand(2);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);
        player.return_memory_to_hand(card);

        bool ok = player.locate(card)             == ZoneId::HAND
               && player.count_in(ZoneId::MEMORY) == 0
               && player.count_in(ZoneId::HAND)   == 2;

        ok ? pass(T) : fail(T, "wrong state after return_memory_to_hand");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 18: discard_from_memory — Memory → Discard ────────────────────────

static void test_discard_from_memory() {
    const std::string T = "discard_from_memory";
    try {
        gmCompDeck player("Rosa", {10, 20, 30}, 15);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);
        player.discard_from_memory(card);

        bool ok = player.locate(card)              == ZoneId::DISCARD
               && player.count_in(ZoneId::MEMORY)  == 0
               && player.count_in(ZoneId::DISCARD) == 1;

        ok ? pass(T) : fail(T, "wrong state after discard_from_memory");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 19: banish_from_memory — Memory → Banish ───────────────────────────

static void test_banish_from_memory() {
    const std::string T = "banish_from_memory";
    try {
        gmCompDeck player("Sam", {10, 20, 30}, 16);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);
        player.banish_from_memory(card);

        bool ok = player.locate(card)               == ZoneId::BANISHED
               && player.count_in(ZoneId::MEMORY)   == 0
               && player.count_in(ZoneId::BANISHED)  == 1;

        ok ? pass(T) : fail(T, "wrong state after banish_from_memory");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 20: banish(id) universal covers Memory ──────────────────────────────

static void test_banish_universal_from_memory() {
    const std::string T = "banish_universal_from_memory";
    try {
        gmCompDeck player("Tara", {10, 20, 30}, 17);
        player.draw_to_hand(1);
        uint32_t card = player.hand().peek_all().front();
        player.remember_from_hand(card);
        player.banish(card);  // universal banish — must find token in Memory

        bool ok = player.locate(card)               == ZoneId::BANISHED
               && player.count_in(ZoneId::MEMORY)   == 0;

        ok ? pass(T) : fail(T, "banish() failed to locate token in Memory");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 21: Memory insertion order preserved ────────────────────────────────

static void test_memory_order_preserved() {
    const std::string T = "memory_order_preserved";
    try {
        gmCompDeck player("Uma", {10, 20, 30}, 18);
        player.draw_to_hand(3);

        // Build hand order from peek
        std::vector<uint32_t> hand = player.hand().peek_all();
        uint32_t first  = hand[0];
        uint32_t second = hand[1];
        uint32_t third  = hand[2];

        // Remember in known order (remove from hand one by one)
        player.remember_from_hand(first);
        player.remember_from_hand(second);
        player.remember_from_hand(third);

        std::vector<uint32_t> mem_order = player.memory().peek_all();
        bool ok = mem_order.size() == 3
               && mem_order[0] == first
               && mem_order[1] == second
               && mem_order[2] == third;

        ok ? pass(T) : fail(T, "memory insertion order not preserved");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 22: remember from wrong zone throws, zone unchanged ─────────────────

static void test_remember_wrong_zone_atomic() {
    const std::string T = "remember_wrong_zone_atomic";
    try {
        gmCompDeck player("Vera", {10, 20}, 19);
        player.draw_to_hand(1);
        int mem_before = player.memory_size();
        int hand_before = player.count_in(ZoneId::HAND);

        bool threw = false;
        try {
            player.remember_from_hand(999);  // 999 not in hand
        } catch (const TokenNotFoundError&) {
            threw = true;
        }

        bool unchanged = player.memory_size()             == mem_before
                      && player.count_in(ZoneId::HAND)    == hand_before
                      && player.total_count()             == 2;

        (threw && unchanged) ? pass(T) : fail(T, "zones changed despite exception");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== gmCompDeck integration tests ===\n\n";

    test_construction();
    test_draw_to_hand();
    test_draw_to_hand_empty();
    test_play_and_resolve();
    test_discard_from_hand();
    test_take_from_discard();
    test_banish_from_hand();
    test_banish_already_banished();
    test_reshuffle_discard();
    test_draw_specific_to_hand();
    test_locate_not_found();
    test_return_from_discard_to_deck();
    // MEMORY zone tests
    test_remember_from_hand();
    test_remember_from_play_area();
    test_remember_from_discard();
    test_play_from_memory();
    test_return_memory_to_hand();
    test_discard_from_memory();
    test_banish_from_memory();
    test_banish_universal_from_memory();
    test_memory_order_preserved();
    test_remember_wrong_zone_atomic();

    std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
    return (g_fail == 0) ? 0 : 1;
}
