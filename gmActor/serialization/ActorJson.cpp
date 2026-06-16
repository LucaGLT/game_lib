/**
 * @file serialization/ActorJson.cpp
 * @brief Minimal to_json / from_json implementations for gmActor types.
 *
 * Round-trip fidelity is guaranteed for:
 * - All enums (int-based encoding)
 * - ActorStateCommon (all fields)
 * - HeroState (common + key hero fields)
 * - AllyState, MonsterInstanceState, MonsterGroupState, BossState,
 *   MissionSystemState
 * - ActorStore (all internal maps)
 *
 * Non-critical structs (ItemDefinition, EquipmentState detail, etc.)
 * preserve only the ID/key fields in this version.
 */

#include "gmActor/serialization/ActorJson.hpp"

#include <vector>

namespace gmActor {

// ── Enum helpers ──────────────────────────────────────────────────────────────
// Encoding: underlying int value, consistent with stable enum ordering.

#define ENUM_TO(Type)   void to_json  (nlohmann::json& j, const Type& v) { j = static_cast<int>(v); }
#define ENUM_FROM(Type) void from_json(const nlohmann::json& j, Type& v) { v = static_cast<Type>(j.get<int>()); }

ENUM_TO(ActorKind)            ENUM_FROM(ActorKind)
ENUM_TO(AreaPosition)         ENUM_FROM(AreaPosition)
ENUM_TO(ActorLifeState)       ENUM_FROM(ActorLifeState)
ENUM_TO(ItemKind)             ENUM_FROM(ItemKind)
ENUM_TO(EquipmentSlot)        ENUM_FROM(EquipmentSlot)
ENUM_TO(ModifierOperation)    ENUM_FROM(ModifierOperation)
ENUM_TO(ModifierDurationKind) ENUM_FROM(ModifierDurationKind)

#undef ENUM_TO
#undef ENUM_FROM

// ── ModifierDefinition / ModifierInstance ─────────────────────────────────────

void to_json(nlohmann::json& j, const ModifierDefinition& v)
{
	j = {
		{"id",        v.id},
		{"name",      v.name},
		{"stat_key",  v.stat_key},
		{"operation", v.operation},
		{"value",     v.value},
		{"tags",      v.tags}
	};
}
void from_json(const nlohmann::json& j, ModifierDefinition& v)
{
	j.at("id").get_to(v.id);
	j.at("name").get_to(v.name);
	j.at("stat_key").get_to(v.stat_key);
	j.at("operation").get_to(v.operation);
	j.at("value").get_to(v.value);
	j.at("tags").get_to(v.tags);
}

void to_json(nlohmann::json& j, const ModifierInstance& v)
{
	j = {
		{"id",            v.id},
		{"source_id",     v.source_id},
		{"stat_key",      v.stat_key},
		{"operation",     v.operation},
		{"value",         v.value},
		{"duration_kind", v.duration_kind},
		{"expires_at",    v.expires_at_time}
	};
}
void from_json(const nlohmann::json& j, ModifierInstance& v)
{
	j.at("id").get_to(v.id);
	j.at("source_id").get_to(v.source_id);
	j.at("stat_key").get_to(v.stat_key);
	j.at("operation").get_to(v.operation);
	j.at("value").get_to(v.value);
	j.at("duration_kind").get_to(v.duration_kind);
	j.at("expires_at").get_to(v.expires_at_time);
}

// ── StatusDefinition / StatusInstance ────────────────────────────────────────

void to_json(nlohmann::json& j, const StatusDefinition& v)
{
	j = {
		{"id",        v.id},
		{"name",      v.name},
		{"stackable", v.stackable},
		{"tags",      v.tags}
	};
}
void from_json(const nlohmann::json& j, StatusDefinition& v)
{
	j.at("id").get_to(v.id);
	j.at("name").get_to(v.name);
	j.at("stackable").get_to(v.stackable);
	j.at("tags").get_to(v.tags);
}

void to_json(nlohmann::json& j, const StatusInstance& v)
{
	j = {
		{"id",            v.id},
		{"source_id",     v.source_id},
		{"stacks",        v.stacks},
		{"duration_kind", v.duration_kind},
		{"expires_at",    v.expires_at_time},
		{"modifiers",     v.modifiers}
	};
}
void from_json(const nlohmann::json& j, StatusInstance& v)
{
	j.at("id").get_to(v.id);
	j.at("source_id").get_to(v.source_id);
	j.at("stacks").get_to(v.stacks);
	j.at("duration_kind").get_to(v.duration_kind);
	j.at("expires_at").get_to(v.expires_at_time);
	j.at("modifiers").get_to(v.modifiers);
}

// ── Items (minimal — IDs only) ────────────────────────────────────────────────

void to_json(nlohmann::json& j, const ItemDefinition& v)
{
	j = {{"id", v.id}, {"name", v.name}, {"kind", v.kind}};
}
void from_json(const nlohmann::json& j, ItemDefinition& v)
{
	j.at("id").get_to(v.id);
	j.at("name").get_to(v.name);
	j.at("kind").get_to(v.kind);
}

void to_json(nlohmann::json& j, const ItemState& v)
{
	j = {
		{"instance_id", v.instance_id},
		{"item_id",     v.item_id},
		{"owner_id",    v.owner_id},
		{"equipped",    v.equipped},
		{"slot",        v.slot},
		{"charges",     v.charges},
		{"exhausted",   v.exhausted}
	};
}
void from_json(const nlohmann::json& j, ItemState& v)
{
	j.at("instance_id").get_to(v.instance_id);
	j.at("item_id").get_to(v.item_id);
	j.at("owner_id").get_to(v.owner_id);
	j.at("equipped").get_to(v.equipped);
	j.at("slot").get_to(v.slot);
	j.at("charges").get_to(v.charges);
	j.at("exhausted").get_to(v.exhausted);
}

void to_json(nlohmann::json& j, const InventoryState& v)
{
	j = {{"items", v.items()}};
}
void from_json(const nlohmann::json& j, InventoryState& v)
{
	for (const auto& id : j.at("items"))
		v.add(id.get<std::string>());
}

// EquipmentState: serialise as array of {slot, item} pairs.
static const std::vector<EquipmentSlot> ALL_EQUIPMENT_SLOTS = {
	EquipmentSlot::MAIN_HAND,
	EquipmentSlot::OFF_HAND,
	EquipmentSlot::ARMOR,
	EquipmentSlot::TRINKET_1,
	EquipmentSlot::TRINKET_2,
	EquipmentSlot::RELIC
};

void to_json(nlohmann::json& j, const EquipmentState& v)
{
	nlohmann::json slots = nlohmann::json::array();
	for (EquipmentSlot s : ALL_EQUIPMENT_SLOTS)
	{
		std::optional<ItemInstanceId> item = v.equipped_at(s);
		if (item.has_value())
			slots.push_back({{"slot", static_cast<int>(s)}, {"item", *item}});
	}
	j = {{"slots", slots}};
}
void from_json(const nlohmann::json& j, EquipmentState& v)
{
	for (const auto& entry : j.at("slots"))
	{
		EquipmentSlot slot = static_cast<EquipmentSlot>(entry.at("slot").get<int>());
		std::string   item = entry.at("item").get<std::string>();
		v.equip(slot, item);
	}
}

// ── ActorStateCommon (full round-trip) ────────────────────────────────────────

void to_json(nlohmann::json& j, const ActorStateCommon& v)
{
	j = {
		{"actor_id",         v.actor_id},
		{"kind",             v.kind},
		{"display_name",     v.display_name},
		{"faction_id",       v.faction_id},
		{"enabled",          v.enabled},
		{"removed",          v.removed},
		{"can_act",          v.can_act},
		{"can_be_targeted",  v.can_be_targeted},
		{"life_state",       v.life_state},
		{"timeline_pos",     v.timeline_position},
		{"tie_break_rank",   v.tie_break_rank},
		{"area_id",          v.area_id},
		{"area_position",    v.area_position},
		{"current_hp",       v.current_hp},
		{"max_hp",           v.max_hp},
		{"statuses",         v.statuses},
		{"active_modifiers", v.active_modifiers},
		{"tags",             v.tags}
	};
}
void from_json(const nlohmann::json& j, ActorStateCommon& v)
{
	j.at("actor_id").get_to(v.actor_id);
	j.at("kind").get_to(v.kind);
	j.at("display_name").get_to(v.display_name);
	j.at("faction_id").get_to(v.faction_id);
	j.at("enabled").get_to(v.enabled);
	j.at("removed").get_to(v.removed);
	j.at("can_act").get_to(v.can_act);
	j.at("can_be_targeted").get_to(v.can_be_targeted);
	j.at("life_state").get_to(v.life_state);
	j.at("timeline_pos").get_to(v.timeline_position);
	j.at("tie_break_rank").get_to(v.tie_break_rank);
	j.at("area_id").get_to(v.area_id);
	j.at("area_position").get_to(v.area_position);
	j.at("current_hp").get_to(v.current_hp);
	j.at("max_hp").get_to(v.max_hp);
	j.at("statuses").get_to(v.statuses);
	j.at("active_modifiers").get_to(v.active_modifiers);
	j.at("tags").get_to(v.tags);
}

// ── Concrete actor states ─────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const HeroState& v)
{
	j = {
		{"common",           v.common},
		{"level",            v.level},
		{"hand_limit",       v.hand_limit},
		{"memory_limit",     v.memory_limit},
		{"total_deck_id",    v.total_deck_id},
		{"mission_deck_id",  v.mission_deck_id},
		{"equipment",        v.equipment},
		{"inventory",        v.inventory},
		{"affiliations",     v.affiliations},
		{"is_ko",            v.is_ko},
		{"carried_mission",  v.carried_mission_items}
	};
}
void from_json(const nlohmann::json& j, HeroState& v)
{
	j.at("common").get_to(v.common);
	j.at("level").get_to(v.level);
	j.at("hand_limit").get_to(v.hand_limit);
	j.at("memory_limit").get_to(v.memory_limit);
	j.at("total_deck_id").get_to(v.total_deck_id);
	j.at("mission_deck_id").get_to(v.mission_deck_id);
	j.at("equipment").get_to(v.equipment);
	j.at("inventory").get_to(v.inventory);
	j.at("affiliations").get_to(v.affiliations);
	j.at("is_ko").get_to(v.is_ko);
	j.at("carried_mission").get_to(v.carried_mission_items);
}

void to_json(nlohmann::json& j, const AllyState& v)
{
	j = {{"common", v.common}, {"traits", v.traits}, {"carried_items", v.carried_items}};
}
void from_json(const nlohmann::json& j, AllyState& v)
{
	j.at("common").get_to(v.common);
	j.at("traits").get_to(v.traits);
	j.at("carried_items").get_to(v.carried_items);
}

void to_json(nlohmann::json& j, const MonsterInstanceState& v)
{
	j = {
		{"common",          v.common},
		{"monster_type_id", v.monster_type_id},
		{"group_id",        v.group_id},
		{"elite",           v.elite},
		{"boss_part",       v.boss_part},
		{"base_damage",     v.base_damage},
		{"base_movement",   v.base_movement},
		{"traits",          v.traits},
		{"loot_ref",        v.loot_ref}
	};
}
void from_json(const nlohmann::json& j, MonsterInstanceState& v)
{
	j.at("common").get_to(v.common);
	j.at("monster_type_id").get_to(v.monster_type_id);
	j.at("group_id").get_to(v.group_id);
	j.at("elite").get_to(v.elite);
	j.at("boss_part").get_to(v.boss_part);
	j.at("base_damage").get_to(v.base_damage);
	j.at("base_movement").get_to(v.base_movement);
	j.at("traits").get_to(v.traits);
	j.at("loot_ref").get_to(v.loot_ref);
}

void to_json(nlohmann::json& j, const MonsterGroupState& v)
{
	j = {
		{"actor_id",         v.actor_id},
		{"group_id",         v.group_id},
		{"monster_type_id",  v.monster_type_id},
		{"display_name",     v.display_name},
		{"enabled",          v.enabled},
		{"removed",          v.removed},
		{"timeline_pos",     v.timeline_position},
		{"tie_break_rank",   v.tie_break_rank},
		{"members",          v.members},
		{"behavior_deck_id", v.behavior_deck_id},
		{"active_card_id",   v.active_behavior_card_id},
		{"discard_id",       v.behavior_discard_id},
		{"group_modifiers",  v.active_group_modifiers},
		{"tags",             v.tags}
	};
}
void from_json(const nlohmann::json& j, MonsterGroupState& v)
{
	j.at("actor_id").get_to(v.actor_id);
	j.at("group_id").get_to(v.group_id);
	j.at("monster_type_id").get_to(v.monster_type_id);
	j.at("display_name").get_to(v.display_name);
	j.at("enabled").get_to(v.enabled);
	j.at("removed").get_to(v.removed);
	j.at("timeline_pos").get_to(v.timeline_position);
	j.at("tie_break_rank").get_to(v.tie_break_rank);
	j.at("members").get_to(v.members);
	j.at("behavior_deck_id").get_to(v.behavior_deck_id);
	j.at("active_card_id").get_to(v.active_behavior_card_id);
	j.at("discard_id").get_to(v.behavior_discard_id);
	j.at("group_modifiers").get_to(v.active_group_modifiers);
	j.at("tags").get_to(v.tags);
}

void to_json(nlohmann::json& j, const BossState& v)
{
	j = {
		{"body_instance_id",       v.body_instance_id},
		{"controller_group_id",    v.controller_group_id},
		{"phase_index",            v.phase_index},
		{"rage",                   v.rage},
		{"linked_objectives",      v.linked_objectives},
		{"tags",                   v.tags}
	};
}
void from_json(const nlohmann::json& j, BossState& v)
{
	j.at("body_instance_id").get_to(v.body_instance_id);
	j.at("controller_group_id").get_to(v.controller_group_id);
	j.at("phase_index").get_to(v.phase_index);
	j.at("rage").get_to(v.rage);
	j.at("linked_objectives").get_to(v.linked_objectives);
	j.at("tags").get_to(v.tags);
}

void to_json(nlohmann::json& j, const MissionSystemState& v)
{
	j = {{"actor_id", v.actor_id}, {"display_name", v.display_name},
	     {"enabled", v.enabled}, {"tags", v.tags}};
}
void from_json(const nlohmann::json& j, MissionSystemState& v)
{
	j.at("actor_id").get_to(v.actor_id);
	j.at("display_name").get_to(v.display_name);
	j.at("enabled").get_to(v.enabled);
	j.at("tags").get_to(v.tags);
}

// ── ActorStore (full maps) ────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const ActorStore& v)
{
	// heroes: serialize as JSON array (key preserved inside each HeroState.common.actor_id)
	nlohmann::json heroes_arr = nlohmann::json::array();
	for (const auto& kv : v.heroes())
		heroes_arr.push_back(kv.second);

	nlohmann::json allies_arr = nlohmann::json::array();
	for (const auto& kv : v.allies())
		allies_arr.push_back(kv.second);

	nlohmann::json monsters_arr = nlohmann::json::array();
	for (const auto& kv : v.monster_instances())
		monsters_arr.push_back(kv.second);

	nlohmann::json groups_arr = nlohmann::json::array();
	for (const auto& kv : v.monster_groups())
		groups_arr.push_back(kv.second);

	nlohmann::json bosses_arr = nlohmann::json::array();
	for (const auto& kv : v.bosses())
		bosses_arr.push_back(kv.second);

	j = {
		{"heroes",           heroes_arr},
		{"allies",           allies_arr},
		{"monster_instances", monsters_arr},
		{"monster_groups",   groups_arr},
		{"bosses",           bosses_arr}
	};

	if (v.mission_system_opt().has_value())
		j["mission_system"] = *v.mission_system_opt();
}

void from_json(const nlohmann::json& j, ActorStore& v)
{
	for (const auto& entry : j.at("heroes"))
	{
		HeroState h;
		entry.get_to(h);
		v.add_hero(std::move(h));
	}
	for (const auto& entry : j.at("allies"))
	{
		AllyState a;
		entry.get_to(a);
		v.add_ally(std::move(a));
	}
	for (const auto& entry : j.at("monster_instances"))
	{
		MonsterInstanceState m;
		entry.get_to(m);
		v.add_monster_instance(std::move(m));
	}
	for (const auto& entry : j.at("monster_groups"))
	{
		MonsterGroupState g;
		entry.get_to(g);
		v.add_monster_group(std::move(g));
	}
	for (const auto& entry : j.at("bosses"))
	{
		BossState b;
		entry.get_to(b);
		v.add_boss(std::move(b));
	}
	if (j.contains("mission_system"))
	{
		MissionSystemState sys;
		j.at("mission_system").get_to(sys);
		v.set_mission_system(std::move(sys));
	}
}

} // namespace gmActor
