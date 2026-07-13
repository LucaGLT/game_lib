#ifndef ELDHOM_FORMATION_ELDHOMFORMATIONADAPTER_HPP
#define ELDHOM_FORMATION_ELDHOMFORMATIONADAPTER_HPP

/**
 * @file formation/EldhomFormationAdapter.hpp
 * @brief Thin adapter that applies gmActor formation validation to Eldhom.
 *
 * `EldhomFormationAdapter` wraps `gmActor::FormationValidator` and exposes
 * an Eldhom-centric API:
 * - accepts Eldhom location IDs and actor stores
 * - returns `FormationCheckResult` with human-readable context
 * - internally builds the (frontline_count, backline_count) pair from the
 *   current actor states per location and faction.
 *
 * ### Eldhom formation rules (§41)
 *
 * For each faction × location pair:
 * - Schieramento: backline_count ≤ frontline_count  (PL ≤ RG rule, inversed for monsters)
 * - Scompaginamento: frontline_count == 0 AND backline_count > 0
 *   → all backline actors forced to frontline
 * - Proiezione: PL actors screen RG from being directly targeted (§15)
 */

#include "gmActor/formation/FormationValidator.hpp"
#include "gmActor/formation/FormationRules.hpp"
#include "gmActor/actors/ActorStore.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include <string>
#include <vector>

namespace eldhom {

/**
 * @struct FormationCheckResult
 * @brief Outcome of an Eldhom formation validity check.
 */
struct FormationCheckResult {
	bool        valid          = true;
	bool        scompaginamento = false; ///< Frontline empty, backline actors forced forward
	int         overflow       = 0;     ///< Number of backline actors exceeding the limit
	std::string faction_id;
	LocationId  location_id;
	std::string message;
};

/**
 * @class EldhomFormationAdapter
 * @brief Bridges gmActor::FormationValidator with Eldhom formation semantics.
 */
class EldhomFormationAdapter
{
public:
	/**
	 * @brief Constructs the adapter with default formation rules
	 *        (backline_count ≤ frontline_count).
	 */
	EldhomFormationAdapter();

	// ── Validation ────────────────────────────────────────────────────────────

	/**
	 * @brief Validates the formation of one faction in one location.
	 *
	 * Counts the frontline and backline live actors of `faction_id` in
	 * `location_id`, then delegates to `FormationValidator::is_valid`.
	 *
	 * @param store       The actor store to query.
	 * @param location_id The location to inspect.
	 * @param faction_id  Faction whose formation is being checked.
	 * @return `FormationCheckResult` with validity status and overflow count.
	 */
	FormationCheckResult check(
		const gmActor::ActorStore& store,
		const LocationId&          location_id,
		const std::string&         faction_id) const;

	/**
	 * @brief Resolves a Scompaginamento: moves all backline actors to frontline.
	 *
	 * Should be called when `check()` returns `scompaginamento == true`.
	 * Modifies `ActorStateCommon::area_position` for all backline instances
	 * in `location_id` of `faction_id`.
	 *
	 * @param store       The actor store to modify.
	 * @param location_id The affected location.
	 * @param faction_id  Faction to resolve.
	 */
	void resolve_scompaginamento(
		gmActor::ActorStore& store,
		const LocationId&    location_id,
		const std::string&   faction_id) const;

private:
	gmActor::FormationValidator _validator; ///< Stateless delegate
};

} // namespace eldhom

#endif // ELDHOM_FORMATION_ELDHOMFORMATIONADAPTER_HPP
