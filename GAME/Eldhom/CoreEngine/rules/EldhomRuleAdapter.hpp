#ifndef ELDHOM_RULES_ELDHOMRULEADAPTER_HPP
#define ELDHOM_RULES_ELDHOMRULEADAPTER_HPP

/**
 * @file rules/EldhomRuleAdapter.hpp
 * @brief Applies card and step effects to the Eldhom actor store.
 *
 * `EldhomRuleAdapter` is the bridge between card data (`EldhomEffect`,
 * `gmActor::BehaviorStep`) and mutable actor state (`gmActor::ActorStore`).
 *
 * It interprets the `effect_type` string and delegates to the correct
 * actor-store mutation.  It also exposes an `EffectResult` so the caller
 * can emit appropriate events.
 *
 * ### Supported effect types (EldhomEffect / BehaviorStep)
 *
 * | effect_type             | Actor types affected        | Params used        |
 * |-------------------------|-----------------------------|--------------------|
 * | `"DAMAGE"`              | Any targetable actor        | amount, target     |
 * | `"HEAL"`                | Hero / Ally                 | amount, target     |
 * | `"MOVE"`                | Hero                        | amount, value=loc  |
 * | `"MOVE_TOWARD_PG"`      | Monster instance            | amount             |
 * | `"DEAL_DAMAGE"`         | Alias for DAMAGE            | amount, target     |
 * | `"FORMATION_PUSH"`      | Hero / monster instance     | value=FRONT/BACK   |
 * | `"WAIT"`                | —                           | —                  |
 *
 * Unknown effect types are silently ignored (open-closed: add new types
 * without touching existing callers).
 *
 * ### Position convention
 *
 * area_id on `ActorStateCommon` stores the `LocationId` string.
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/behavior/BehaviorStep.hpp"
#include "GAME/Eldhom/CoreEngine/engine/CardData.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/targeting/TargetingFilter.hpp"

#include <functional>
#include <string>
#include <vector>

namespace eldhom {

/**
 * @struct EffectResult
 * @brief Return value from an effect application.
 */
struct EffectResult {
	bool               resolved = false;   ///< True if the effect was applied
	gmActor::ActorId   target_id;          ///< ID of the actor affected (empty if none)
	int                damage_dealt = 0;   ///< HP removed (positive)
	int                hp_restored  = 0;   ///< HP restored (positive)
	bool               target_ko    = false; ///< True if the target reached 0 HP
	std::string        note;               ///< Debug / narrative description
};

/**
 * @class EldhomRuleAdapter
 * @brief Applies EldhomEffect and BehaviorStep entries to actor state.
 */
class EldhomRuleAdapter
{
public:
	/**
	 * @brief Constructs the adapter.
	 *
	 * @param targeting  TargetingFilter used for target resolution (copied by value;
	 *                   TargetingFilter is stateless so the copy is trivial).
	 * @param location_adjacency  Map from LocationId to adjacent LocationId list.
	 *                            Used by MOVE effects to validate destinations.
	 */
	EldhomRuleAdapter(
		TargetingFilter                                              targeting,
		const std::unordered_map<LocationId, std::vector<LocationId>>& location_adjacency);

	// ── Hero card effects ─────────────────────────────────────────────────────

	/**
	 * @brief Applies one `EldhomEffect` from a hero action card.
	 *
	 * @param effect     The effect to apply.
	 * @param actor_id   The acting hero's actor ID.
	 * @param store      Actor store (modified in place).
	 * @param faction_id Target faction (e.g. "BRIGANTI" when a hero attacks).
	 * @return `EffectResult` describing what happened.
	 */
	EffectResult apply_effect(
		const EldhomEffect& effect,
		const HeroId&       actor_id,
		gmActor::ActorStore& store,
		const std::string&  target_faction) const;

	// ── Monster behavior step effects ─────────────────────────────────────────

	/**
	 * @brief Applies one `gmActor::BehaviorStep` from a behavior card.
	 *
	 * Used as the inner body of the `StepExecutor` lambda passed to
	 * `BehaviorCardProcessor`.
	 *
	 * @param step        The behavior step to execute.
	 * @param group_id    The acting monster group's actor ID.
	 * @param member_id   The specific monster instance executing the step.
	 * @param store       Actor store (modified in place).
	 * @param hero_faction Target hero faction (e.g. "HEROES").
	 * @return `EffectResult` describing what happened.
	 */
	EffectResult apply_behavior_step(
		const gmActor::BehaviorStep& step,
		const GroupId&               group_id,
		const gmActor::ActorId&      member_id,
		gmActor::ActorStore&         store,
		const std::string&           hero_faction) const;

	// ── Simple action effects ─────────────────────────────────────────────────

	/**
	 * @brief Applies a Movimento Semplice for a hero.
	 *
	 * @param hero_id     Actor ID of the moving hero.
	 * @param dest_id     Target LocationId (must be adjacent to current).
	 * @param store       Actor store (modified in place).
	 * @return `EffectResult` (resolved=false if dest_id is not adjacent).
	 */
	EffectResult apply_simple_move(
		const HeroId&        hero_id,
		const LocationId&    dest_id,
		gmActor::ActorStore& store) const;

	/**
	 * @brief Applies an Attacco Semplice for a hero (1 damage, nearest target).
	 *
	 * @param hero_id        Actor ID of the attacking hero.
	 * @param store          Actor store (modified in place).
	 * @param target_faction Faction being attacked.
	 * @return `EffectResult`.
	 */
	EffectResult apply_simple_attack(
		const HeroId&        hero_id,
		gmActor::ActorStore& store,
		const std::string&   target_faction) const;

	/**
	 * @brief Applies a Recupero Semplice: restores 1 HP to the hero.
	 *
	 * @param hero_id  Actor ID of the recovering hero.
	 * @param store    Actor store (modified in place).
	 * @return `EffectResult`.
	 */
	EffectResult apply_simple_recover(
		const HeroId&        hero_id,
		gmActor::ActorStore& store) const;

private:
	TargetingFilter                                                _targeting;
	std::unordered_map<LocationId, std::vector<LocationId>>        _adjacency;

	EffectResult apply_damage(
		const gmActor::ActorId& target_id,
		int                     amount,
		gmActor::ActorStore&    store) const;

	bool is_adjacent(const LocationId& from, const LocationId& to) const;
};

} // namespace eldhom

#endif // ELDHOM_RULES_ELDHOMRULEADAPTER_HPP
