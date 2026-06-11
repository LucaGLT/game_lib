/**
 * @file serialization/ActorJson.cpp
 * @brief Stub implementations of to_json / from_json for all gmActor types.
 *
 * Phase 2: each function is a safe no-op stub.
 * Phase 4: replace stubs with full serialization logic.
 */

#include "gmActor/serialization/ActorJson.hpp"

namespace gmActor {

// ── Enum helpers (macro to reduce boilerplate in stubs) ───────────────────────
// Full implementations in Phase 4 will use nlohmann's NLOHMANN_JSON_SERIALIZE_ENUM
// or explicit switch statements.

#define STUB_ENUM_TO(Type)   void to_json(nlohmann::json& j, const Type& v)   { j = static_cast<int>(v); }
#define STUB_ENUM_FROM(Type) void from_json(const nlohmann::json& j, Type& v) { v = static_cast<Type>(j.get<int>()); }

STUB_ENUM_TO(ActorKind)          STUB_ENUM_FROM(ActorKind)
STUB_ENUM_TO(AreaPosition)       STUB_ENUM_FROM(AreaPosition)
STUB_ENUM_TO(ActorLifeState)     STUB_ENUM_FROM(ActorLifeState)
STUB_ENUM_TO(ItemKind)           STUB_ENUM_FROM(ItemKind)
STUB_ENUM_TO(EquipmentSlot)      STUB_ENUM_FROM(EquipmentSlot)
STUB_ENUM_TO(ModifierOperation)  STUB_ENUM_FROM(ModifierOperation)
STUB_ENUM_TO(ModifierDurationKind) STUB_ENUM_FROM(ModifierDurationKind)

#undef STUB_ENUM_TO
#undef STUB_ENUM_FROM

// ── Modifier ──────────────────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const ModifierDefinition& v)
{
    // TODO Phase 4
    j = nlohmann::json{{"id", v.id}};
}
void from_json(const nlohmann::json& j, ModifierDefinition& v)
{
    // TODO Phase 4
    j.at("id").get_to(v.id);
}

void to_json(nlohmann::json& j, const ModifierInstance& v)
{
    j = nlohmann::json{{"id", v.id}};
}
void from_json(const nlohmann::json& j, ModifierInstance& v)
{
    j.at("id").get_to(v.id);
}

// ── Status ────────────────────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const StatusDefinition& v)
{
    j = nlohmann::json{{"id", v.id}};
}
void from_json(const nlohmann::json& j, StatusDefinition& v)
{
    j.at("id").get_to(v.id);
}

void to_json(nlohmann::json& j, const StatusInstance& v)
{
    j = nlohmann::json{{"id", v.id}};
}
void from_json(const nlohmann::json& j, StatusInstance& v)
{
    j.at("id").get_to(v.id);
}

// ── Items ─────────────────────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const ItemDefinition& v)
{
    j = nlohmann::json{{"id", v.id}};
}
void from_json(const nlohmann::json& j, ItemDefinition& v)
{
    j.at("id").get_to(v.id);
}

void to_json(nlohmann::json& j, const ItemState& v)
{
    j = nlohmann::json{{"instance_id", v.instance_id}};
}
void from_json(const nlohmann::json& j, ItemState& v)
{
    j.at("instance_id").get_to(v.instance_id);
}

void to_json(nlohmann::json& j, const InventoryState& v)
{
    j = nlohmann::json{{"items", v.items()}};
}
void from_json(const nlohmann::json& j, InventoryState& v)
{
    for (const auto& id : j.at("items")) { v.add(id.get<std::string>()); }
}

void to_json(nlohmann::json& j, const EquipmentState& v)
{
    j = nlohmann::json{{"equipped", v.all_equipped()}};
}
void from_json(const nlohmann::json& j, EquipmentState& v)
{
    // TODO Phase 4: restore slot → item mapping
    (void)j; (void)v;
}

// ── Actor states ──────────────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const ActorStateCommon& v)
{
    j = nlohmann::json{{"actor_id", v.actor_id}, {"display_name", v.display_name}};
}
void from_json(const nlohmann::json& j, ActorStateCommon& v)
{
    j.at("actor_id").get_to(v.actor_id);
    j.at("display_name").get_to(v.display_name);
}

void to_json(nlohmann::json& j, const HeroState& v)
{
    j = nlohmann::json{{"common", v.common}};
}
void from_json(const nlohmann::json& j, HeroState& v)
{
    j.at("common").get_to(v.common);
}

void to_json(nlohmann::json& j, const AllyState& v)
{
    j = nlohmann::json{{"common", v.common}};
}
void from_json(const nlohmann::json& j, AllyState& v)
{
    j.at("common").get_to(v.common);
}

void to_json(nlohmann::json& j, const MonsterInstanceState& v)
{
    j = nlohmann::json{{"common", v.common}};
}
void from_json(const nlohmann::json& j, MonsterInstanceState& v)
{
    j.at("common").get_to(v.common);
}

void to_json(nlohmann::json& j, const MonsterGroupState& v)
{
    j = nlohmann::json{{"actor_id", v.actor_id}};
}
void from_json(const nlohmann::json& j, MonsterGroupState& v)
{
    j.at("actor_id").get_to(v.actor_id);
}

void to_json(nlohmann::json& j, const BossState& v)
{
    j = nlohmann::json{{"controller_group_id", v.controller_group_id}};
}
void from_json(const nlohmann::json& j, BossState& v)
{
    j.at("controller_group_id").get_to(v.controller_group_id);
}

void to_json(nlohmann::json& j, const MissionSystemState& v)
{
    j = nlohmann::json{{"actor_id", v.actor_id}};
}
void from_json(const nlohmann::json& j, MissionSystemState& v)
{
    j.at("actor_id").get_to(v.actor_id);
}

void to_json(nlohmann::json& j, const ActorStore& /*v*/)
{
    // TODO Phase 4: serialize all maps
    j = nlohmann::json::object();
}
void from_json(const nlohmann::json& /*j*/, ActorStore& /*v*/)
{
    // TODO Phase 4: deserialize all maps
}

} // namespace gmActor
