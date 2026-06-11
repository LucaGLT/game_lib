#ifndef FATEBAG_POLICYBASEDDECK_HPP
#define FATEBAG_POLICYBASEDDECK_HPP

/**
 * @file PolicyBasedDeck.hpp
 * @brief Template zone wrapper that enforces compile-time zone behavioural policies.
 *
 * All method bodies are defined in this header because C++ requires template
 * definitions to be visible at the point of instantiation.
 */

#include "gmDeck.hpp"
#include "ZonePolicy.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace FateBag {

// ─────────────────────────────────────────────────────────────────────────────
// Zone-specific exception
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thrown when a policy-forbidden operation is called at runtime.
 *
 * Note: most policy violations are caught at **compile time** via `static_assert`
 * inside `PolicyBasedDeck` method bodies.  This exception is reserved for
 * cases where the violation can only be detected at runtime (e.g. future
 * dynamic-policy extensions).
 */
class ZonePolicyViolation : public std::runtime_error {
public:
    explicit ZonePolicyViolation(const std::string& message)
        : std::runtime_error("ZonePolicyViolation: " + message) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// PolicyBasedDeck<Policy>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class PolicyBasedDeck
 * @brief A @ref gmDeck wrapper that enforces compile-time zone behavioural policies.
 *
 * @tparam Policy One of the five built-in policy structs from `ZonePolicy.hpp`:
 *   - `MainDeckPolicy`  — shufflable, direct access
 *   - `HandPolicy`      — no shuffle, direct access
 *   - `PlayAreaPolicy`  — no shuffle, direct access, order preserved
 *   - `DiscardPolicy`   — no shuffle (order sacred), direct access
 *   - `BanishPolicy`    — no shuffle, no access, insert-only
 *
 * Policy flags are enforced via `static_assert` inside method bodies, producing
 * **compile-time errors** for disallowed operations.
 *
 * ### Example
 * @code
 *   FateBag::DiscardPile discard("DISCARD");
 *   discard.add(42);
 *   discard.add(17);
 *   // discard.shuffle();         ← compile error: DiscardPolicy::can_shuffle == false
 *   uint32_t card = discard.take_specific(42);
 * @endcode
 */
template <typename Policy>
class PolicyBasedDeck {
public:
    // ── Constructors ─────────────────────────────────────────────────────────

    /**
     * @brief Constructs an empty zone with the given display name.
     *
     * The internal `gmDeck` is initialised with an empty token list and
     * `auto_shuffle = false` (no-op for an empty deck).
     *
     * @param zone_name Human-readable name used in error messages (e.g. "HAND").
     */
    explicit PolicyBasedDeck(std::string zone_name)
        : zone_name_(std::move(zone_name))
        , deck_({}, std::nullopt, false)
    {}

    /**
     * @brief Constructs a pre-populated zone.
     *
     * If `Policy::can_shuffle == true` (e.g. MainDeckPolicy), the tokens
     * are shuffled automatically on construction.  Otherwise the order from
     * `initial_tokens` is preserved.
     *
     * @param zone_name     Human-readable zone name.
     * @param initial_tokens Tokens to load into this zone.
     * @param seed          Optional RNG seed (only relevant when can_shuffle is true).
     * @throws DuplicateTokenIdError if `initial_tokens` contains duplicates.
     */
    PolicyBasedDeck(std::string zone_name,
                    const std::vector<uint32_t>& initial_tokens,
                    std::optional<unsigned int> seed = std::nullopt)
        : zone_name_(std::move(zone_name))
        , deck_(initial_tokens, seed, Policy::can_shuffle)
    {}

    // ── Mutation ─────────────────────────────────────────────────────────────

    /**
     * @brief Adds a token at the back (bottom) of the zone.
     *
     * @param token_id Token to add.
     * @throws DuplicateTokenIdError if token_id is already in this zone.
     */
    void add(uint32_t token_id) {
        deck_.push_back(token_id);
    }

    /**
     * @brief Adds a token at the front (top) of the zone.
     *
     * @param token_id Token to add.
     * @throws DuplicateTokenIdError if token_id is already in this zone.
     */
    void add_to_top(uint32_t token_id) {
        deck_.push_front(token_id);
    }

    /**
     * @brief Removes and returns the top token from the zone.
     *
     * @return The drawn token ID.
     * @throws DeckEmptyError if the zone is empty.
     *
     * @note **Compile error** if `Policy::is_insert_only == true` (BanishZone).
     */
    uint32_t draw() {
        static_assert(
            !Policy::is_insert_only,
            "draw() is not allowed on insert-only zones (e.g. BanishZone).");
        return deck_.draw_one();
    }

    /**
     * @brief Finds, removes, and returns a specific token by ID.
     *
     * @param token_id Token to retrieve.
     * @return token_id (for call-chain convenience).
     * @throws TokenNotFoundError if token_id is not in this zone.
     *
     * @note **Compile error** if `Policy::can_direct_access == false` (BanishZone).
     */
    uint32_t take_specific(uint32_t token_id) {
        static_assert(
            Policy::can_direct_access,
            "take_specific() is not allowed on zones with can_direct_access == false "
            "(e.g. BanishZone).");
        return deck_.draw_specific(token_id);
    }

    /**
     * @brief Shuffles the tokens in this zone randomly.
     *
     * @note **Compile error** if `Policy::can_shuffle == false`
     *       (Hand, PlayArea, Discard, BanishZone).
     */
    void shuffle() {
        static_assert(
            Policy::can_shuffle,
            "shuffle() is not allowed on this zone type.  "
            "Only MainDeckPolicy permits shuffling.");
        deck_.shuffle();
    }

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Returns a copy of all tokens (top to bottom) without removing them.
     */
    std::vector<uint32_t> peek_all() const {
        return deck_.peek_all();
    }

    /**
     * @brief Returns a copy of the top `n` tokens without removing them.
     *
     * If `n` exceeds the zone size, all tokens are returned.
     *
     * @param n Maximum number of tokens to peek.
     */
    std::vector<uint32_t> peek_top(int n) const {
        std::vector<uint32_t> all = deck_.peek_all();
        int limit = std::min(n, static_cast<int>(all.size()));
        return std::vector<uint32_t>(all.begin(), all.begin() + limit);
    }

    /**
     * @brief Returns true if the zone contains the given token.
     */
    bool contains(uint32_t token_id) const {
        return deck_.contains(token_id);
    }

    /**
     * @brief Returns the number of tokens currently in this zone.
     */
    int count() const {
        return deck_.remaining_count();
    }

    /**
     * @brief Returns true if this zone is empty.
     */
    bool is_empty() const {
        return deck_.is_empty();
    }

    /**
     * @brief Returns the zone's display name.
     */
    const std::string& zone_name() const {
        return zone_name_;
    }

private:
    std::string zone_name_;
    gmDeck      deck_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Convenience type aliases for the five built-in zones
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Primary shuffled draw deck. */
using MainDeck    = PolicyBasedDeck<MainDeckPolicy>;

/** @brief Player's hand — no shuffle, direct access by ID. */
using CardHand    = PolicyBasedDeck<HandPolicy>;

/** @brief Cards currently in play on the table. */
using PlayArea    = PolicyBasedDeck<PlayAreaPolicy>;

/** @brief Ordered discard pile — insertion order preserved, no shuffle. */
using DiscardPile = PolicyBasedDeck<DiscardPolicy>;

/** @brief Banish zone — insert-only, tokens permanently out of play. */
using BanishZone  = PolicyBasedDeck<BanishPolicy>;

} // namespace FateBag

#endif // FATEBAG_POLICYBASEDDECK_HPP
