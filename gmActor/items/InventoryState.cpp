/**
 * @file items/InventoryState.cpp
 * @brief Implementation of InventoryState.
 */

#include "gmActor/items/InventoryState.hpp"

#include <algorithm>

namespace gmActor {

void InventoryState::add(ItemInstanceId id)
{
    items_.push_back(std::move(id));
}

void InventoryState::remove(const ItemInstanceId& id)
{
    auto it = std::find(items_.begin(), items_.end(), id);
    if (it == items_.end()) {
        throw UnknownItemError(id);
    }
    items_.erase(it);
}

bool InventoryState::contains(const ItemInstanceId& id) const
{
    return std::find(items_.begin(), items_.end(), id) != items_.end();
}

const std::vector<ItemInstanceId>& InventoryState::items() const
{
    return items_;
}

int InventoryState::count() const
{
    return static_cast<int>(items_.size());
}

} // namespace gmActor
