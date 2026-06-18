#ifndef GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP
#define GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP

/**
 * @file rules/DungeonRuleAdapter.hpp
 * @brief Bridge between the dungeon game state and the gmRules rule engine.
 *
 * DungeonRuleAdapter implements @c gmRules::RuleContext for the dungeon domain.
 * It exposes the current map and actor state to gmRules so that
 * ConditionSpec / EffectSpec evaluations defined in the GRS specifications can
 * query and mutate the game world through the standard RuleContext interface.
 *
 * Additionally, it provides direct convenience query methods (can_move, can_heal,
 * can_equip) used by @ref ActionV1 to validate actions before execution.
 *
 * @note All methods that mutate state must be called from the engine thread.
 */

#include "actors/ActorRoster.hpp"
#include "engine/DungeonTypes.hpp"
#include "world/DungeonMap.hpp"

#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Rule context and convenience validator for v1 dungeon actions.
 *
 * Holds non-owning references to the DungeonMap and ActorRoster so that rule
 * conditions can query game state without owning it. The engine constructs this
 * adapter after loading the map and populating the roster, then passes it to
 * ActionV1 for action validation.
 */
class DungeonRuleAdapter
{
public:
	/**
	 * @brief Constructs the adapter with references to the live game state.
	 *
	 * @param map     Reference to the dungeon map (must outlive this adapter).
	 * @param actors  Reference to the actor roster (must outlive this adapter).
	 */
	DungeonRuleAdapter(DungeonMap& map, ActorRoster& actors);

	// ── v1 action condition checks ────────────────────────────────────────────

	/**
	 * @brief Checks whether the hero can legally move to a destination room.
	 *
	 * Evaluates conditions: C_HeroExists AND C_DestinationValid AND the
	 * destination must be adjacent to the hero's current location. Stunned heroes
	 * are blocked by trigger T_BlockIfStunned.
	 *
	 * @param hero_id      Hero actor identifier.
	 * @param destination  Target room identifier.
	 * @return             @c true if the move is allowed by the current rule set.
	 */
	bool can_move(const std::string& hero_id, const std::string& destination) const;

	/**
	 * @brief Checks whether the hero can use a healing potion.
	 *
	 * Evaluates condition C_HeroCanHeal: ACTOR_HAS_TAG(hero_id, has_potion).
	 *
	 * @param hero_id    Hero actor identifier.
	 * @param target_id  Target actor identifier (may equal hero_id for self-heal).
	 * @return           @c true if the heal action is currently valid.
	 */
	bool can_heal(const std::string& hero_id, const std::string& target_id) const;

	/**
	 * @brief Checks whether the hero can equip a weapon.
	 *
	 * Evaluates conditions C_HasBigSword AND C_NoWeaponEquipped.
	 *
	 * @param hero_id   Hero actor identifier.
	 * @param item_tag  Item tag to equip (e.g. "bigword_available").
	 * @return          @c true if the equip action is currently valid.
	 */
	bool can_equip(const std::string& hero_id, const std::string& item_tag) const;

	// ── Rejection reason ─────────────────────────────────────────────────────

	/**
	 * @brief Returns a human-readable reason why the last condition check failed.
	 *
	 * Valid only immediately after a @c can_*() call that returned @c false.
	 *
	 * @return  Rejection reason string (used in ACTION_REJECTED event).
	 */
	std::string rejection_reason() const;

private:
	DungeonMap&  _map;     ///< Non-owning reference to the dungeon map.
	ActorRoster& _actors;  ///< Non-owning reference to the actor roster.

	mutable std::string _rejection_reason;  ///< Set by failed can_*() calls.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP
