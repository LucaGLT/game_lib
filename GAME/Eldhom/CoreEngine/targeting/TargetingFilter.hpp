#ifndef ELDHOM_TARGETING_TARGETINGFILTER_HPP
#define ELDHOM_TARGETING_TARGETINGFILTER_HPP

/**
 * @file targeting/TargetingFilter.hpp
 * @brief Resolves valid attack targets according to Eldhom formation rules.
 *
 * ### Eldhom targeting rules (§15 — Proiezione)
 *
 * When an attacker selects a target in a location:
 * 1. If any FRONTLINE actor of the target faction is alive in that location,
 *    only FRONTLINE actors may be targeted.
 * 2. If no FRONTLINE actor exists (Scompaginamento already resolved), any
 *    actor in that location may be targeted.
 *
 * In Eldhom the "target faction" for PG attacks is the monster faction
 * (e.g. "BRIGANTI"), and the "target faction" for monster attacks is "HEROES".
 *
 * `TargetingFilter` is stateless.  It uses only the `ActorStore` snapshot
 * and the caller-supplied location to determine valid targets.
 */

#include "gmActor/actors/ActorStore.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include <string>
#include <vector>

namespace eldhom {

/**
 * @class TargetingFilter
 * @brief Computes the list of valid targets for an attack in one location.
 */
class TargetingFilter
{
public:
	// ── Target resolution ─────────────────────────────────────────────────────

	/**
	 * @brief Returns IDs of all valid targets for an attack in `location_id`.
	 *
	 * Applies §15 Proiezione: if any FRONTLINE actor of `target_faction`
	 * is present and alive, only FRONTLINE actors are returned.
	 *
	 * @param store          The actor store to query.
	 * @param location_id    The location where the attack originates.
	 * @param target_faction The faction being targeted (e.g. "BRIGANTI").
	 * @return Ordered vector of actor IDs.  Heroes come before monster
	 *         instances.  Empty if no valid targets exist.
	 */
	std::vector<gmActor::ActorId> valid_targets(
		const gmActor::ActorStore& store,
		const LocationId&          location_id,
		const std::string&         target_faction) const;

	/**
	 * @brief Returns the nearest valid target for an attack in `location_id`.
	 *
	 * "Nearest" in Eldhom means "same location first".  If the attacker
	 * is in the same location, that is always nearest.  Among multiple
	 * valid targets in the same location, prefers FRONTLINE over BACKLINE
	 * then lowest HP.
	 *
	 * @param store          The actor store to query.
	 * @param location_id    The location where targets are searched.
	 * @param target_faction The faction being targeted.
	 * @return Actor ID of the nearest valid target, or empty string if none.
	 */
	gmActor::ActorId nearest_target(
		const gmActor::ActorStore& store,
		const LocationId&          location_id,
		const std::string&         target_faction) const;

	/**
	 * @brief Returns true if the attacker's location has at least one valid target
	 *        of `target_faction`.
	 *
	 * Convenience overload used by `EldhomEngine` to avoid computing the
	 * full target list when only existence is needed.
	 *
	 * @param store          The actor store.
	 * @param location_id    Location to check.
	 * @param target_faction Faction to look for.
	 */
	bool has_valid_target(
		const gmActor::ActorStore& store,
		const LocationId&          location_id,
		const std::string&         target_faction) const;
};

} // namespace eldhom

#endif // ELDHOM_TARGETING_TARGETINGFILTER_HPP
