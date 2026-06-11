#ifndef GMACTOR_ACTORS_ACTORSTORE_HPP
#define GMACTOR_ACTORS_ACTORSTORE_HPP

/**
 * @file actors/ActorStore.hpp
 * @brief Central registry for all mutable actor states in one game session.
 *
 * `ActorStore` is the single source of truth for actor-related game state.
 * All actor states are owned by the store and accessed by ID.
 *
 * ## Actor type routing
 *
 * | Method group          | Applicable kinds                           |
 * |-----------------------|--------------------------------------------|
 * | `common(id)`          | HERO, ALLY_NPC, MONSTER_INSTANCE           |
 * | `hero(id)`            | HERO                                       |
 * | `ally(id)`            | ALLY_NPC                                   |
 * | `monster_instance(id)`| MONSTER_INSTANCE                           |
 * | `monster_group(id)`   | MONSTER_GROUP                              |
 * | `boss(id)`            | BOSS (keyed by controller_group_id)        |
 * | `mission_system()`    | MISSION_SYSTEM (at most one)               |
 *
 * Calling `common(group_id)` where the ID belongs to a `MonsterGroupState`
 * throws `InvalidActorKindError` (design decision D1).
 */

#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/AllyState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/actors/BossState.hpp"
#include "gmActor/actors/MissionSystemState.hpp"
#include "gmActor/core/Errors.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gmActor {

/**
 * @brief Central, owning registry for all actor states.
 */
class ActorStore {
public:
    // ── Registration ──────────────────────────────────────────────────────────

    /**
     * @brief Registers a hero in the store.
     * @throws DuplicateActorError if `hero.common.actor_id` is already registered.
     */
    void add_hero(HeroState hero);

    /**
     * @brief Registers an ally NPC in the store.
     * @throws DuplicateActorError if `ally.common.actor_id` is already registered.
     */
    void add_ally(AllyState ally);

    /**
     * @brief Registers a monster instance in the store.
     * @throws DuplicateActorError if `monster.common.actor_id` is already registered.
     */
    void add_monster_instance(MonsterInstanceState monster);

    /**
     * @brief Registers a monster group in the store.
     * @throws DuplicateActorError if `group.actor_id` is already registered.
     */
    void add_monster_group(MonsterGroupState group);

    /**
     * @brief Registers boss extension data in the store.
     *
     * Keyed by `boss.controller_group_id`.
     * @throws DuplicateActorError if the key is already registered.
     */
    void add_boss(BossState boss);

    /**
     * @brief Sets (or replaces) the mission system actor.
     */
    void set_mission_system(MissionSystemState system);

    // ── Existence / kind queries ───────────────────────────────────────────────

    /**
     * @brief Returns true if any actor with the given ID is registered.
     *
     * @param id Actor ID to look up.
     */
    bool has_actor(const ActorId& id) const;

    /**
     * @brief Returns the ActorKind for the given ID.
     *
     * @param id Actor ID.
     * @throws UnknownActorError if not found.
     */
    ActorKind actor_kind(const ActorId& id) const;

    // ── Common state accessors ────────────────────────────────────────────────

    /**
     * @brief Returns a mutable reference to the common state for the actor.
     *
     * Valid for HERO, ALLY_NPC, and MONSTER_INSTANCE.
     *
     * @throws InvalidActorKindError for MONSTER_GROUP (see D1).
     * @throws UnknownActorError if not found.
     */
    ActorStateCommon& common(const ActorId& id);

    /** @brief Const overload of common(). */
    const ActorStateCommon& common(const ActorId& id) const;

    // ── Typed accessors ───────────────────────────────────────────────────────

    /**
     * @brief Returns a mutable reference to the HeroState.
     * @throws InvalidActorKindError if the actor is not a hero.
     * @throws UnknownActorError if not found.
     */
    HeroState& hero(const ActorId& id);
    const HeroState& hero(const ActorId& id) const; ///< Const overload

    /**
     * @brief Returns a mutable reference to the AllyState.
     * @throws InvalidActorKindError if not an ally.
     * @throws UnknownActorError if not found.
     */
    AllyState& ally(const ActorId& id);
    const AllyState& ally(const ActorId& id) const; ///< Const overload

    /**
     * @brief Returns a mutable reference to the MonsterInstanceState.
     * @throws InvalidActorKindError if not a monster instance.
     * @throws UnknownActorError if not found.
     */
    MonsterInstanceState& monster_instance(const MonsterInstanceId& id);
    const MonsterInstanceState& monster_instance(const MonsterInstanceId& id) const; ///< Const overload

    /**
     * @brief Returns a mutable reference to the MonsterGroupState.
     * @throws InvalidActorKindError if not a monster group.
     * @throws UnknownActorError if not found.
     */
    MonsterGroupState& monster_group(const ActorId& id);
    const MonsterGroupState& monster_group(const ActorId& id) const; ///< Const overload

    /**
     * @brief Returns a mutable reference to the BossState keyed by controller_group_id.
     * @throws UnknownActorError if not found.
     */
    BossState& boss(const ActorId& controller_group_id);
    const BossState& boss(const ActorId& controller_group_id) const; ///< Const overload

    /**
     * @brief Returns a mutable reference to the mission system actor.
     * @throws UnknownActorError if no mission system has been set.
     */
    MissionSystemState& mission_system();
    const MissionSystemState& mission_system() const; ///< Const overload

    // ── Bulk queries ──────────────────────────────────────────────────────────

    /**
     * @brief Returns IDs of all actors that participate on the timeline.
     *
     * Includes:
     * - Heroes
     * - Allies where `common.can_act == true`
     * - Monster groups where `enabled == true`
     * - Mission system if `enabled == true`
     *
     * Does not include monster instances (they are bodies, not timeline actors).
     */
    std::vector<ActorId> timeline_actor_ids() const;

    /**
     * @brief Returns IDs of all actors whose `area_id` matches the given area.
     *
     * Searches heroes, allies, and monster instances.
     * Monster groups are not included (they have no `area_id`).
     *
     * @param area Target area ID.
     */
    std::vector<ActorId> actors_in_area(const AreaId& area) const;

    /**
     * @brief Returns IDs of all targetable actors in the given area.
     *
     * Subset of `actors_in_area()` filtered by `common.can_be_targeted == true`
     * and `common.life_state == ACTIVE`.
     *
     * @param area Target area ID.
     */
    std::vector<ActorId> targetable_actors_in_area(const AreaId& area) const;

private:
    std::unordered_map<ActorId, HeroState>             heroes_;
    std::unordered_map<ActorId, AllyState>             allies_;
    std::unordered_map<MonsterInstanceId, MonsterInstanceState> monsters_;
    std::unordered_map<ActorId, MonsterGroupState>     monster_groups_;
    std::unordered_map<ActorId, BossState>             bosses_;
    std::optional<MissionSystemState>                  mission_system_;

    // Flat registry: actor_id → ActorKind (for has_actor / actor_kind lookups)
    std::unordered_map<ActorId, ActorKind> kind_registry_;
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_ACTORSTORE_HPP
