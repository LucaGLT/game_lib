#ifndef GMALEA_GMCOMPDECK_HPP
#define GMALEA_GMCOMPDECK_HPP

/**
 * @file GmCompDeck.hpp
 * @brief Multi-zone deck orchestrator for a single game entity.
 *
 * `GmCompDeck` manages the full card lifecycle — from the initial shuffled
 * draw deck all the way through hand, play area, discard pile, and permanent
 * banishment — while guaranteeing that **every token ID exists in exactly one
 * zone at all times**.
 */

#include "PolicyBasedDeck.hpp"
#include "CardLocation.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gmAlea
{

/**
 * @brief Callback fired after every token zone change.
 *
 * Parameters:
 *   - `token_id`      — the token that moved.
 *   - `rule_group_id` — the rule group linked to this token (empty if none).
 *   - `from`          — source zone.
 *   - `to`            — destination zone.
 *
 * The callback is called **after** the move has already occurred and the
 * uniqueness invariant has been restored.  It is safe to call any const
 * method of `GmCompDeck` inside the callback.
 *
 * @note `GmCompDeck` stores at most one callback.  Registering a new one
 *       replaces the previous one silently.
 */
using ZoneChangeCallback = std::function<
	void(uint32_t           token_id,
	     const std::string& rule_group_id,
	     ZoneId             from,
	     ZoneId             to)
>;

/**
 * @class GmCompDeck
 * @brief Multi-zone deck manager for a single game entity (player, faction, event source).
 *
 * @invariant Every token registered with this `GmCompDeck` exists in **exactly
 *            one** of its six zones at any point in time.
 *
 * ## Zones
 *
 * | Zone        | Type alias   | Key constraints                              |
 * |-------------|--------------|----------------------------------------------|
 * | Main Deck   | `MainDeck`   | Shufflable; draw from top; pick by ID        |
 * | Hand        | `CardHand`   | No shuffle; pick any card by ID              |
 * | Play Area   | `PlayArea`   | No shuffle; order preserved; pick by ID      |
 * | Memory      | `MemoryZone` | No shuffle; no sequential draw; pick by ID   |
 * | Discard     | `DiscardPile`| Order **sacred** (no shuffle); pick by ID   |
 * | Banish Zone | `BanishZone` | Insert-only; no retrieval                    |
 *
 * ## Ownership model
 *
 * `GmCompDeck` owns all six zone objects.  External code may **read** them
 * via the `const` accessors (`main_deck()`, `hand()`, …).  All **writes**
 * (card movements) must go through `GmCompDeck` methods to preserve the
 * uniqueness invariant.
 *
 * ## Typical game flow
 * @code
 *   gmAlea::GmCompDeck player("Alice", {101, 102, 103, 104, 105, 106, 107});
 *
 *   player.draw_to_hand(3);            // Main Deck → Hand
 *   player.play_card(102);             // Hand → Play Area
 *   player.resolve_card(102);          // Play Area → Discard
 *   player.take_from_discard(102);     // Discard → Hand
 *   player.banish(102);                // Hand → Banish (permanent)
 *
 *   gmAlea::ZoneId loc = player.locate(102); // → ZoneId::BANISHED
 * @endcode
 */
class GmCompDeck
{
public:
	// ─────────────────────────────────────────────────────────────────────────
	// Construction / Destruction
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Constructs a `GmCompDeck` with a named owner and an optional initial deck.
	 *
	 * All tokens in `deck_tokens` are loaded into the **main deck**.  The main
	 * deck is shuffled automatically (respecting `MainDeckPolicy::can_shuffle`).
	 * All other zones start empty.
	 *
	 * @param owner_name  Human-readable identifier for the entity (e.g. `"Player1"`).
	 * @param deck_tokens Token IDs that populate the main deck.  May be empty.
	 * @param seed        Optional RNG seed for deterministic shuffle.
	 * @throws EAleaDuplicateTokenIdError if `deck_tokens` contains duplicate IDs.
	 */
	explicit GmCompDeck(std::string                  owner_name,
						const std::vector<uint32_t>& deck_tokens = {},
						std::optional<unsigned int>  seed        = std::nullopt);

	~GmCompDeck() = default;

	GmCompDeck(const GmCompDeck&)            = delete;
	GmCompDeck& operator=(const GmCompDeck&) = delete;

	GmCompDeck(GmCompDeck&&)            = default;
	GmCompDeck& operator=(GmCompDeck&&) = default;

	// ─────────────────────────────────────────────────────────────────────────
	// Cross-zone moves  — all guarantee the uniqueness invariant
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Draws `count` cards from the top of the main deck into hand.
	 *
	 * @param count Number of cards to draw.  Must be > 0.
	 * @throws EAleaInvalidDrawCountError if `count <= 0`.
	 * @throws EAleaDeckEmptyError if the main deck has fewer than `count` tokens.
	 */
	void draw_to_hand(int count);

	/**
	 * @brief Draws a specific token from anywhere in the main deck into hand.
	 *
	 * Searches the entire main deck for `token_id` (not just the top), removes
	 * it, and places it in hand.
	 *
	 * @param token_id Token to move.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the main deck.
	 */
	void draw_specific_to_hand(uint32_t token_id);

	/**
	 * @brief Plays a card from hand onto the play area.
	 *
	 * @param token_id Token to play.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in hand.
	 */
	void play_card(uint32_t token_id);

	/**
	 * @brief Resolves (removes from play) a card from the play area to the discard pile.
	 *
	 * @param token_id Token to resolve.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the play area.
	 */
	void resolve_card(uint32_t token_id);

	/**
	 * @brief Discards a card directly from hand to the discard pile.
	 *
	 * @param token_id Token to discard.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in hand.
	 */
	void discard_from_hand(uint32_t token_id);

	/**
	 * @brief Forcibly discards a card from the play area to the discard pile.
	 *
	 * @param token_id Token to discard.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the play area.
	 */
	void discard_from_table(uint32_t token_id);

	/**
	 * @brief Retrieves a specific card from the discard pile back to hand.
	 *
	 * The discard pile order is preserved for the remaining cards.
	 *
	 * @param token_id Token to retrieve.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the discard pile.
	 */
	void take_from_discard(uint32_t token_id);

	/**
	 * @brief Returns a specific card from the discard pile to the bottom of the main deck.
	 *
	 * @param token_id Token to return.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the discard pile.
	 */
	void return_from_discard_to_deck(uint32_t token_id);

	/**
	 * @brief Permanently banishes a token from any zone (except banish zone itself).
	 *
	 * Locates the token in whichever zone it currently occupies, removes it,
	 * and places it in the banish zone.  Banished tokens cannot be retrieved.
	 *
	 * @param token_id Token to banish.
	 * @throws EAleaTokenNotFoundError if `token_id` is not tracked by this `GmCompDeck`,
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
	// Memory zone moves
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Moves a card from hand to memory.
	 *
	 * @param token_id Token to remember.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in hand.
	 */
	void remember_from_hand(uint32_t token_id);

	/**
	 * @brief Moves a card from the play area to memory.
	 *
	 * @param token_id Token to remember.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the play area.
	 */
	void remember_from_play_area(uint32_t token_id);

	/**
	 * @brief Moves a card from the discard pile to memory.
	 *
	 * @param token_id Token to remember.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in the discard pile.
	 */
	void remember_from_discard(uint32_t token_id);

	/**
	 * @brief Moves a card from memory to the play area.
	 *
	 * The card enters play directly without passing through hand.
	 *
	 * @param token_id Token to play.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in memory.
	 */
	void play_from_memory(uint32_t token_id);

	/**
	 * @brief Moves a card from memory back to hand.
	 *
	 * @param token_id Token to return.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in memory.
	 */
	void return_memory_to_hand(uint32_t token_id);

	/**
	 * @brief Discards a card from memory to the discard pile.
	 *
	 * @param token_id Token to discard.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in memory.
	 */
	void discard_from_memory(uint32_t token_id);

	/**
	 * @brief Permanently banishes a card from memory.
	 *
	 * @param token_id Token to banish.
	 * @throws EAleaTokenNotFoundError if `token_id` is not in memory.
	 */
	void banish_from_memory(uint32_t token_id);

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
	 * Sum of main_deck + hand + play_area + memory + discard + banish.
	 */
	int total_count() const;

	/**
	 * @brief Returns the owner's name (set at construction).
	 */
	const std::string& owner_name() const;

	// ───────────────────────────────────────────────────────────────────────────
	// Rule-group binding
	// ───────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Associates a token with a rule group identifier.
	 *
	 * The `rule_group_id` string is carried alongside the token through all
	 * zone transitions and is included in every `ZoneChangeCallback` call.
	 * It is opaque to `GmCompDeck` — the game engine layer interprets it.
	 *
	 * Calling this method for a token that is already registered silently
	 * replaces the previous binding.
	 *
	 * @param token_id      Token to bind.
	 * @param rule_group_id Identifier of the rule group (e.g. `"rg_village"`).
	 *                      Pass `""` to clear an existing binding.
	 */
	void register_rule_group(uint32_t token_id, const std::string& rule_group_id);

	/**
	 * @brief Returns the rule group ID currently bound to a token, or `""`.
	 *
	 * @param token_id Token to query.
	 * @return The rule group identifier, or empty string if none registered.
	 */
	const std::string& rule_group_of(uint32_t token_id) const;

	/**
	 * @brief Registers the callback invoked after every zone change.
	 *
	 * At most one callback is stored.  Passing a default-constructed
	 * (empty) `ZoneChangeCallback` clears the existing callback.
	 *
	 * @param cb Callback to register.
	 */
	void set_zone_change_callback(ZoneChangeCallback cb);

	// ─────────────────────────────────────────────────────────────────────────
	// Read-only zone accessors
	// ─────────────────────────────────────────────────────────────────────────

	/** @brief Read-only access to the main deck zone. */
	const MainDeck&    main_deck()   const { return _main_deck;   }

	/** @brief Read-only access to the hand zone. */
	const CardHand&    hand()        const { return _hand;        }

	/** @brief Read-only access to the play area zone. */
	const PlayArea&    play_area()   const { return _play_area;   }

	/** @brief Read-only access to the discard pile zone. */
	const DiscardPile& discard()     const { return _discard;     }

	/** @brief Read-only access to the banish zone. */
	const BanishZone&  banish_zone() const { return _banish_zone; }

	/** @brief Read-only access to the memory zone. */
	const MemoryZone&  memory()      const { return _memory;      }

	/** @brief Returns the number of cards currently in memory. */
	int memory_size() const { return _memory.count(); }

	/** @brief Returns true if the given token is currently in memory. */
	bool is_in_memory(uint32_t token_id) const { return _memory.contains(token_id); }

private:
	/**
	 * @brief Removes a token from the specified zone.
	 *
	 * Internal helper used by cross-zone move methods.
	 * Must not be called with `ZoneId::BANISHED` or `ZoneId::NOT_FOUND`
	 * — those cases throw `EAleaTokenNotFoundError`.
	 *
	 * @param zone     Zone from which to remove the token.
	 * @param token_id Token to remove.
	 * @throws EAleaTokenNotFoundError if `zone` is BANISHED or NOT_FOUND.
	 */
	void _remove_from_zone(ZoneId zone, uint32_t token_id);

	/**
	 * @brief Fires the registered zone-change callback, if any.
	 *
	 * Called internally after every successful cross-zone move.
	 * No-op when no callback is registered or the token has no rule group.
	 */
	void _fire_zone_change(uint32_t token_id, ZoneId from, ZoneId to) const;

	static const std::string _EMPTY_RULE_GROUP_ID; ///< Sentinel empty string.

	std::string   _owner_name;
	MainDeck      _main_deck;
	CardHand      _hand;
	PlayArea      _play_area;
	MemoryZone    _memory;
	DiscardPile   _discard;
	BanishZone    _banish_zone;

	/// token_id → rule_group_id (empty string = no binding)
	std::unordered_map<uint32_t, std::string> _rule_group_map;

	ZoneChangeCallback _zone_change_cb; ///< Optional observer; default-constructed = empty.
};

} // namespace gmAlea

#endif // GMALEA_GMCOMPDECK_HPP
