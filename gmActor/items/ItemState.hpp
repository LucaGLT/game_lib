#ifndef GMACTOR_ITEMS_ITEMSTATE_HPP
#define GMACTOR_ITEMS_ITEMSTATE_HPP

/**
 * @file items/ItemState.hpp
 * @brief Mutable runtime state for one item instance.
 *
 * Each physical item in play has one `ItemState`.  Multiple instances of the
 * same `ItemDefinition` each have their own `ItemState` with a unique
 * `instance_id`.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"

namespace gmActor {

/**
 * @brief Tracks runtime values for a single item instance.
 */
struct ItemState {
    ItemInstanceId instance_id;                    ///< Unique runtime identifier
    ItemId         item_id;                        ///< Definition identifier
    ActorId        owner_id;                       ///< Actor currently holding this item

    bool           equipped = false;               ///< True if currently in an equipment slot
    EquipmentSlot  slot     = EquipmentSlot::NONE; ///< Active slot (NONE if unequipped)

    int            charges   = 0;                  ///< Remaining charges (0 if N/A)
    bool           exhausted = false;              ///< True if the item has been used this round
};

} // namespace gmActor

#endif // GMACTOR_ITEMS_ITEMSTATE_HPP
