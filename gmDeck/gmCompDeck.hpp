#ifndef FATEBAG_GMCOMPDECK_HPP
#define FATEBAG_GMCOMPDECK_HPP

/**
 * @file gmCompDeck.hpp
 * @brief Multi-zone deck orchestrator for a single game entity.
 *
 * `gmCompDeck` manages the full card lifecycle — from the initial shuffled
 * draw deck all the way through hand, play area, discard pile, and permanent
 * banishment — while guaranteeing that **every token ID exists in exactly one
 * zone at all times**.
 */

#include "PolicyBasedDeck.hpp"
#include "CardLocation.hpp"

#include <optional>
#include <string>
#include <vector>

namespace FateBag {

/**
 * @class gmCompDeck
 * @brief Multi-zone deck manager for a single game entity (player, faction, event source).
 *
 * @invariant Every token registered with this `gmCompDeck` exists in **exactly
 *            one** of its five zones at any point in time.
 *
 * ## Zones
 *
 * | Zone        | Type alias   | Key constraints                              |
 * |-------------|--------------|----------------------------------------------|
 * | Main Deck   | `MainDeck`   | Shufflable; draw from top; pick by ID        |
 * | Hand        | `CardHand`   | No shuffle; pick any card by ID              |
 * | Play Area   | `PlayArea`   | No shuffle; order preserved; pick by ID      |
 * | Discard     | `DiscardPile`| Order **sacred** (no shuffle); pick by ID   |
 * | Banish Zone | `BanishZone` | Insert-only; no retrieval                    |
 *
 * ## Ownership model
 *
 * `gmCompDeck` owns all five zone objects.  External code may **read** them
 * via the `const` accessors (`main_deck()`, `hand()`, …).  All **writes**
 * (card movements) must go through `gmCompDeck` methods to preserve the
 * uniqueness invariant.
 *
 * ## Typical game flow
 * @code
 *   FateBag::gmCompDeck player("Alice", {101, 102, 103, 104, 105, 106, 107});
 *
 *   player.draw_to_hand(3);            // Main Deck → Hand
 *   player.play_card(102);             // Hand → Play Area
 *   player.resolve_card(102);          // Play Area → Discard
 *   player.take_from_discard(102);     // Discard → Hand
 *   player.banish(102);                // Hand → Banish (permanent)
 *
 *   FateBag::ZoneId loc = player.locate(102); // → ZoneId::BANISHED
 * @endcode
 */
class gmCompDeck {
public:
    // ─────────────────────────────────────────────────────────────────────────
    // Construction / Destruction
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Constructs a `gmCompDeck` with a named owner and an optional initial deck.
     *
     * All tokens in `deck_tokens` are loaded into the **main deck**.  The main
     * deck is shuffled automatically (respecting `MainDeckPolicy::can_shuffle`).
     * All other zones start empty.
     *
     * @param owner_name  Human-readable identifier for the entity (e.g. `"Player1"`).
     * @param deck_tokens Token IDs that populate the main deck.  May be empty.
     * @param seed        Optional RNG seed for deterministic shuffle.
     * @throws DuplicateTokenIdError if `deck_tokens` contains duplicate IDs.
     */
    explicit gmCompDeck(std::string                  owner_name,
                        const std::vector<uint32_t>& deck_tokens = {},
                        std::optional<unsigned int>  seed        = std::nullopt);

    ~gmCompDeck() = default;

    gmCompDeck(const gmCompDeck&)            = delete;
    gmCompDeck& operator=(const gmCompDeck&) = delete;

    gmCompDeck(gmCompDeck&&)            = default;
    gmCompDeck& operator=(gmCompDeck&&) = default;

    // ─────────────────────────────────────────────────────────────────────────
    // Cross-zone moves  — all guarantee the uniqueness invariant
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Draws `count` cards from the top of the main deck into hand.
     *
     * @param count Number of cards to draw.  Must be > 0.
     * @throws InvalidDrawCountError if `count <= 0`.
     * @throws DeckEmptyError if the main deck has fewer than `count` tokens.
     */
    void draw_to_hand(int count);

    /**
     * @brief Draws a specific token from anywhere in the main deck into hand.
     *
     * Searches the entire main deck for `token_id` (not just the top), removes
     * it, and places it in hand.
     *
     * @param token_id Token to move.
     * @throws TokenNotFoundError if `token_id` is not in the main deck.
     */
    void draw_specific_to_hand(uint32_t token_id);

