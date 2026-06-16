#ifndef GMACTOR_SERIALIZATION_ACTORJSON_HPP
#define GMACTOR_SERIALIZATION_ACTORJSON_HPP

/**
 * @file serialization/ActorJson.hpp
 * @brief to_json / from_json declarations for all gmActor types.
 *
 * Uses nlohmann/json (via `gmSave/json.hpp`).  Implements the ADL convention
 * required by `gmSave`: free functions `to_json` / `from_json` in the same
 * namespace as the type.
 *
 * Include this header to enable JSON serialization for any gmActor struct.
 * The implementations live in `ActorJson.cpp`.
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/AllyState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/actors/BossState.hpp"
#include "gmActor/actors/MissionSystemState.hpp"
#include "gmActor/items/ItemDefinition.hpp"
#include "gmActor/items/ItemState.hpp"
#include "gmActor/items/InventoryState.hpp"
#include "gmActor/items/EquipmentState.hpp"
#include "gmActor/statuses/StatusDefinition.hpp"
#include "gmActor/statuses/StatusInstance.hpp"
#include "gmActor/modifiers/Modifier.hpp"

#include "gmSave/json.hpp"

namespace gmActor {

// ── Enums ──────────────────────────────────────────────────────────────────────
void to_json(nlohmann::json& j, const ActorKind& v);
void from_json(const nlohmann::json& j, ActorKind& v);

void to_json(nlohmann::json& j, const AreaPosition& v);
void from_json(const nlohmann::json& j, AreaPosition& v);

void to_json(nlohmann::json& j, const ActorLifeState& v);
void from_json(const nlohmann::json& j, ActorLifeState& v);

void to_json(nlohmann::json& j, const ItemKind& v);
void from_json(const nlohmann::json& j, ItemKind& v);

void to_json(nlohmann::json& j, const EquipmentSlot& v);
void from_json(const nlohmann::json& j, EquipmentSlot& v);

void to_json(nlohmann::json& j, const ModifierOperation& v);
void from_json(const nlohmann::json& j, ModifierOperation& v);

void to_json(nlohmann::json& j, const ModifierDurationKind& v);
void from_json(const nlohmann::json& j, ModifierDurationKind& v);

// ── Modifier ──────────────────────────────────────────────────────────────────
void to_json(nlohmann::json& j, const ModifierDefinition& v);
void from_json(const nlohmann::json& j, ModifierDefinition& v);

void to_json(nlohmann::json& j, const ModifierInstance& v);
void from_json(const nlohmann::json& j, ModifierInstance& v);

// ── Status ────────────────────────────────────────────────────────────────────
void to_json(nlohmann::json& j, const StatusDefinition& v);
void from_json(const nlohmann::json& j, StatusDefinition& v);

void to_json(nlohmann::json& j, const StatusInstance& v);
void from_json(const nlohmann::json& j, StatusInstance& v);

// ── Items ─────────────────────────────────────────────────────────────────────
void to_json(nlohmann::json& j, const ItemDefinition& v);
void from_json(const nlohmann::json& j, ItemDefinition& v);

void to_json(nlohmann::json& j, const ItemState& v);
void from_json(const nlohmann::json& j, ItemState& v);

void to_json(nlohmann::json& j, const InventoryState& v);
void from_json(const nlohmann::json& j, InventoryState& v);

void to_json(nlohmann::json& j, const EquipmentState& v);
void from_json(const nlohmann::json& j, EquipmentState& v);

// ── Actor states ──────────────────────────────────────────────────────────────
void to_json(nlohmann::json& j, const ActorStateCommon& v);
void from_json(const nlohmann::json& j, ActorStateCommon& v);

void to_json(nlohmann::json& j, const HeroState& v);
void from_json(const nlohmann::json& j, HeroState& v);

void to_json(nlohmann::json& j, const AllyState& v);
void from_json(const nlohmann::json& j, AllyState& v);

void to_json(nlohmann::json& j, const MonsterInstanceState& v);
void from_json(const nlohmann::json& j, MonsterInstanceState& v);

void to_json(nlohmann::json& j, const MonsterGroupState& v);
void from_json(const nlohmann::json& j, MonsterGroupState& v);

void to_json(nlohmann::json& j, const BossState& v);
void from_json(const nlohmann::json& j, BossState& v);

void to_json(nlohmann::json& j, const MissionSystemState& v);
void from_json(const nlohmann::json& j, MissionSystemState& v);

void to_json(nlohmann::json& j, const ActorStore& v);
void from_json(const nlohmann::json& j, ActorStore& v);

} // namespace gmActor

#endif // GMACTOR_SERIALIZATION_ACTORJSON_HPP
