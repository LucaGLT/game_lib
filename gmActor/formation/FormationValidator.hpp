#ifndef GMACTOR_FORMATION_FORMATIONVALIDATOR_HPP
#define GMACTOR_FORMATION_FORMATIONVALIDATOR_HPP

/**
 * @file formation/FormationValidator.hpp
 * @brief Stateless validator for front-line / back-line formation constraints.
 *
 * `FormationValidator` tests whether a given pair of actor counts satisfies
 * the rules encoded in a `FormationRules` policy object.  It has no
 * dependency on `ActorStore` or any domain type: all it needs are two
 * integers — the number of frontline actors and the number of backline actors.
 *
 * ### Usage example
 *
 * @code
 *   gmActor::FormationRules rules;  // default: backline ≤ frontline
 *   gmActor::FormationValidator v(rules);
 *
 *   v.is_valid(2, 1);   // true  (1 ≤ 2)
 *   v.is_valid(1, 2);   // false (2 > 1)
 *
 *   v.backline_overflow(1, 3);  // 2  (3 - 1 * 1 = 2 extra)
 * @endcode
 */

#include "gmActor/formation/FormationRules.hpp"

namespace gmActor {

/**
 * @class FormationValidator
 * @brief Tests formation legality given a `FormationRules` policy.
 */
class FormationValidator
{
public:
	// ── Constructor ───────────────────────────────────────────────────────────

	/**
	 * @brief Constructs a validator with the given rules.
	 *
	 * @param rules  Policy to use for all subsequent checks.
	 *               Defaults to the standard (backline ≤ frontline) policy.
	 */
	explicit FormationValidator(FormationRules rules = {});

	// ── Validation ────────────────────────────────────────────────────────────

	/**
	 * @brief Returns true if the formation is legal under the configured policy.
	 *
	 * All of the following conditions must hold simultaneously:
	 *
	 * 1. `frontline_count >= 0` and `backline_count >= 0`.
	 * 2. If `backline_requires_frontline == true`, then `frontline_count > 0`
	 *    whenever `backline_count > 0`.
	 * 3. `backline_count <= frontline_count × max_backline_per_frontline`.
	 *    (When `frontline_count == 0` and `backline_requires_frontline == false`,
	 *    condition 3 is relaxed: backline is only constrained by `max_backline`.)
	 * 4. If `max_frontline >= 0`: `frontline_count <= max_frontline`.
	 * 5. If `max_backline  >= 0`: `backline_count  <= max_backline`.
	 *
	 * @param frontline_count  Number of actors in the front rank (≥ 0).
	 * @param backline_count   Number of actors in the back rank  (≥ 0).
	 * @return `true` if the formation satisfies all policy constraints.
	 */
	bool is_valid(int frontline_count, int backline_count) const;

	/**
	 * @brief Returns the number of backline actors that exceed the legal limit.
	 *
	 * A return value of 0 means the formation is legal (no overflow).
	 * A positive value N means N backline actors must move to the frontline
	 * (or be removed) to restore a valid formation.
	 *
	 * When `frontline_count == 0` and `backline_requires_frontline == true`,
	 * all backline actors are in overflow.
	 *
	 * @param frontline_count  Number of actors in the front rank (≥ 0).
	 * @param backline_count   Number of actors in the back rank  (≥ 0).
	 * @return Number of excess backline actors (≥ 0).
	 */
	int backline_overflow(int frontline_count, int backline_count) const;

	// ── Accessors ─────────────────────────────────────────────────────────────

	/**
	 * @brief Returns the policy this validator was constructed with.
	 */
	const FormationRules& rules() const;

private:
	FormationRules _rules;

	/// Returns the maximum legal backline given the current frontline count.
	int max_legal_backline(int frontline_count) const;
};

} // namespace gmActor

#endif // GMACTOR_FORMATION_FORMATIONVALIDATOR_HPP