    /**
     * @brief Plays a card from hand onto the play area.
     *
     * @param token_id Token to play.
     * @throws TokenNotFoundError if `token_id` is not in hand.
     */
    void play_card(uint32_t token_id);

    /**
     * @brief Resolves (removes from play) a card from the play area to the discard pile.
     *
     * @param token_id Token to resolve.
     * @throws TokenNotFoundError if `token_id` is not in the play area.
     */
    void resolve_card(uint32_t token_id);

    /**
     * @brief Discards a card directly from hand to the discard pile.
     *
     * @param token_id Token to discard.
     * @throws TokenNotFoundError if `token_id` is not in hand.
     */
    void discard_from_hand(uint32_t token_id);

    /**
     * @brief Forcibly discards a card from the play area to the discard pile.
     *
     * @param token_id Token to discard.
     * @throws TokenNotFoundError if `token_id` is not in the play area.
     */
    void discard_from_table(uint32_t token_id);

    /**
     * @brief Retrieves a specific card from the discard pile back to hand.
     *
     * The discard pile order is preserved for the remaining cards.
     *
     * @param token_id Token to retrieve.
     * @throws TokenNotFoundError if `token_id` is not in the discard pile.
     */
    void take_from_discard(uint32_t token_id);

    /**
     * @brief Returns a specific card from the discard pile to the bottom of the main deck.
     *
     * @param token_id Token to return.
     * @throws TokenNotFoundError if `token_id` is not in the discard pile.
     */
    void return_from_discard_to_deck(uint32_t token_id);

    /**
     * @brief Permanently banishes a token from any zone (except banish zone itself).
     *
     * Locates the token in whichever zone it currently occupies, removes it,
     * and places it in the banish zone.  Banished tokens cannot be retrieved.
     *
     * @param token_id Token to banish.
     * @throws TokenNotFoundError if `token_id` is not tracked by this `gmCompDeck`,
     *         or if it is already banished.
     */
    void banish(uint32_t token_id);

    /**
     * @brief Moves all cards from the discard pile back into the main deck and reshuffles.
     *
     * After this call the discard pile is empty and the main deck contains all
     * previously discarded tokens (plus whatever was already there), shuffled.
     */
    void reshuffle_discard_into_deck();

    // ─────────────────────────────────────────────────────────────────────────
    // Query
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Returns the zone that currently holds the given token.
     *
     * @param token_id Token to locate.
     * @return `ZoneId` of the containing zone, or `ZoneId::NOT_FOUND`.
     */
    ZoneId locate(uint32_t token_id) const;

    /**
     * @brief Returns the number of tokens in a specific zone.
     *
     * @param zone Target zone.
     * @return Token count in that zone.
     */
    int count_in(ZoneId zone) const;

    /**
     * @brief Returns the total number of tokens tracked across all zones.
     *
     * Sum of main_deck + hand + play_area + discard + banish.
     */
    int total_count() const;

    /**
     * @brief Returns the owner's name (set at construction).
     */
    const std::string& owner_name() const;

    // ─────────────────────────────────────────────────────────────────────────
    // Read-only zone accessors
    // ─────────────────────────────────────────────────────────────────────────

    /** @brief Read-only access to the main deck zone. */
    const MainDeck&    main_deck()   const { return main_deck_;   }

    /** @brief Read-only access to the hand zone. */
    const CardHand&    hand()        const { return hand_;        }

    /** @brief Read-only access to the play area zone. */
    const PlayArea&    play_area()   const { return play_area_;   }

    /** @brief Read-only access to the discard pile zone. */
    const DiscardPile& discard()     const { return discard_;     }

    /** @brief Read-only access to the banish zone. */
    const BanishZone&  banish_zone() const { return banish_zone_; }

private:
    /**
     * @brief Removes a token from the specified zone.
     *
     * Internal helper used by cross-zone move methods.
     * Must not be called with `ZoneId::BANISHED` or `ZoneId::NOT_FOUND`
     * — those cases throw `TokenNotFoundError`.
     *
     * @param zone     Zone from which to remove the token.
     * @param token_id Token to remove.
     * @throws TokenNotFoundError if `zone` is BANISHED or NOT_FOUND.
     */
    void _remove_from_zone(ZoneId zone, uint32_t token_id);

    std::string   owner_name_;
    MainDeck      main_deck_;
    CardHand      hand_;
    PlayArea      play_area_;
    DiscardPile   discard_;
    BanishZone    banish_zone_;
};

} // namespace FateBag

#endif // FATEBAG_GMCOMPDECK_HPP
