/**
 * @file statuses/StatusContainer.cpp
 * @brief Implementation of StatusContainer.
 */

#include "gmActor/statuses/StatusContainer.hpp"

#include <algorithm>

namespace gmActor {

void StatusContainer::add(StatusInstance status, bool stackable)
{
    // TODO Phase 4: implement stackable / replace logic.
    (void)stackable;
    statuses_.push_back(std::move(status));
}

void StatusContainer::remove(const StatusId& id)
{
    // TODO Phase 4: erase by id.
    (void)id;
}

void StatusContainer::clear()
{
    statuses_.clear();
}

bool StatusContainer::has(const StatusId& id) const
{
    // TODO Phase 4: search by id.
    (void)id;
    return false;
}

std::optional<StatusInstance> StatusContainer::get(const StatusId& id) const
{
    // TODO Phase 4: find and return.
    (void)id;
    return std::nullopt;
}

const std::vector<StatusInstance>& StatusContainer::all() const
{
    return statuses_;
}

} // namespace gmActor
