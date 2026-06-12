#ifndef GMALEA_GMDECK_HPP
#define GMALEA_GMDECK_HPP

#include <cstdint>
#include <vector>
#include <random>
#include <optional>
#include <stdexcept>
#include <string>

namespace gmAlea
{

// Exception classes
class EAleaError : public std::runtime_error
{
public:
	explicit EAleaError(const std::string& message)
		: std::runtime_error("EAleaError: " + message) {}
};

class EAleaDeckEmptyError : public EAleaError
{
public:
	explicit EAleaDeckEmptyError(const std::string& message)
		: EAleaError(message) {}
};

class EAleaDuplicateTokenIdError : public EAleaError
{
public:
	explicit EAleaDuplicateTokenIdError(const std::string& message)
		: EAleaError(message) {}
};

class EAleaInvalidDrawCountError : public EAleaError
{
public:
	explicit EAleaInvalidDrawCountError(const std::string& message)
		: EAleaError(message) {}
};

class EAleaTokenNotFoundError : public EAleaError
{
public:
	explicit EAleaTokenNotFoundError(const std::string& message)
		: EAleaError(message) {}
};

/**
 * @class GmDeck
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
class GmDeck
{
public:
	/**
	 * @brief Constructor - creates a new GmDeck with initial token IDs.
	 *
	 * @param token_ids       List of token IDs (uint32_t) to populate the deck.
	 * @param seed            Optional random seed for deterministic shuffling.
	 * @param auto_shuffle    If true, shuffle the deck after construction.
	 * @param allow_duplicates If false (default), throws EAleaDuplicateTokenIdError
	 *                        when the same ID appears more than once.  Set to
	 *                        true for probability decks where the same card type
	 *                        must appear multiple times (e.g. 8� Success, 2� Failure).
	 *
	 * @throws EAleaDuplicateTokenIdError if @p allow_duplicates is false and @p token_ids
	 *         contains duplicate IDs.
	 */
	explicit GmDeck(const std::vector<uint32_t>& token_ids,
					 std::optional<unsigned int> seed = std::nullopt,
					 bool auto_shuffle      = true,
					 bool allow_duplicates  = false);

	/**
	 * @brief Shuffles the deck using the stored RNG.
	 */
	void shuffle();

	/**
	 * @brief Draws a single token from the front of the deck.
	 * @return The drawn token ID (uint32_t)
	 * @throws EAleaDeckEmptyError if the deck is empty
	 */
	uint32_t draw_one();

	/**
	 * @brief Draws k tokens from the front of the deck.
	 * @param k Number of tokens to draw
	 * @return Vector of drawn token IDs
	 * @throws EAleaInvalidDrawCountError if k <= 0
	 * @throws EAleaDeckEmptyError if not enough tokens remain
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
	 * @param token_ids Optional new list of token IDs; if null, uses initial IDs.
	 * @throws EAleaDuplicateTokenIdError if the deck was constructed with
	 *         allow_duplicates=false and the new token_ids contain duplicates.
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
	 * @param token_id Token to add.
	 * @throws EAleaDuplicateTokenIdError if allow_duplicates is false and token_id
	 *         is already in the deck.
	 */
	void push_back(uint32_t token_id);

	/**
	 * @brief Prepends a token at the front (top) of the deck (no shuffle).
	 * @param token_id Token to add.
	 * @throws EAleaDuplicateTokenIdError if allow_duplicates is false and token_id
	 *         is already in the deck.
	 */
	void push_front(uint32_t token_id);

	/**
	 * @brief Views the next token at the top of the deck without removing it.
	 *
	 * Allows inspection of the top token without modifying deck structure.
	 * Useful for conditional logic: "if I draw this token, then...".
	 *
	 * @return The token ID at the top of the deck.
	 * @throws EAleaDeckEmptyError if the deck is empty.
	 */
	uint32_t see_top() const;

	/**
	 * @brief Views the token at the bottom of the deck without removing it.
	 *
	 * Allows inspection of the bottom token without modifying deck structure.
	 *
	 * @return The token ID at the bottom of the deck.
	 * @throws EAleaDeckEmptyError if the deck is empty.
	 */
	uint32_t see_bottom() const;

	/**
	 * @brief Finds, removes, and returns a specific token by ID.
	 * @param token_id Token to draw
	 * @return token_id (for convenience)
	 * @throws EAleaTokenNotFoundError if token_id is not in the deck
	 */
	uint32_t draw_specific(uint32_t token_id);

private:
	/**
	 * @brief Validates that token_ids contains no duplicates.
	 * @param token_ids List to validate
	 * @throws EAleaDuplicateTokenIdError if duplicates are found
	 */
	static void _validate_token_ids(const std::vector<uint32_t>& token_ids);

	std::vector<uint32_t> _deck;
	std::vector<uint32_t> _initial_token_ids;
	std::optional<unsigned int> _seed;
	bool _allow_duplicates;
	std::mt19937 _rng;
};

} // namespace gmAlea

#endif // GMALEA_GMDECK_HPP
