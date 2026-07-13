#ifndef GMACTOR_BEHAVIOR_BEHAVIORREACTIONSYSTEM_HPP
#define GMACTOR_BEHAVIOR_BEHAVIORREACTIONSYSTEM_HPP

/**
 * @file behavior/BehaviorReactionSystem.hpp
 * @brief Handles out-of-turn reaction firing for monster behavior cards.
 *
 * Some behavior cards can react to events that happen during the heroes' turn
 * (e.g. "when a hero plays an attack card, this group attacks back").
 * `BehaviorReactionSystem` checks whether the group's active card has a
 * registered reaction and, if so, fires it.
 *
 * ### Zero-coupling design
 *
 * Like `BehaviorCardProcessor`, this class has no knowledge of game domain
 * objects.  The card catalog lookup and the step executor are injected by the
 * game adapter.
 *
 * Deck management (discard active card, draw new card) is delegated to a
 * `BehaviorDeckOp` callback so that the system remains independent of any
 * specific deck implementation (`GmCompDeck` or otherwise).
 *
 * ### Reaction life cycle
 *
 * ```
 * Engine fires event "gmflow.hero.played_card"
 *     ↓
 * BehaviorReactionSystem::has_reaction(group, "gmflow.hero.played_card") → true
 *     ↓
 * BehaviorReactionSystem::fire_reaction(group, discard_and_draw, executor)
 *     ↓  executes reaction_steps for each member
 *     ↓  calls discard_and_draw() → new CardId (or "" if deck empty)
 *     ↓  updates group.active_behavior_card_id
 *     returns card.reaction_interrupts  →  engine decides whether to halt hero turn
 * ```
 *
 * ### Providing `BehaviorDeckOp` from a `GmCompDeck`
 *
 * @code
 *   uint32_t active_token = token_registry.at(group.active_behavior_card_id);
 *
 *   auto discard_and_draw = [&](void) -> gmActor::CardId
 *   {
 *       behavior_deck.discard_from_table(active_token); // play area → discard
 *       if (behavior_deck.count_in(gmAlea::ZoneId::MAIN_DECK) == 0)
 *           behavior_deck.reshuffle_discard_into_deck();
 *       if (behavior_deck.count_in(gmAlea::ZoneId::MAIN_DECK) == 0)
 *           return "";  // deck exhausted
 *       behavior_deck.draw_to_hand(1);
 *       uint32_t next_token = behavior_deck.hand().tokens().back();
 *       behavior_deck.play_card(next_token);
 *       return card_registry.at(next_token);  // uint32_t → CardId mapping
 *   };
 * @endcode
 */

#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/behavior/BehaviorCardProcessor.hpp"   // re-uses StepExecutor + CardCatalogLookup
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/core/Ids.hpp"

#include <functional>
#include <string>

namespace gmActor {

/**
 * @brief Callback that performs the deck operation for a reaction.
 *
 * When called, the implementation must:
 * 1. Move the currently active behavior card to the discard zone.
 * 2. Draw the next card from the main deck (reshuffling from discard if needed).
 * 3. Return the `CardId` of the newly active card, or `""` if the deck is
 *    exhausted.
 *
 * The system sets `group.active_behavior_card_id` to the returned value.
 */
using BehaviorDeckOp = std::function<CardId()>;

/**
 * @class BehaviorReactionSystem
 * @brief Checks and fires out-of-turn reactions on monster behavior cards.
 */
class BehaviorReactionSystem
{
public:
	// ── Constructor ───────────────────────────────────────────────────────────

	/**
	 * @brief Constructs the system with a card catalog lookup.
	 *
	 * @param catalog_lookup  Same lookup used by `BehaviorCardProcessor`;
	 *                        both can share the same underlying catalog.
	 */
	explicit BehaviorReactionSystem(CardCatalogLookup catalog_lookup);

	// ── Query ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Returns `true` if the group's active card has a reaction matching
	 *        `trigger_event_type`.
	 *
	 * Comparison is exact (case-sensitive).  Returns `false` if
	 * `group.active_behavior_card_id` is empty or if the card's
	 * `reaction_trigger` is empty.
	 *
	 * @param group               Monster group to query.
	 * @param trigger_event_type  Engine event type string (e.g. `"gmflow.hero.played_card"`).
	 * @return `true` if the reaction trigger matches.
	 */
	bool has_reaction(const MonsterGroupState& group,
	                  const std::string&       trigger_event_type) const;

	// ── Execution ─────────────────────────────────────────────────────────────

	/**
	 * @brief Fires the reaction of the group's active behavior card.
	 *
	 * Execution order:
	 * 1. Executes `BehaviorCard::reaction_steps` via `executor` for each member
	 *    in `group.members` (in member order, step order).
	 * 2. Calls `discard_and_draw()` to discard the current card and draw the next.
	 * 3. Sets `group.active_behavior_card_id` to the returned `CardId`
	 *    (may be empty if the deck is exhausted).
	 *
	 * @note This method does **not** check `has_reaction()` first.  The caller
	 *       is responsible for calling `has_reaction()` before `fire_reaction()`.
	 *
	 * @param group            Monster group (mutated: `active_behavior_card_id` updated).
	 * @param discard_and_draw Callback that performs the deck swap.
	 * @param executor         Step execution callback (same type as in BehaviorCardProcessor).
	 * @return The value of `BehaviorCard::reaction_interrupts` — pass this back
	 *         to the engine to decide whether to halt the current hero action.
	 */
	bool fire_reaction(MonsterGroupState&  group,
	                   const BehaviorDeckOp& discard_and_draw,
	                   const StepExecutor& executor) const;

private:
	CardCatalogLookup _catalog_lookup;
};

} // namespace gmActor

#endif // GMACTOR_BEHAVIOR_BEHAVIORREACTIONSYSTEM_HPP
