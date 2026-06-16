#ifndef GMACTOR_ITEMS_INVENTORYSTATE_HPP
#define GMACTOR_ITEMS_INVENTORYSTATE_HPP

/**
 * @file items/InventoryState.hpp
 * @brief Ordered list of item instance IDs held by one actor.
 *
 * Inventory stores only `ItemInstanceId` references; the actual `ItemState`
 * lives in a game-engine-managed item registry.  Slot legality and capacity
 * limits belong to the game-specific engine.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Errors.hpp"

#include <vector>

namespace gmActor {

/**
 * @brief Manages the list of item instance IDs an actor is carrying.
 */
class InventoryState {
public:
    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * @brief Adds an item instance to the inventory.
     *
     * @param id Item instance ID to add.
     */
    void add(ItemInstanceId id);

    /**
     * @brief Removes an item instance from the inventory.
     *
     * @param id Item instance ID to remove.
     * @throws UnknownItemError if `id` is not in the inventory.
     */
    void remove(const ItemInstanceId& id);

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Returns true if the inventory contains the given item instance.
     *
     * @param id Item instance ID to check.
     */
    bool contains(const ItemInstanceId& id) const;

    /**
     * @brief Returns a const reference to all held item instance IDs.
     */
    const std::vector<ItemInstanceId>& items() const;

    /**
     * @brief Returns the number of items in the inventory.
     */
    int count() const;

private:
    std::vector<ItemInstanceId> items_;
};

} // namespace gmActor

#endif // GMACTOR_ITEMS_INVENTORYSTATE_HPP
