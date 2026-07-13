#ifndef ELDHOM_MONSTERS_ELDHOMBEHAVIORADAPTER_HPP
#define ELDHOM_MONSTERS_ELDHOMBEHAVIORADAPTER_HPP

/**
 * @file monsters/EldhomBehaviorAdapter.hpp
 * @brief Thin adapter that maps Eldhom monster data to gmActor::BehaviorCardProcessor.
 *
 * `EldhomBehaviorAdapter` holds:
 * - a `gmActor::BehaviorCardProcessor` configured with the Eldhom behavior
 *   card catalog
 * - a simple per-group behavior deck state (round-robin card cycling) that
 *   replaces the heavier `gmCompDeck` for this prototype phase
 *
 * The adapter exposes:
 * - `process_group_turn()` — executes the behavior card for the given group
 * - `advance_behavior_card()` — cycles to the next card in the group's deck
 * - `current_card_id()` — returns the active card ID for a group
 *
 * ### Effect type keys used by the step executor (§22 / §23)
 *
 * | Key                         | Meaning                                          |
 * |-----------------------------|--------------------------------------------------|
 * | `"DEAL_DAMAGE"`             | Inflict `amount` damage on nearest valid target  |
 * | `"MOVE_TOWARD_PG"`          | Move group `amount` steps toward nearest PG      |
 * | `"MOVE_TOWARD_NEAREST_PG"`  | Alias for MOVE_TOWARD_PG                         |
 * | `"WAIT"`                    | Do nothing; pay timeline cost                    |
 *
 * The `StepExecutor` lambda is provided by `EldhomEngine` and performs the
 * actual target lookup and state mutation.
 */

#include "gmActor/behavior/BehaviorCardProcessor.hpp"
#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/behavior/BehaviorStep.hpp"
#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace eldhom {

/**
 * @brief Simple round-robin behavior deck for one monster group.
 *
 * This replaces `gmCompDeck` in the prototype (P6).  The deck is a fixed
 * ordered list; after each activation the current index advances by one
 * (wrapping around).
 */
struct BehaviorDeckState {
	std::vector<gmActor::CardId> cards;        ///< Ordered card IDs (≥ 1)
	int                          current = 0;  ///< Index of the active card

	/** @brief Returns the currently active card ID. */
	gmActor::CardId current_card_id() const;

	/** @brief Advances to the next card (wraps around). */
	void advance();
};

/**
 * @class EldhomBehaviorAdapter
 * @brief Bridges gmActor::BehaviorCardProcessor with Eldhom monster state.
 */
class EldhomBehaviorAdapter
{
public:
	/** @brief Callback type matching gmActor::StepExecutor. */
	using StepExecutor = gmActor::StepExecutor;

	/**
	 * @brief Constructs the adapter with a behavior card catalog.
	 *
	 * @param catalog  Map from `BCardId` to `gmActor::BehaviorCard`.
	 *                 All behavior cards referenced by `BehaviorDeckState`
	 *                 instances must be present.
	 */
	explicit EldhomBehaviorAdapter(
		std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> catalog);

	// ── Deck management ───────────────────────────────────────────────────────

	/**
	 * @brief Registers a behavior deck for a group.
	 *
	 * Must be called during mission setup for every monster group.
	 *
	 * @param group_id The group actor ID.
	 * @param deck     The initial deck state (≥ 1 card ID).
	 */
	void register_deck(const GroupId& group_id, BehaviorDeckState deck);

	/**
	 * @brief Returns the active behavior card ID for a group.
	 *
	 * @param group_id The group actor ID.
	 * @throws std::out_of_range if group_id has no registered deck.
	 */
	gmActor::CardId current_card_id(const GroupId& group_id) const;

	/**
	 * @brief Advances the behavior deck for a group (called after each turn).
	 *
	 * @param group_id The group actor ID.
	 */
	void advance_behavior_card(const GroupId& group_id);

	// ── Turn execution ────────────────────────────────────────────────────────

	/**
	 * @brief Executes the behavior card turn for a monster group.
	 *
	 * Updates `group.active_behavior_card_id` from the current deck state,
	 * then calls `BehaviorCardProcessor::process_group_turn`.
	 *
	 * @param group    Monster group state (modified: `timeline_position` advances).
	 * @param store    Actor store for target queries and state mutation.
	 * @param executor Game-side lambda that resolves one step for one member.
	 */
	void process_group_turn(
		gmActor::MonsterGroupState& group,
		gmActor::ActorStore&        store,
		const StepExecutor&         executor);

private:
	// shared_ptr keeps the catalog alive in the BehaviorCardProcessor lambda
	// even if the adapter is moved after construction.
	std::shared_ptr<const std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>>
	                                                           _catalog;
	std::unordered_map<GroupId, BehaviorDeckState>             _decks;
	gmActor::BehaviorCardProcessor                             _processor;
};

} // namespace eldhom

#endif // ELDHOM_MONSTERS_ELDHOMBEHAVIORADAPTER_HPP
