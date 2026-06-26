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
#include <unordered_map>

namespace gmDungeonBasic
{

/**
 * @brief Defender's reactive choice when a defense window is open.
 *
 * Exactly one of @c cancel / @c pass should be set; if both are false the
 * defender actively reduces the incoming damage by @c block plus any status /
 * resource bonuses resolved by @ref ActionV1::resolve_attack.
 */
struct DefenseChoice
{
	bool cancel = false;  ///< Fully negates the incoming attack (0 final damage).
	bool pass   = false;  ///< Declines to defend: full damage is applied.
	int  block  = 0;      ///< Reduction granted by the played defense card.
};

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

	// ── Attack / reactive defense ───────────────────────────────────────

	/**
	 * @brief Validates an attack and computes its declared (pre-defense) damage.
	 *
	 * Does not mutate game state: the damage is applied later by
	 * @ref resolve_attack once the defender has responded. The declared damage is
	 * @c max(0, attacker_attack_stat + card_damage_mod).
	 *
	 * @param attacker_id      Actor declaring the attack.
	 * @param target_id        Actor being attacked.
	 * @param card_damage_mod  Extra damage contributed by the attack card (0 for a
	 *                         base attack).
	 * @param out_base_damage  Set to the declared damage on success.
	 * @return                 @c true if the attack is valid, @c false if rejected.
	 */
	bool declare_attack(const std::string& attacker_id,
	                    const std::string& target_id,
	                    int card_damage_mod,
	                    int& out_base_damage);

	/**
	 * @brief Resolves a declared attack against the defender's reactive choice.
	 *
	 * Computes the final damage, consumes the defender's spent defenses (the
	 * @c difeso status and one @c scudo_equipaggiato charge when actively
	 * defending) and applies the damage to the target. The final damage is
	 * @c 0 if the attack was cancelled, otherwise
	 * @c max(0, base_damage - reduction).
	 *
	 * @param target_id     Defender receiving the attack.
	 * @param base_damage   Declared damage from @ref declare_attack.
	 * @param defense       Defender's reactive choice.
	 * @param out_hp_after  Set to the defender's HP after the damage is applied.
	 * @return              The final damage actually dealt.
	 */
	int resolve_attack(const std::string& target_id,
	                   int base_damage,
	                   const DefenseChoice& defense,
	                   int& out_hp_after);

	/// @brief Resets transient combat state (shield charges). Call on new game.
	void reset_combat();

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

	/// @brief Remaining shield charges per actor (lazily seeded from the tag).
	std::unordered_map<std::string, int> _shield_charges;
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_ACTIONV1_HPP
