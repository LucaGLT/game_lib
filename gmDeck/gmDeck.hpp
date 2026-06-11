#ifndef FATEBAG_GMDECK_HPP
#define FATEBAG_GMDECK_HPP

#include <cstdint>
#include <vector>
#include <random>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <string>

namespace FateBag {

// Exception classes
class DeckAdapterError : public std::runtime_error {
public:
    explicit DeckAdapterError(const std::string& message)
        : std::runtime_error("DeckAdapterError: " + message) {}
};

class DeckEmptyError : public DeckAdapterError {
public:
    explicit DeckEmptyError(const std::string& message)
        : DeckAdapterError(message) {}
};

class DuplicateTokenIdError : public DeckAdapterError {
public:
    explicit DuplicateTokenIdError(const std::string& message)
        : DeckAdapterError(message) {}
};

class InvalidDrawCountError : public DeckAdapterError {
public:
    explicit InvalidDrawCountError(const std::string& message)
        : DeckAdapterError(message) {}
};

class TokenNotFoundError : public DeckAdapterError {
public:
    explicit TokenNotFoundError(const std::string& message)
        : DeckAdapterError(message) {}
};

/**
 * @class gmDeck
 * @brief Deterministic in-memory token deck adapter (optimized with uint32_t IDs).
 *
 * The adapter only handles low-level deck operations over token IDs.
 * Uses uint32_t for token IDs for maximum efficiency:
 * - 4 bytes per token vs string overhead (50-100+ bytes)
 * - O(1) comparisons vs O(n) string comparisons
 * - Better cache locality for shuffle operations
 *
 * Domain rules stay in core engine/services.
 */
class gmDeck {
public:
    /**
     * @brief Constructor - creates a new gmDeck with initial token IDs.
     * @param token_ids List of token IDs (uint32_t) to populate the deck
     * @param seed Optional random seed for deterministic shuffling (null = random)
     * @throws DuplicateTokenIdError if token_ids contains duplicates
     */
    explicit gmDeck(const std::vector<uint32_t>& token_ids,
                     std::optional<unsigned int> seed = std::nullopt,
                     bool auto_shuffle = true);

    /**
     * @brief Shuffles the deck using the stored RNG.
     */
    void shuffle();

    /**
     * @brief Draws a single token from the front of the deck.
     * @return The drawn token ID (uint32_t)
     * @throws DeckEmptyError if the deck is empty
     */
    uint32_t draw_one();

    /**
     * @brief Draws k tokens from the front of the deck.
     * @param k Number of tokens to draw
     * @return Vector of drawn token IDs
     * @throws InvalidDrawCountError if k <= 0
     * @throws DeckEmptyError if not enough tokens remain
     */
    std::vector<uint32_t> draw_many(int k);

    /**
     * @brief Returns the number of tokens remaining in the deck.
     * @return Count of remaining tokens
     */
    int remaining_count() const;

    /**
     * @brief Checks if the deck is empty.
     * @return true if deck is empty, false otherwise
     */
    bool is_empty() const;

    /**
     * @brief Resets the deck to its initial state or new token IDs.
     * @param token_ids Optional new list of token IDs; if null, uses initial IDs
     * @throws DuplicateTokenIdError if new token_ids contains duplicates
     */
    void reset(const std::optional<std::vector<uint32_t>>& token_ids = std::nullopt);

    /**
     * @brief Returns a copy of all remaining tokens in the deck.
     * @return Vector of all remaining token IDs
     */
    std::vector<uint32_t> peek_all() const;

    /**
     * @brief Removes a specific token from the deck.
     * @param token_id The token ID to remove (does nothing if not found)
     */
    void remove(uint32_t token_id);

    /**
     * @brief Checks if a specific token exists in the deck.
     * @param token_id The token ID to check
     * @return true if token is in deck, false otherwise
     */
    bool contains(uint32_t token_id) const;

    /**
     * @brief Appends a token at the back of the deck (no shuffle).
     * @param token_id Token to add
     * @throws DuplicateTokenIdError if token_id is already in the deck
     */
    void push_back(uint32_t token_id);

    /**
     * @brief Prepends a token at the front of the deck (no shuffle).
     * @param token_id Token to add
     * @throws DuplicateTokenIdError if token_id is already in the deck
     */
    void push_front(uint32_t token_id);

    /**
     * @brief Finds, removes, and returns a specific token by ID.
     * @param token_id Token to draw
     * @return token_id (for convenience)
     * @throws TokenNotFoundError if token_id is not in the deck
     */
    uint32_t draw_specific(uint32_t token_id);

private:
    /**
     * @brief Validates that token_ids contains no duplicates.
     * @param token_ids List to validate
     * @throws DuplicateTokenIdError if duplicates are found
     */
    static void _validate_token_ids(const std::vector<uint32_t>& token_ids);

    std::vector<uint32_t> _deck;
    std::vector<uint32_t> _initial_token_ids;
    std::optional<unsigned int> _seed;
    std::mt19937 _rng;
};

} // namespace FateBag

#endif // FATEBAG_GMDECK_HPP
