#ifndef GMALEA_ZONEPOLICY_HPP
#define GMALEA_ZONEPOLICY_HPP

/**
 * @file ZonePolicy.hpp
 * @brief Compile-time policy structs that define per-zone behavioural constraints.
 *
 * Each policy struct provides three `constexpr bool` flags that are evaluated
 * at **compile time** by @ref PolicyBasedDeck via `static_assert`:
 *
 * | Flag                | Meaning                                               |
 * |---------------------|-------------------------------------------------------|
 * | `can_shuffle`       | Whether `shuffle()` is permitted on this zone.        |
 * | `can_direct_access` | Whether `take_specific()` is permitted on this zone.  |
 * | `is_insert_only`    | Whether `draw()` and `take_specific()` are forbidden. |
 *
 * ### How to define a custom policy
 * @code
 *   struct MyZonePolicy
 {
 *       static constexpr bool can_shuffle       = false;
 *       static constexpr bool can_direct_access = true;
 *       static constexpr bool is_insert_only    = false;
 *   };
 *   using MyZone = gmAlea::PolicyBasedDeck<MyZonePolicy>;
 * @endcode
 */

namespace gmAlea
{

// ─────────────────────────────────────────────────────────────────────────────
// Built-in zone policies
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Policy for the main shuffled draw deck.
 *
 * Supports shuffling and direct extraction of a specific card (e.g. a player
 * reaching into the deck to pull a specific card by ID).
 */
struct MainDeckPolicy
{
	static constexpr bool can_shuffle       = true;
	static constexpr bool can_draw          = true;
	static constexpr bool can_direct_access = true;
	static constexpr bool is_insert_only    = false;
	static constexpr bool preserves_order   = false; ///< Shuffled — order not sacred
};

/**
 * @brief Policy for the player's hand.
 *
 * Cards in hand are not shuffled.  Any card may be played or discarded by ID.
 */
struct HandPolicy
{
	static constexpr bool can_shuffle       = false;
	static constexpr bool can_draw          = true;
	static constexpr bool can_direct_access = true;
	static constexpr bool is_insert_only    = false;
	static constexpr bool preserves_order   = false; ///< No meaningful order in hand
};

/**
 * @brief Policy for the play area (cards on the table).
 *
 * Cards on the table are visible and accessible by ID.  Order is preserved
 * (represents play order).  No shuffle.
 */
struct PlayAreaPolicy
{
	static constexpr bool can_shuffle       = false;
	static constexpr bool can_draw          = true;
	static constexpr bool can_direct_access = true;
	static constexpr bool is_insert_only    = false;
	static constexpr bool preserves_order   = true; ///< Play order preserved
};

/**
 * @brief Policy for the discard pile.
 *
 * @note **ORDER IS SACRED** — the discard pile preserves insertion order so
 * that players can inspect the discard history.  `can_shuffle = false`
 * makes it a compile-time error to call `shuffle()` on a `DiscardPile`.
 *
 * Direct access is allowed (a player may pick a specific card from the discard).
 */
struct DiscardPolicy
{
	static constexpr bool can_shuffle       = false;   ///< Insertion order preserved
	static constexpr bool can_draw          = true;
	static constexpr bool can_direct_access = true;
	static constexpr bool is_insert_only    = false;
	static constexpr bool preserves_order   = true;    ///< ORDER IS SACRED
};

/**
 * @brief Policy for the banish zone (permanently removed cards).
 *
 * Cards in the banish zone cannot be drawn or retrieved — they are permanently
 * out of play.  `is_insert_only = true` prevents `draw()` and `take_specific()`
 * from even compiling on a `BanishZone`.
 */
struct BanishPolicy
{
	static constexpr bool can_shuffle       = false;
	static constexpr bool can_draw          = false;   ///< No sequential draw
	static constexpr bool can_direct_access = false;   ///< No retrieval by ID
	static constexpr bool is_insert_only    = true;    ///< Add-only
	static constexpr bool preserves_order   = true;    ///< Insertion order preserved
};

/**
 * @brief Policy for the memory zone (cards retained across immediate resolution).
 *
 * Memory is an ordered, inspectable list.  Cards enter via an explicit
 * "remember" action and leave via explicit "play / discard / banish" actions.
 * Sequential draw (top-of-deck style) is forbidden; removal is always by ID.
 *
 * | Flag              | Value | Rationale                                      |
 * |-------------------|-------|------------------------------------------------|
 * | can_shuffle       | false | Memory order is intentional                    |
 * | can_draw          | false | No sequential "draw from memory" as a deck     |
 * | can_direct_access | true  | Remove any specific card by ID                 |
 * | can_insert        | true  | Cards can be added (remembered)                |
 * | is_insert_only    | false | Retrieval IS allowed (unlike Banish)           |
 * | preserves_order   | true  | Insertion order is part of Memory semantics    |
 */
struct MemoryPolicy
{
	static constexpr bool can_shuffle       = false;
	static constexpr bool can_draw          = false;   ///< No deck-style sequential draw
	static constexpr bool can_direct_access = true;    ///< take_specific() allowed
	static constexpr bool can_insert        = true;    ///< Cards can be remembered
	static constexpr bool is_insert_only    = false;   ///< Not like Banish — retrieval OK
	static constexpr bool preserves_order   = true;    ///< Insertion order is semantic
};

} // namespace gmAlea

#endif // GMALEA_ZONEPOLICY_HPP
