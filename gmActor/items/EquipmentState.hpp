#ifndef GMACTOR_ITEMS_EQUIPMENTSTATE_HPP
#define GMACTOR_ITEMS_EQUIPMENTSTATE_HPP

/**
 * @file items/EquipmentState.hpp
 * @brief Maps equipment slots to the item instances currently occupying them.
 *
 * `EquipmentState` stores `ItemInstanceId` references; actual item data lives
 * elsewhere.  Slot legality (whether an actor may use a given slot) belongs to
 * the game-specific engine.
 *
 * ## Constraints
 * - Each slot holds at most one item instance.
 * - Equipping to an already-occupied slot throws `InvalidEquipmentSlotError`.
 * - Equipping to `EquipmentSlot::NONE` throws `InvalidEquipmentSlotError`.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/core/Errors.hpp"

#include <optional>
#include <vector>
#include <unordered_map>

namespace gmActor {

/**
 * @brief Tracks which item instance occupies each equipment slot.
 */
class EquipmentState {
public:
    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * @brief Equips an item in the given slot.
     *
     * @param slot Destination slot.
     * @param item Item instance ID to equip.
     * @throws InvalidEquipmentSlotError if `slot == NONE`.
     * @throws InvalidEquipmentSlotError if the slot is already occupied.
     */
    void equip(EquipmentSlot slot, ItemInstanceId item);

    /**
     * @brief Removes the item from the given slot.  No-op if slot is empty.
     *
     * @param slot Slot to clear.
     */
    void unequip(EquipmentSlot slot);

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Returns true if the given slot has an item equipped.
     *
     * @param slot Slot to check.
     */
    bool has_equipped(EquipmentSlot slot) const;

    /**
     * @brief Returns the item instance ID equipped in the given slot.
     *
     * @param slot Slot to query.
     * @return     Optional containing the instance ID, or `std::nullopt`.
     */
    std::optional<ItemInstanceId> equipped_at(EquipmentSlot slot) const;

    /**
     * @brief Returns a list of all currently equipped item instance IDs.
     *
     * Order is not guaranteed.
     */
    std::vector<ItemInstanceId> all_equipped() const;

private:
    // Key: underlying int of EquipmentSlot enum value
    std::unordered_map<int, ItemInstanceId> slots_;
};

} // namespace gmActor

#endif // GMACTOR_ITEMS_EQUIPMENTSTATE_HPP
