/**
 * @file items/EquipmentState.cpp
 * @brief Implementation of EquipmentState.
 */

#include "gmActor/items/EquipmentState.hpp"

namespace gmActor {

void EquipmentState::equip(EquipmentSlot slot, ItemInstanceId item)
{
    if (slot == EquipmentSlot::NONE) {
        throw InvalidEquipmentSlotError("cannot equip to EquipmentSlot::NONE");
    }
    int key = static_cast<int>(slot);
    if (slots_.count(key) > 0) {
        throw InvalidEquipmentSlotError(
            "slot is already occupied by item '" + slots_.at(key) + "'");
    }
    slots_[key] = std::move(item);
}

void EquipmentState::unequip(EquipmentSlot slot)
{
    slots_.erase(static_cast<int>(slot));
}

bool EquipmentState::has_equipped(EquipmentSlot slot) const
{
    return slots_.count(static_cast<int>(slot)) > 0;
}

std::optional<ItemInstanceId> EquipmentState::equipped_at(EquipmentSlot slot) const
{
    auto it = slots_.find(static_cast<int>(slot));
    if (it == slots_.end()) return std::nullopt;
    return it->second;
}

std::vector<ItemInstanceId> EquipmentState::all_equipped() const
{
    std::vector<ItemInstanceId> result;
    result.reserve(slots_.size());
    for (const auto& kv : slots_) {
        result.push_back(kv.second);
    }
    return result;
}

} // namespace gmActor
