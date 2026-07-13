/**
 * @file monsters/EldhomBehaviorAdapter.cpp
 * @brief Implementation of EldhomBehaviorAdapter and BehaviorDeckState.
 */

#include "GAME/Eldhom/CoreEngine/monsters/EldhomBehaviorAdapter.hpp"

#include <stdexcept>

namespace eldhom {

// ── BehaviorDeckState ─────────────────────────────────────────────────────────

gmActor::CardId BehaviorDeckState::current_card_id() const
{
	return cards.at(static_cast<std::size_t>(current));
}

void BehaviorDeckState::advance()
{
	if (cards.empty()) { return; }
	current = (current + 1) % static_cast<int>(cards.size());
}

// ── EldhomBehaviorAdapter ─────────────────────────────────────────────────────

EldhomBehaviorAdapter::EldhomBehaviorAdapter(
	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> catalog)
	: _catalog(std::make_shared<
		const std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>>(
			std::move(catalog)))
	, _processor(
		// Capture shared_ptr by value: safe after move/copy of the adapter.
		[cat = _catalog](const gmActor::CardId& id) -> gmActor::BehaviorCard
		{
			auto it = cat->find(id);
			if (it == cat->end())
			{
				throw std::out_of_range(
					"EldhomBehaviorAdapter: behavior card not found: " + id);
			}
			return it->second;
		})
{
}

void EldhomBehaviorAdapter::register_deck(
	const GroupId&   group_id,
	BehaviorDeckState deck)
{
	_decks[group_id] = std::move(deck);
}

gmActor::CardId EldhomBehaviorAdapter::current_card_id(const GroupId& group_id) const
{
	return _decks.at(group_id).current_card_id();
}

void EldhomBehaviorAdapter::advance_behavior_card(const GroupId& group_id)
{
	_decks.at(group_id).advance();
}

void EldhomBehaviorAdapter::process_group_turn(
	gmActor::MonsterGroupState& group,
	gmActor::ActorStore&        store,
	const StepExecutor&         executor)
{
	// Sync the active card ID from our deck state into the group state so that
	// BehaviorCardProcessor can look it up via the catalog.
	group.active_behavior_card_id = current_card_id(group.actor_id);

	_processor.process_group_turn(group, store, executor);
}

} // namespace eldhom
