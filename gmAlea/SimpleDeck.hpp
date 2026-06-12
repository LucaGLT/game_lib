#ifndef GMALEA_SIMPLEDECK_HPP
#define GMALEA_SIMPLEDECK_HPP

/**
 * @file  SimpleDeck.hpp
 * @brief Wrapper for GmDeck supporting embedded token metadata (label, value).
 */

#include "GmDeck.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace gmAlea
{

/**
 * @struct Token
 * @brief Simple token with embedded metadata.
 *
 * Used by SimpleDeck to provide a higher-level API for common cases
 * where tokens need minimal structure (ID, label, value).
 * For complex tokens with many attributes, use GmDeck directly
 * with an external mapping.
 */
struct Token
{
	uint32_t id;          ///< Unique token identifier.
	std::string label;    ///< Human-readable token name (e.g. "Face1", "Success").
	int value;            ///< Numeric value (e.g. die face value, probability weight).
};

/**
 * @class SimpleDeck
 * @brief Wrapper around GmDeck for cases with embedded token metadata.
 *
 * SimpleDeck encapsulates a GmDeck (for ordering/shuffling) plus a lookup map
 * (for token data). This eliminates the need to manually maintain a separate
 * dictionary for simple use cases like dice faces or basic tokens.
 *
 * Design philosophy:
 *   - GmDeck remains generic and reusable (ID-only, no coupling to Token struct).
 *   - SimpleDeck adapts GmDeck to simple single-struct cases.
 *   - For complex tokens (e.g. cards with cost/effects/images), use GmDeck directly.
 *
 * @note Not thread-safe.
 */
class SimpleDeck
{
public:
	/**
	 * @brief Constructs a SimpleDeck from a vector of Token objects.
	 *
	 * Automatically builds the internal lookup map from token IDs.
	 * By default, disallows duplicate token IDs (throws if found).
	 *
	 * @param tokens          Vector of Token objects to populate the deck.
	 * @param seed            Optional random seed for deterministic shuffling.
	 * @param allow_duplicates If false (default), throws EAleaDuplicateTokenIdError
	 *                        when the same ID appears more than once.
	 *
	 * @throws EAleaDuplicateTokenIdError if @p allow_duplicates is false and
	 *         duplicate token IDs are found in @p tokens.
	 */
	explicit SimpleDeck(const std::vector<Token>& tokens,
					    std::optional<unsigned int> seed = std::nullopt,
					    bool allow_duplicates = false);

	/**
	 * @brief Draws a single token from the front of the deck.
	 *
	 * @return The drawn Token (ID + label + value).
	 * @throws EAleaDeckEmptyError if the deck is empty.
	 */
	Token draw_one();

	/**
	 * @brief Draws k tokens from the front of the deck.
	 *
	 * @param k Number of tokens to draw.
	 * @return Vector of drawn Token objects.
	 * @throws EAleaInvalidDrawCountError if @p k <= 0.
	 * @throws EAleaDeckEmptyError if not enough tokens remain.
	 */
	std::vector<Token> draw_many(int k);

	/**
	 * @brief Returns the number of tokens remaining in the deck.
	 *
	 * @return Count of remaining tokens.
	 */
	int remaining_count() const;

	/**
	 * @brief Checks if the deck is empty.
	 *
	 * @return true if deck is empty, false otherwise.
	 */
	bool is_empty() const;

	/**
	 * @brief Shuffles the deck using the stored RNG.
	 */
	void shuffle();

	/**
	 * @brief Resets the deck to its initial state or new tokens.
	 *
	 * @param tokens Optional new list of Token objects; if null, uses initial tokens.
	 * @throws EAleaDuplicateTokenIdError if constructed with @p allow_duplicates=false
	 *         and the new tokens contain duplicate IDs.
	 */
	void reset(const std::optional<std::vector<Token>>& tokens = std::nullopt);

	/**
	 * @brief Returns a copy of all remaining tokens in the deck.
	 *
	 * @return Vector of all remaining Token objects.
	 */
	std::vector<Token> peek_all() const;

	/**
	 * @brief Removes the first occurrence of a specific token by ID.
	 *
	 * @param token_id The token ID to remove (does nothing if not found).
	 */
	void remove(uint32_t token_id);

	/**
	 * @brief Checks if a specific token exists in the deck by ID.
	 *
	 * @param token_id The token ID to check.
	 * @return true if token is in deck, false otherwise.
	 */
	bool contains(uint32_t token_id) const;

	/**
	 * @brief Appends a token at the back of the deck (no shuffle).
	 *
	 * @param token Token to add.
	 * @throws EAleaDuplicateTokenIdError if @p allow_duplicates is false and
	 *         a token with the same ID already exists in the deck.
	 */
	void push_back(const Token& token);

	/**
	 * @brief Prepends a token at the front of the deck (no shuffle).
	 *
	 * @param token Token to add.
	 * @throws EAleaDuplicateTokenIdError if @p allow_duplicates is false and
	 *         a token with the same ID already exists in the deck.
	 */
	void push_front(const Token& token);

	/**
	 * @brief Views the next token at the top of the deck without removing it.
	 *
	 * Allows inspection of the top token without modifying deck structure.
	 *
	 * @return The Token object at the top of the deck.
	 * @throws EAleaDeckEmptyError if the deck is empty.
	 */
	Token see_top() const;

	/**
	 * @brief Views the token at the bottom of the deck without removing it.
	 *
	 * Allows inspection of the bottom token without modifying deck structure.
	 *
	 * @return The Token object at the bottom of the deck.
	 * @throws EAleaDeckEmptyError if the deck is empty.
	 */
	Token see_bottom() const;

	/**
	 * @brief Finds, removes, and returns a specific token by ID.
	 *
	 * @param token_id The token ID to draw.
	 * @return The drawn Token object.
	 * @throws EAleaTokenNotFoundError if @p token_id is not in the deck.
	 */
	Token draw_specific(uint32_t token_id);

private:
	// Underlying ordered container (GmDeck handles shuffle, structure only)
	GmDeck _deck;

	// Lookup map: token ID → Token metadata
	std::unordered_map<uint32_t, Token> _token_db;

	// Initial tokens for reset() support
	std::vector<Token> _initial_tokens;

	// Allow duplicate IDs (e.g., probability decks)
	bool _allow_duplicates;

	// Extract all IDs from a Token vector (helper for GmDeck initialization)
	static std::vector<uint32_t> _extract_ids(const std::vector<Token>& tokens);
};

} // namespace gmAlea

#endif // GMALEA_SIMPLEDECK_HPP
