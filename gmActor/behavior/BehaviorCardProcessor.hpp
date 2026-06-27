#ifndef GMACTOR_BEHAVIOR_BEHAVIORCARDPROCESSOR_HPP
#define GMACTOR_BEHAVIOR_BEHAVIORCARDPROCESSOR_HPP

/**
 * @file behavior/BehaviorCardProcessor.hpp
 * @brief Executes the step sequence of a monster group's active behavior card.
 *
 * `BehaviorCardProcessor` is a stateless service object.  It iterates the
 * steps of the currently active behavior card, calls the injected
 * `StepExecutor` for each member of the group, and advances the group's
 * `timeline_position` by the cost defined in each step.
 *
 * ### Zero-coupling design
 *
 * The processor has **no knowledge of game domain objects** (heroes, damage
 * formulas, target selection).  All such knowledge lives in the `StepExecutor`
 * lambda that the game adapter provides at the call site.
 *
 * ### Execution semantics
 *
 * For each `BehaviorStep` in the active card:
 *
 * 1. For each member in `group.members`, call
 *    `executor(group.actor_id, member_id, step)`.
 *    - A return value of `true` means the step succeeded for that member.
 *    - A return value of `false` means the step was unresolvable for that
 *      member (target unreachable, effect blocked, …).
 *
 * 2. If the step is **not optional** and **every member returned false**,
 *    the main sequence is abandoned and `process_fallback()` is called.
 *    Processing then stops.
 *
 * 3. `group.timeline_position += step.timeline_cost` is applied **once per
 *    step**, regardless of how many members executed it.
 *
 * ### Fallback semantics
 *
 * `process_fallback()` runs `BehaviorCard::fallback_steps` in the same way.
 * If `fallback_steps` is empty, the group pays the cost of the first failed
 * mandatory step and ends its turn.
 *
 * ### Usage example
 *
 * @code
 *   // At game setup — build a card catalog.
 *   std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> catalog = ...;
 *   auto lookup = [&catalog](const gmActor::CardId& id) { return catalog.at(id); };
 *
 *   gmActor::BehaviorCardProcessor processor(lookup);
 *
 *   // Each time a monster group activates on the timeline:
 *   processor.process_group_turn(group, store,
 *       [&](const gmActor::ActorId& grp,
 *           const gmActor::ActorId& member,
 *           const gmActor::BehaviorStep& step) -> bool
 *       {
 *           // game-specific logic: find target, apply effect ...
 *           return /* true if executed, false if impossible *\/;
 *       });
 * @endcode
 */

#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/core/Ids.hpp"

#include <functional>

namespace gmActor {

/**
 * @brief Callback type for looking up a BehaviorCard by its CardId.
 *
 * The function must return a valid `BehaviorCard` for every `CardId` that
 * can appear as `MonsterGroupState::active_behavior_card_id`.  Throwing an
 * exception from this callback will propagate to the caller.
 */
using CardCatalogLookup = std::function<BehaviorCard(const CardId& card_id)>;

/**
 * @brief Callback type that executes one behavior step for one monster member.
 *
 * @param group_id   The acting monster group's actor ID.
 * @param member_id  The specific monster instance executing this step.
 * @param step       The step to execute.
 * @return `true` if the step was executed successfully for this member;
 *         `false` if it could not be executed (target missing, blocked, …).
 */
using StepExecutor = std::function<bool(const ActorId&       group_id,
                                        const ActorId&       member_id,
                                        const BehaviorStep&  step)>;

/**
 * @class BehaviorCardProcessor
 * @brief Executes a monster group's active behavior card via a step sequence.
 */
class BehaviorCardProcessor
{
public:
	// ── Constructor ───────────────────────────────────────────────────────────

	/**
	 * @brief Constructs the processor with a catalog lookup function.
	 *
	 * @param catalog_lookup  Callable that returns a `BehaviorCard` given a `CardId`.
	 *                        Called during `process_group_turn()` and
	 *                        `process_fallback()` to resolve the active card.
	 */
	explicit BehaviorCardProcessor(CardCatalogLookup catalog_lookup);

	// ── Main execution ────────────────────────────────────────────────────────

	/**
	 * @brief Executes all steps of the group's active behavior card.
	 *
	 * Iterates `BehaviorCard::steps` in order.  For each step:
	 * - Calls `executor(group.actor_id, member_id, step)` for every member
	 *   listed in `group.members`.
	 * - Advances `group.timeline_position += step.timeline_cost` once.
	 * - If the step is mandatory and no member could execute it, calls
	 *   `process_fallback()` and returns immediately.
	 *
	 * No-op if `group.active_behavior_card_id` is empty or if
	 * `group.members` is empty.
	 *
	 * @param group     Monster group to activate (mutated: timeline advanced).
	 * @param store     Read-only actor store (passed through to executor context).
	 * @param executor  Game-provided step execution callback.
	 */
	void process_group_turn(MonsterGroupState&  group,
	                        const ActorStore&   store,
	                        const StepExecutor& executor) const;

	/**
	 * @brief Executes the fallback steps of the group's active behavior card.
	 *
	 * Called automatically by `process_group_turn()` when a mandatory step
	 * fails.  May also be called directly by the game engine (e.g. when the
	 * active card has already been identified as unexecutable before activation).
	 *
	 * Iterates `BehaviorCard::fallback_steps`.  If the fallback list is empty,
	 * the group's `timeline_position` is advanced by the minimum cost (1 tick)
	 * to prevent infinite loops.
	 *
	 * @param group     Monster group (mutated: timeline advanced).
	 * @param store     Read-only actor store.
	 * @param executor  Game-provided step execution callback.
	 */
	void process_fallback(MonsterGroupState&  group,
	                      const ActorStore&   store,
	                      const StepExecutor& executor) const;

private:
	CardCatalogLookup _catalog_lookup;

	/// Executes a list of steps, advancing group.timeline_position after each.
	/// Returns false if a non-optional step failed for all members.
	bool run_steps(const std::vector<BehaviorStep>& steps,
	               MonsterGroupState&                group,
	               const ActorStore&                 store,
	               const StepExecutor&               executor) const;
};

} // namespace gmActor

#endif // GMACTOR_BEHAVIOR_BEHAVIORCARDPROCESSOR_HPP
