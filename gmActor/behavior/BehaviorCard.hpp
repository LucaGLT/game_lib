#ifndef GMACTOR_BEHAVIOR_BEHAVIORCARD_HPP
#define GMACTOR_BEHAVIOR_BEHAVIORCARD_HPP

/**
 * @file behavior/BehaviorCard.hpp
 * @brief Full definition of a behavior card, including steps, fallback, and reaction.
 *
 * `BehaviorCard` is a plain data object loaded from a game-data catalog.  It
 * has no behavior of its own.  The game adapter populates instances at startup
 * and provides them to `BehaviorCardProcessor` and `BehaviorReactionSystem`
 * via a `CardCatalogLookup` callback.
 *
 * ### Life cycle on the behavior deck
 *
 * ```
 * MainDeck → (draw) → PlayArea [active_behavior_card_id set on MonsterGroupState]
 *                             ↓ executor processes steps each turn
 *                             ↓ reaction fires  (optional)
 *                         Discard → (reshuffle if empty) → MainDeck
 * ```
 *
 * ### Reaction mechanics
 *
 * A card with a non-empty `reaction_trigger` can fire **outside the normal
 * turn order** when the engine emits the matching event type string.
 *
 * `BehaviorReactionSystem::has_reaction()` checks this field.
 * `BehaviorReactionSystem::fire_reaction()` executes `reaction_steps`,
 * then calls the provided `BehaviorDeckOp` to discard the card and draw a
 * replacement.
 */

#include "gmActor/behavior/BehaviorStep.hpp"
#include "gmActor/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @struct BehaviorCard
 * @brief Complete definition of a monster behavior card.
 */
struct BehaviorCard
{
	// ── Identity ──────────────────────────────────────────────────────────────
	CardId card_id;   ///< Matches `MonsterGroupState::active_behavior_card_id`.

	// ── Normal turn ───────────────────────────────────────────────────────────

	/**
	 * @brief Steps executed each time it is this group's activation.
	 *
	 * Steps are executed in order.  If a non-optional step cannot be resolved
	 * by the executor, the processor calls `process_fallback()` and stops.
	 */
	std::vector<BehaviorStep> steps;

	/**
	 * @brief Steps executed instead of `steps` when the main sequence fails.
	 *
	 * If empty, the group simply pays the first failed step's `timeline_cost`
	 * and ends its turn.
	 */
	std::vector<BehaviorStep> fallback_steps;

	// ── Reaction ──────────────────────────────────────────────────────────────

	/**
	 * @brief Engine event type string that triggers the reaction.
	 *
	 * Empty string means this card has no reaction.  Compared verbatim by
	 * `BehaviorReactionSystem::has_reaction()`.
	 */
	std::string reaction_trigger;

	/**
	 * @brief Steps executed when the reaction fires.
	 *
	 * May be empty (reaction fires but executes nothing — useful for
	 * "replace card immediately" behaviours).
	 */
	std::vector<BehaviorStep> reaction_steps;

	/**
	 * @brief When `true`, firing this reaction interrupts the current sequence.
	 *
	 * `BehaviorReactionSystem::fire_reaction()` returns this value to the
	 * caller so it can decide whether to halt the hero's action mid-resolution.
	 */
	bool reaction_interrupts = true;
};

} // namespace gmActor

#endif // GMACTOR_BEHAVIOR_BEHAVIORCARD_HPP
