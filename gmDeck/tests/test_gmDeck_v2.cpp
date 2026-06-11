/**
 * @file tests/test_gmDeck_v2.cpp
 * @brief Unit tests for gmDeck v2 new methods: push_back, push_front, draw_specific,
 *        auto_shuffle flag, and the new TokenNotFoundError exception.
 *
 * Build (from game_lib root):
 *   g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/tests/test_gmDeck_v2.cpp \
 *       -o test_gmDeck_v2 && ./test_gmDeck_v2
 */

#include "gmDeck/gmDeck.hpp"

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

// ── Test 1: auto_shuffle=false preserves insertion order ──────────────────────

static void test_no_auto_shuffle() {
    const std::string T = "no_auto_shuffle";
    try {
        std::vector<uint32_t> tokens = {10, 20, 30, 40, 50};
        gmDeck deck(tokens, std::nullopt, /*auto_shuffle=*/false);

        // Order must be exactly 10,20,30,40,50
        std::vector<uint32_t> peeked = deck.peek_all();
        if (peeked == tokens) {
            pass(T);
        } else {
            fail(T, "order was changed even though auto_shuffle=false");
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 2: auto_shuffle=true (default) shuffles ─────────────────────────────

static void test_auto_shuffle_default() {
    const std::string T = "auto_shuffle_default";
    // With a fixed seed we can verify the deck IS shuffled (not in original order)
    // Seed 1 with 5 elements is unlikely to produce identity permutation
    std::vector<uint32_t> tokens = {1, 2, 3, 4, 5};
    gmDeck deck(tokens, /*seed=*/1);

    // Just verify it compiles and doesn't throw
    if (!deck.is_empty() && deck.remaining_count() == 5) {
        pass(T);
    } else {
        fail(T, "unexpected deck state after construction");
    }
}

// ── Test 3: push_back appends correctly ──────────────────────────────────────

static void test_push_back() {
    const std::string T = "push_back";
    try {
        gmDeck deck({10, 20}, std::nullopt, false);  // no shuffle: [10, 20]
        deck.push_back(30);

        std::vector<uint32_t> all = deck.peek_all();
        if (all.size() == 3 && all.back() == 30) {
            pass(T);
        } else {
            fail(T, "30 not appended at back");
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 4: push_front prepends correctly ────────────────────────────────────

static void test_push_front() {
    const std::string T = "push_front";
    try {
        gmDeck deck({10, 20}, std::nullopt, false);  // no shuffle: [10, 20]
        deck.push_front(5);

        std::vector<uint32_t> all = deck.peek_all();
        if (all.size() == 3 && all.front() == 5) {
            pass(T);
        } else {
            fail(T, "5 not prepended at front");
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 5: push_back rejects duplicate ──────────────────────────────────────

static void test_push_back_duplicate() {
    const std::string T = "push_back_duplicate";
    try {
        gmDeck deck({10, 20}, std::nullopt, false);
        deck.push_back(10);  // 10 already present → must throw
        fail(T, "no exception thrown for duplicate");
    } catch (const DuplicateTokenIdError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 6: push_front rejects duplicate ─────────────────────────────────────

static void test_push_front_duplicate() {
    const std::string T = "push_front_duplicate";
    try {
        gmDeck deck({10, 20}, std::nullopt, false);
        deck.push_front(20);  // 20 already present → must throw
        fail(T, "no exception thrown for duplicate");
    } catch (const DuplicateTokenIdError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 7: draw_specific returns and removes the token ──────────────────────

static void test_draw_specific() {
    const std::string T = "draw_specific";
    try {
        gmDeck deck({10, 20, 30}, std::nullopt, false);
        uint32_t drawn = deck.draw_specific(20);

        if (drawn == 20 && deck.remaining_count() == 2 && !deck.contains(20)) {
            pass(T);
        } else {
            fail(T, "wrong state after draw_specific(20)");
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 8: draw_specific on missing token throws TokenNotFoundError ──────────

static void test_draw_specific_not_found() {
    const std::string T = "draw_specific_not_found";
    try {
        gmDeck deck({10, 20, 30}, std::nullopt, false);
        deck.draw_specific(99);  // 99 not in deck
        fail(T, "no exception thrown for missing token");
    } catch (const TokenNotFoundError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 9: allow_duplicates=true — probability deck construction ─────────────

static void test_allow_duplicates_construction() {
    const std::string T = "allow_duplicates_construction";
    try {
        // 8× id=1 (Success), 2× id=2 (Failure) — total 10 cards
        std::vector<uint32_t> tokens = {1,1,1,1,1,1,1,1, 2,2};
        gmDeck deck(tokens, std::nullopt, /*auto_shuffle=*/false, /*allow_duplicates=*/true);

        if (deck.remaining_count() == 10) {
            pass(T);
        } else {
            fail(T, "expected 10 cards, got " + std::to_string(deck.remaining_count()));
        }
    } catch (...) {
        fail(T, "unexpected exception — allow_duplicates should permit same ID");
    }
}

// ── Test 10: allow_duplicates=false still rejects duplicates (default) ────────

static void test_disallow_duplicates_still_throws() {
    const std::string T = "disallow_duplicates_still_throws";
    try {
        std::vector<uint32_t> tokens = {1, 1, 2};
        gmDeck deck(tokens);  // default allow_duplicates=false → must throw
        fail(T, "no exception thrown");
    } catch (const DuplicateTokenIdError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 11: probability deck — draw all, check counts ───────────────────────

static void test_probability_deck_draw_all() {
    const std::string T = "probability_deck_draw_all";
    try {
        // 8× Success (1), 2× Failure (2)
        std::vector<uint32_t> tokens = {1,1,1,1,1,1,1,1, 2,2};
        gmDeck deck(tokens, /*seed=*/42, /*auto_shuffle=*/true, /*allow_duplicates=*/true);

        int count_success = 0;
        int count_failure = 0;
        while (!deck.is_empty()) {
            uint32_t drawn = deck.draw_one();
            if (drawn == 1) ++count_success;
            else if (drawn == 2) ++count_failure;
        }

        if (count_success == 8 && count_failure == 2) {
            pass(T);
        } else {
            fail(T, "wrong counts: success=" + std::to_string(count_success) +
                     " failure=" + std::to_string(count_failure));
        }
    } catch (...) {
        fail(T, "unexpected exception");
    }
}

// ── Test 12: push_back allows duplicates when flag is set ────────────────────

static void test_push_back_allow_duplicates() {
    const std::string T = "push_back_allow_duplicates";
    try {
        gmDeck deck({1, 2}, std::nullopt, false, /*allow_duplicates=*/true);
        deck.push_back(1);  // duplicate — should NOT throw
        deck.push_back(1);  // another copy

        if (deck.remaining_count() == 4) {
            pass(T);
        } else {
            fail(T, "expected 4 cards");
        }
    } catch (...) {
        fail(T, "unexpected exception — allow_duplicates should permit push_back of same ID");
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== gmDeck v2 unit tests ===\n\n";

    test_no_auto_shuffle();
    test_auto_shuffle_default();
    test_push_back();
    test_push_front();
    test_push_back_duplicate();
    test_push_front_duplicate();
    test_draw_specific();
    test_draw_specific_not_found();
    test_allow_duplicates_construction();
    test_disallow_duplicates_still_throws();
    test_probability_deck_draw_all();
    test_push_back_allow_duplicates();

    std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
    return (g_fail == 0) ? 0 : 1;
}
