/**
 * @file behavior/BehaviorReactionSystem.cpp
 * @brief Implementation of BehaviorReactionSystem.
 */

#include "gmActor/behavior/BehaviorReactionSystem.hpp"

namespace gmActor {

// ── Constructor ───────────────────────────────────────────────────────────────

BehaviorReactionSystem::BehaviorReactionSystem(CardCatalogLookup catalog_lookup)
	: _catalog_lookup(std::move(catalog_lookup))
{}

// ── Query ─────────────────────────────────────────────────────────────────────

bool BehaviorReactionSystem::has_reaction(const MonsterGroupState& group,
                                          const std::string&       trigger_event_type) const
{
	if (group.active_behavior_card_id.empty() || trigger_event_type.empty())
		return false;

	BehaviorCard card = _catalog_lookup(group.active_behavior_card_id);

	return !card.reaction_trigger.empty()
	    && card.reaction_trigger == trigger_event_type;
}

// ── Execution ─────────────────────────────────────────────────────────────────

bool BehaviorReactionSystem::fire_reaction(MonsterGroupState&     group,
                                           const BehaviorDeckOp&  discard_and_draw,
                                           const StepExecutor&    executor) const
{
	BehaviorCard card = _catalog_lookup(group.active_behavior_card_id);

	// Execute reaction steps for each member.
	for (const BehaviorStep& step : card.reaction_steps)
	{
		for (const ActorId& member_id : group.members)
			executor(group.actor_id, member_id, step);
	}

	// Discard the active card and draw the next one.
	CardId next_card = discard_and_draw();
	group.active_behavior_card_id = next_card;

	return card.reaction_interrupts;
}

} // namespace gmActor
