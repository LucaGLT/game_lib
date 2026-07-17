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
#include <set>
#include <string>
#include <utility>
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

	/// Extra timeline cost from crossing closed zone-boundary doors during a MOVE
	/// (added on top of the action/card's base timeline cost).
	int extra_timeline_cost = 0;
	/// Zone-boundary doors newly opened by this MOVE (normalized pairs), so the
	/// caller can emit a GUI notification for each one.
	std::vector<std::pair<LocationId, LocationId>> opened_doors;
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
		const std::string&  target_faction);

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
		gmActor::ActorStore& store);

	/**
	 * @brief Moves a hero to dest_id if reachable within max_steps BFS steps.
	 *
	 * Used by card-driven MOVE effects (e.g. Passo Sicuro with amount=3).
	 * Unlike apply_simple_move, adjacency is not required — only BFS
	 * reachability within the given step budget.
	 *
	 * @param hero_id    Actor ID of the moving hero.
	 * @param dest_id    Target LocationId.
	 * @param max_steps  Maximum BFS distance allowed (>= 1).
	 * @param store      Actor store (modified in place).
	 * @param avoid_enemy_locations  If true, intermediate path locations occupied
	 *                   by `enemy_faction` are excluded from the search (the
	 *                   final destination is exempt). Used by Passo Cauto /
	 *                   Scatto Breve.
	 * @param enemy_faction  Faction to avoid crossing through, when
	 *                   `avoid_enemy_locations` is true.
	 * @return EffectResult (resolved=false if dest_id is not reachable).
	 */
	EffectResult apply_card_move(
		const HeroId&        hero_id,
		const LocationId&    dest_id,
		int                  max_steps,
		gmActor::ActorStore& store,
		bool                 avoid_enemy_locations = false,
		const std::string&   enemy_faction = std::string{});

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

	/**
	 * @brief Applies a fixed amount of damage to an explicit target.
	 *
	 * Public primitive used by the interactive attack / reaction window: the
	 * target is chosen by the caller (not auto-selected) and the amount is the
	 * already-reduced final damage.
	 *
	 * @param target_id Actor receiving the damage.
	 * @param amount    Final damage to apply (>= 0).
	 * @param store     Actor store (modified in place).
	 * @return `EffectResult` describing the outcome (target_ko on lethal hit).
	 */
	EffectResult deal_damage(
		const gmActor::ActorId& target_id,
		int                     amount,
		gmActor::ActorStore&    store) const;

	/**
	 * @brief Finds the nearest valid target without applying any effect.
	 *
	 * Convenience wrapper around the internal TargetingFilter, used by
	 * EldhomEngine::play_card to locate a DAMAGE target before parking the
	 * pending-attack struct.
	 *
	 * @param store     Actor store to query (read-only).
	 * @param from_loc  Location from which targeting is resolved.
	 * @param faction   Faction being targeted.
	 * @param range     Search radius in location-hops (0 = same location only,
	 *                  the default). Searches the nearest location (fewest
	 *                  hops) with at least one valid target, up to `range`.
	 * @return Actor ID of the nearest valid target, or empty string if none.
	 */
	gmActor::ActorId find_nearest_target(
		const gmActor::ActorStore& store,
		const LocationId&          from_loc,
		const std::string&         faction,
		int                        range = 0) const;

	/**
	 * @brief Checks whether an explicit, player-chosen target is reachable
	 * and targetable within a card's declared range.
	 *
	 * Used by EldhomEngine::play_card when the caller (GUI) supplies a
	 * specific `target_id` (e.g. the monster the player clicked) instead of
	 * letting the engine auto-select via find_nearest_target. Performs the
	 * same hop-by-hop BFS as find_nearest_target, but checks membership of
	 * `target_id` in each hop's valid-target set (respecting §15 Proiezione)
	 * instead of returning the first hit.
	 *
	 * @param store     Actor store to query (read-only).
	 * @param from_loc  Location from which targeting is resolved.
	 * @param target_id Actor the player chose.
	 * @param faction   Faction being targeted.
	 * @param range     Search radius in location-hops (0 = same location only).
	 * @return True if `target_id` is a valid target within `range` hops.
	 */
	bool is_valid_target_in_range(
		const gmActor::ActorStore& store,
		const LocationId&          from_loc,
		const gmActor::ActorId&    target_id,
		const std::string&         faction,
		int                        range = 0) const;

	TargetingFilter                                                _targeting;
	std::unordered_map<LocationId, std::vector<LocationId>>        _adjacency;
	/// Zone-boundary doors (LocationId pairs, always stored with `first < second`)
	/// that a PG has already crossed. Until a pair is in this set, it is a
	/// CLOSED_DOOR: PGs pay +1 extra timeline cost to cross it and monsters cannot cross it
	/// at all. Once opened it behaves exactly like a free passage for everyone.
	std::set<std::pair<LocationId, LocationId>>                    _opened_zone_doors;

	EffectResult apply_damage(
		const gmActor::ActorId& target_id,
		int                     amount,
		gmActor::ActorStore&    store) const;

	bool is_adjacent(const LocationId& from, const LocationId& to) const;

	/**
	 * @brief Returns the zone prefix of a LocationId (trailing digits stripped).
	 *
	 * Examples: "S1"->"S", "C2"->"C", "IN"->"IN". Mirrors the GUI's own
	 * heuristic (board_widget.py `_zone_from_loc_id`) so both sides always
	 * agree on which passages are zone-boundary doors.
	 *
	 * @param loc_id LocationId to classify.
	 * @return Zone prefix string.
	 */
	static std::string zone_of(const LocationId& loc_id);

	/**
	 * @brief True if *a* and *b* belong to different zones (a CLOSED_DOOR
	 * candidate), regardless of whether it has already been opened.
	 *
	 * @param a First LocationId.
	 * @param b Second LocationId.
	 */
	bool is_zone_boundary(const LocationId& a, const LocationId& b) const;

	/**
	 * @brief True if the zone-boundary door between *a* and *b* has already
	 * been opened by a PG (or if a and b are in the same zone, i.e. there was
	 * never a door to open).
	 *
	 * @param a First LocationId.
	 * @param b Second LocationId.
	 */
	bool is_zone_door_open(const LocationId& a, const LocationId& b) const;

	/**
	 * @brief Marks the zone-boundary door between *a* and *b* as open.
	 *
	 * Idempotent: calling it again on an already-open pair has no effect.
	 *
	 * @param a First LocationId.
	 * @param b Second LocationId.
	 */
	void open_zone_door(const LocationId& a, const LocationId& b);

	/**
	 * @brief Returns all zone-boundary doors opened so far (for GUI/state export).
	 */
	const std::set<std::pair<LocationId, LocationId>>& opened_zone_doors() const;

	/**
	 * @brief Adds a new adjacency edge at runtime (e.g. when a door is opened).
	 *
	 * Does nothing if the pair already exists.
	 *
	 * @param from  Origin location ID.
	 * @param to    Destination location ID.
	 */
	void add_adjacency(const LocationId& from, const LocationId& to);
};

} // namespace eldhom

#endif // ELDHOM_RULES_ELDHOMRULEADAPTER_HPP
