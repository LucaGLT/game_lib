#ifndef GMACTOR_FORMATION_FORMATIONRULES_HPP
#define GMACTOR_FORMATION_FORMATIONRULES_HPP

/**
 * @file formation/FormationRules.hpp
 * @brief Configurable policy for front-line / back-line formation constraints.
 *
 * `FormationRules` is a plain data struct that encodes the game-specific
 * constraints of a front-rank / back-rank formation.  It contains no
 * domain vocabulary; all thresholds are integers or booleans.
 *
 * ### Typical usage
 *
 * Most tactical games use the default configuration, which enforces
 * `backline_count <= frontline_count` and does not cap absolute counts:
 *
 * @code
 *   // Default rules: backline ≤ frontline, no absolute cap.
 *   FormationRules rules;
 *
 *   // Custom rules: backline ≤ 2 × frontline, max 4 frontliners.
 *   FormationRules custom;
 *   custom.max_backline_per_frontline = 2;
 *   custom.max_frontline = 4;
 * @endcode
 */

namespace gmActor {

/**
 * @struct FormationRules
 * @brief Formation policy POD used by FormationValidator and FormationResolver.
 *
 * ## Field semantics
 *
 * | Field                        | Meaning                                               |
 * |------------------------------|-------------------------------------------------------|
 * | `max_backline_per_frontline` | Maximum backline actors per one frontline actor.      |
 * |                              | `1` means backline ≤ frontline (most common setting). |
 * | `max_frontline`              | Hard cap on frontline actors.  `-1` = no cap.        |
 * | `max_backline`               | Hard cap on backline actors.   `-1` = no cap.        |
 * | `backline_requires_frontline`| When `true`, backline > 0 is only legal if           |
 * |                              | frontline > 0.  When `false`, a pure backline         |
 * |                              | formation is allowed.                                 |
 */
struct FormationRules
{
	/// Max backline actors per frontline actor (≥ 1).  Default: 1 (backline ≤ frontline).
	int max_backline_per_frontline = 1;

	/// Hard cap on frontline count.  -1 = unlimited.
	int max_frontline = -1;

	/// Hard cap on backline count.  -1 = unlimited.
	int max_backline = -1;

	/// When true, backline actors may only exist if frontline > 0.
	bool backline_requires_frontline = true;
};

} // namespace gmActor

#endif // GMACTOR_FORMATION_FORMATIONRULES_HPP
