/**
 * @file actors/ActorStore.cpp
 * @brief Stub implementation of ActorStore.
 *
 * Phase 2: all method bodies return safe placeholders.
 * Phase 4: replace TODO stubs with full logic.
 */

#include "gmActor/actors/ActorStore.hpp"

namespace gmActor {

// ── Registration ──────────────────────────────────────────────────────────────

void ActorStore::add_hero(HeroState hero)
{
	const ActorId id = hero.common.actor_id;
	if (kind_registry_.count(id) > 0) throw DuplicateActorError(id);
	kind_registry_[id] = ActorKind::HERO;
	heroes_.emplace(id, std::move(hero));
}

void ActorStore::add_ally(AllyState ally)
{
	const ActorId id = ally.common.actor_id;
	if (kind_registry_.count(id) > 0) throw DuplicateActorError(id);
	kind_registry_[id] = ActorKind::ALLY_NPC;
	allies_.emplace(id, std::move(ally));
}

void ActorStore::add_monster_instance(MonsterInstanceState monster)
{
	const ActorId id = monster.common.actor_id;
	if (kind_registry_.count(id) > 0) throw DuplicateActorError(id);
	kind_registry_[id] = ActorKind::MONSTER_INSTANCE;
	monsters_.emplace(id, std::move(monster));
}

void ActorStore::add_monster_group(MonsterGroupState group)
{
	const ActorId id = group.actor_id;
	if (kind_registry_.count(id) > 0) throw DuplicateActorError(id);
	kind_registry_[id] = ActorKind::MONSTER_GROUP;
	monster_groups_.emplace(id, std::move(group));
}

void ActorStore::add_boss(BossState boss)
{
	const ActorId key = boss.controller_group_id;
	if (bosses_.count(key) > 0) throw DuplicateActorError(key);
	bosses_.emplace(key, std::move(boss));
}

void ActorStore::set_mission_system(MissionSystemState system)
{
    mission_system_ = std::move(system);
}

// ── Existence / kind queries ───────────────────────────────────────────────────

bool ActorStore::has_actor(const ActorId& id) const
{
    return kind_registry_.count(id) > 0;
}

ActorKind ActorStore::actor_kind(const ActorId& id) const
{
    auto it = kind_registry_.find(id);
    if (it == kind_registry_.end()) throw UnknownActorError(id);
    return it->second;
}

// ── Common state accessors ────────────────────────────────────────────────────

ActorStateCommon& ActorStore::common(const ActorId& id)
{
	auto kit = kind_registry_.find(id);
	if (kit == kind_registry_.end()) throw UnknownActorError(id);

	switch (kit->second)
	{
		case ActorKind::HERO:
			return heroes_.at(id).common;
		case ActorKind::ALLY_NPC:
			return allies_.at(id).common;
		case ActorKind::MONSTER_INSTANCE:
			return monsters_.at(id).common;
		case ActorKind::MONSTER_GROUP:
			throw InvalidActorKindError(id, "use monster_group() for MONSTER_GROUP actors");
		default:
			throw InvalidActorKindError(id, "actor kind has no ActorStateCommon");
	}
}

const ActorStateCommon& ActorStore::common(const ActorId& id) const
{
    return const_cast<ActorStore*>(this)->common(id);
}

// ── Typed accessors ───────────────────────────────────────────────────────────

HeroState& ActorStore::hero(const ActorId& id)
{
    auto it = heroes_.find(id);
    if (it == heroes_.end()) throw UnknownActorError(id);
    return it->second;
}

const HeroState& ActorStore::hero(const ActorId& id) const
{
    return const_cast<ActorStore*>(this)->hero(id);
}

AllyState& ActorStore::ally(const ActorId& id)
{
    auto it = allies_.find(id);
    if (it == allies_.end()) throw UnknownActorError(id);
    return it->second;
}

const AllyState& ActorStore::ally(const ActorId& id) const
{
    return const_cast<ActorStore*>(this)->ally(id);
}

MonsterInstanceState& ActorStore::monster_instance(const MonsterInstanceId& id)
{
    auto it = monsters_.find(id);
    if (it == monsters_.end()) throw UnknownActorError(id);
    return it->second;
}

const MonsterInstanceState& ActorStore::monster_instance(const MonsterInstanceId& id) const
{
    return const_cast<ActorStore*>(this)->monster_instance(id);
}

MonsterGroupState& ActorStore::monster_group(const ActorId& id)
{
    auto it = monster_groups_.find(id);
    if (it == monster_groups_.end()) throw UnknownActorError(id);
    return it->second;
}

const MonsterGroupState& ActorStore::monster_group(const ActorId& id) const
{
    return const_cast<ActorStore*>(this)->monster_group(id);
}

BossState& ActorStore::boss(const ActorId& controller_group_id)
{
    auto it = bosses_.find(controller_group_id);
    if (it == bosses_.end()) throw UnknownActorError(controller_group_id);
    return it->second;
}

const BossState& ActorStore::boss(const ActorId& controller_group_id) const
{
    return const_cast<ActorStore*>(this)->boss(controller_group_id);
}

MissionSystemState& ActorStore::mission_system()
{
    if (!mission_system_.has_value())
        throw UnknownActorError("system_mission");
    return *mission_system_;
}

const MissionSystemState& ActorStore::mission_system() const
{
    return const_cast<ActorStore*>(this)->mission_system();
}

// ── Bulk queries ──────────────────────────────────────────────────────────────

std::vector<ActorId> ActorStore::timeline_actor_ids() const
{
	std::vector<ActorId> result;

	// Heroes (always on the timeline unless removed)
	for (const auto& kv : heroes_)
	{
		const ActorStateCommon& c = kv.second.common;
		if (!c.removed)
			result.push_back(kv.first);
	}

	// Allies: only those that can act
	for (const auto& kv : allies_)
	{
		const ActorStateCommon& c = kv.second.common;
		if (!c.removed && c.can_act)
			result.push_back(kv.first);
	}

	// Monster groups: enabled and not removed (D3)
	for (const auto& kv : monster_groups_)
	{
		const MonsterGroupState& g = kv.second;
		if (g.enabled && !g.removed)
			result.push_back(kv.first);
	}

	// Mission system actor if set and enabled
	if (mission_system_.has_value() && mission_system_->enabled)
		result.push_back(mission_system_->actor_id);

	return result;
}

std::vector<ActorId> ActorStore::actors_in_area(const AreaId& area) const
{
	std::vector<ActorId> result;

	for (const auto& kv : heroes_)
	{
		if (kv.second.common.area_id == area)
			result.push_back(kv.first);
	}
	for (const auto& kv : allies_)
	{
		if (kv.second.common.area_id == area)
			result.push_back(kv.first);
	}
	for (const auto& kv : monsters_)
	{
		if (kv.second.common.area_id == area)
			result.push_back(kv.first);
	}

	return result;
}

std::vector<ActorId> ActorStore::targetable_actors_in_area(const AreaId& area) const
{
	std::vector<ActorId> result;

	for (const ActorId& id : actors_in_area(area))
	{
		const ActorStateCommon& c = common(id);
		if (c.can_be_targeted && c.life_state == ActorLifeState::ACTIVE)
			result.push_back(id);
	}

	return result;
}

// ── Collection accessors ──────────────────────────────────────────────────────

const std::unordered_map<ActorId, HeroState>&
ActorStore::heroes() const
{
	return heroes_;
}

const std::unordered_map<ActorId, AllyState>&
ActorStore::allies() const
{
	return allies_;
}

const std::unordered_map<MonsterInstanceId, MonsterInstanceState>&
ActorStore::monster_instances() const
{
	return monsters_;
}

const std::unordered_map<ActorId, MonsterGroupState>&
ActorStore::monster_groups() const
{
	return monster_groups_;
}

const std::unordered_map<ActorId, BossState>&
ActorStore::bosses() const
{
	return bosses_;
}

const std::optional<MissionSystemState>&
ActorStore::mission_system_opt() const
{
	return mission_system_;
}

} // namespace gmActor
