#ifndef GMDUNGEONBASIC_ACTIONV1_HPP
#define GMDUNGEONBASIC_ACTIONV1_HPP

/**
 * @file actions/ActionV1.hpp
 * @brief Executor for the three v1 gameplay actions: Move, Heal, Equip.
 *
 * ActionV1 is the single place in the engine that takes a validated action
 * decision and applies its effects to the @ref ActorRoster and @ref DungeonMap.
 * It uses @ref DungeonRuleAdapter to pre-validate the action; if the validation
 * fails it returns @c false and fills the rejection reason without touching the
 * game state.
 *
 * Actions outside the v1 scope (Attack, Defend) are intentionally absent.
 * They will be introduced in a future phase under a separate contract version.
 *
 * @note Not thread-safe.
 */

#include "actors/ActorRoster.hpp"
#include "rules/DungeonRuleAdapter.hpp"
#include "world/DungeonMap.hpp"

#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Executes the three v1 dungeon actions (Move, Heal, Equip).
 *
 * Each @c execute_*() method:
 *   -# Validates the action via DungeonRuleAdapter.
 *   -# If invalid, returns @c false (caller emits ACTION_REJECTED).
 *   -# If valid, mutates ActorRoster and/or DungeonMap.
 *   -# Returns @c true (caller emits the corresponding success event).
 */
class ActionV1
{
public:
	/**
	 * @brief Constructs the executor with references to the live game state.
	 *
	 * @param map     Reference to the dungeon map (must outlive this object).
	 * @param actors  Reference to the actor roster (must outlive this object).
	 * @param rules   Reference to the rule adapter (must outlive this object).
	 */
	ActionV1(DungeonMap& map, ActorRoster& actors, DungeonRuleAdapter& rules);

	// ── Move ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Moves a hero actor to an adjacent destination room.
	 *
	 * Corresponds to GRS rule Move_Hero (priority 100).
	 * Validates C_CanMove (hero exists AND destination valid AND adjacent).
	 * On success calls @c ActorRoster::move_to().
	 *
	 * @param hero_id      Actor to move.
	 * @param destination  Target room identifier.
	 * @return             @c true if the move was executed, @c false if rejected.
	 */
	bool execute_move(const std::string& hero_id, const std::string& destination);

	// ── Heal ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Uses a healing potion to restore HP to a target actor.
	 *
	 * Corresponds to GRS rules Heal_Self (priority 120) / Heal_Adjacent (125).
	 * Validates C_HeroCanHeal (hero has tag "has_potion").
	 * On success applies HEAL and REMOVE_TAG(has_potion) via ActorRoster.
	 *
	 * @param hero_id    Actor consuming the potion.
	 * @param target_id  Actor receiving the healing (may equal @p hero_id).
	 * @return           @c true if the heal was executed, @c false if rejected.
	 */
	bool execute_heal(const std::string& hero_id, const std::string& target_id);

	// ── Equip ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Equips a weapon for the hero.
	 *
	 * Corresponds to GRS rule Equip_BigSword (priority 80).
	 * Validates C_HasBigSword AND C_NoWeaponEquipped.
	 * On success calls @c ActorRoster::add_tag(hero_id, "equipped_weapon").
	 *
	 * @param hero_id   Actor equipping the weapon.
	 * @param item_tag  Item tag to activate (e.g. "bigword_available").
	 * @return          @c true if the equip was executed, @c false if rejected.
	 */
	bool execute_equip(const std::string& hero_id, const std::string& item_tag);

	/**
	 * @brief Returns the rejection reason from the last failed execute call.
	 *
	 * @return  Human-readable rejection string, or empty if last call succeeded.
	 */
	std::string last_rejection_reason() const;

private:
	DungeonMap&         _map;     ///< Non-owning reference to the dungeon map.
	ActorRoster&        _actors;  ///< Non-owning reference to the actor roster.
	DungeonRuleAdapter& _rules;   ///< Non-owning reference to the rule adapter.

	std::string _last_rejection;  ///< Filled when execute returns false.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_ACTIONV1_HPP
