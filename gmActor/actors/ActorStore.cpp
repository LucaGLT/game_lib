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
    // TODO Phase 4: duplicate check + insert into heroes_ + kind_registry_.
    (void)hero;
}

void ActorStore::add_ally(AllyState ally)
{
    // TODO Phase 4
    (void)ally;
}

void ActorStore::add_monster_instance(MonsterInstanceState monster)
{
    // TODO Phase 4
    (void)monster;
}

void ActorStore::add_monster_group(MonsterGroupState group)
{
    // TODO Phase 4
    (void)group;
}

void ActorStore::add_boss(BossState boss)
{
    // TODO Phase 4
    (void)boss;
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
    // TODO Phase 4: route to hero/ally/monster and return reference.
    // Throw InvalidActorKindError for MONSTER_GROUP.
    (void)id;
    static ActorStateCommon dummy;
    return dummy;
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
    // TODO Phase 4: implement full collection.
    return {};
}

std::vector<ActorId> ActorStore::actors_in_area(const AreaId& area) const
{
    // TODO Phase 4
    (void)area;
    return {};
}

std::vector<ActorId> ActorStore::targetable_actors_in_area(const AreaId& area) const
{
    // TODO Phase 4
    (void)area;
    return {};
}

} // namespace gmActor
