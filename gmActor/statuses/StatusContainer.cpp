/**
 * @file statuses/StatusContainer.cpp
 * @brief Implementation of StatusContainer.
 */

#include "gmActor/statuses/StatusContainer.hpp"

#include <algorithm>

namespace gmActor {

void StatusContainer::add(StatusInstance status, bool stackable)
{
	auto it = std::find_if(
		statuses_.begin(), statuses_.end(),
		[&](const StatusInstance& s) { return s.id == status.id; });

	if (it == statuses_.end())
	{
		statuses_.push_back(std::move(status));
	}
	else if (stackable)
	{
		it->stacks += status.stacks;
	}
	else
	{
		// Non-stackable: replace instance, keep stacks = 1 (D11)
		*it = std::move(status);
		it->stacks = 1;
	}
}

void StatusContainer::remove(const StatusId& id)
{
	statuses_.erase(
		std::remove_if(statuses_.begin(), statuses_.end(),
			[&](const StatusInstance& s) { return s.id == id; }),
		statuses_.end());
}

void StatusContainer::clear()
{
	statuses_.clear();
}

bool StatusContainer::has(const StatusId& id) const
{
	return std::any_of(statuses_.begin(), statuses_.end(),
		[&](const StatusInstance& s) { return s.id == id; });
}

std::optional<StatusInstance> StatusContainer::get(const StatusId& id) const
{
	for (const StatusInstance& s : statuses_)
	{
		if (s.id == id) return s;
	}
	return std::nullopt;
}

const std::vector<StatusInstance>& StatusContainer::all() const
{
    return statuses_;
}

} // namespace gmActor
