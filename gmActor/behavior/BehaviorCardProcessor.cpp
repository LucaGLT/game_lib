/**
 * @file behavior/BehaviorCardProcessor.cpp
 * @brief Implementation of BehaviorCardProcessor.
 */

#include "gmActor/behavior/BehaviorCardProcessor.hpp"

namespace gmActor {

// ── Constructor ───────────────────────────────────────────────────────────────

BehaviorCardProcessor::BehaviorCardProcessor(CardCatalogLookup catalog_lookup)
	: _catalog_lookup(std::move(catalog_lookup))
{}

// ── Private helper ────────────────────────────────────────────────────────────

bool BehaviorCardProcessor::run_steps(const std::vector<BehaviorStep>& steps,
                                      MonsterGroupState&                group,
                                      const ActorStore&                 /*store*/,
                                      const StepExecutor&               executor) const
{
	for (const BehaviorStep& step : steps)
	{
		bool any_succeeded = false;

		for (const ActorId& member_id : group.members)
		{
			bool ok = executor(group.actor_id, member_id, step);
			if (ok)
				any_succeeded = true;
		}

		// Advance timeline once per step regardless of outcome.
		group.timeline_position += step.timeline_cost;

		// If the step is mandatory and nobody could execute it, signal failure.
		if (!step.optional && !any_succeeded && !group.members.empty())
			return false;
	}

	return true;
}

// ── Main execution ────────────────────────────────────────────────────────────

void BehaviorCardProcessor::process_group_turn(MonsterGroupState&  group,
                                               const ActorStore&   store,
                                               const StepExecutor& executor) const
{
	if (group.active_behavior_card_id.empty() || group.members.empty())
		return;

	BehaviorCard card = _catalog_lookup(group.active_behavior_card_id);

	bool succeeded = run_steps(card.steps, group, store, executor);

	if (!succeeded)
		process_fallback(group, store, executor);
}

void BehaviorCardProcessor::process_fallback(MonsterGroupState&  group,
                                             const ActorStore&   store,
                                             const StepExecutor& executor) const
{
	if (group.active_behavior_card_id.empty())
		return;

	BehaviorCard card = _catalog_lookup(group.active_behavior_card_id);

	if (card.fallback_steps.empty())
	{
		// No fallback defined — pay minimum 1 tick to avoid infinite loops.
		group.timeline_position += 1;
		return;
	}

	run_steps(card.fallback_steps, group, store, executor);
}

} // namespace gmActor
