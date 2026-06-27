#ifndef GMACTOR_FORMATION_FORMATIONRESOLVER_HPP
#define GMACTOR_FORMATION_FORMATIONRESOLVER_HPP

/**
 * @file formation/FormationResolver.hpp
 * @brief Sorts backline actors into movement priority order using pluggable criteria.
 *
 * When a `FormationValidator` reports that a formation is over the legal
 * backline limit, the game needs to decide **which** backline actors must
 * move (or be removed).  `FormationResolver` answers that question.
 *
 * ### Sorting algorithm
 *
 * The resolver sorts candidates using a **lexicographic chain** of
 * `FormationCriterion` comparators:
 *
 * 1. Compare actors with criterion[0].
 * 2. If criterion[0] returns the same preference for both actors (i.e. both
 *    `compare(a,b)` and `compare(b,a)` return `false`), fall through to
 *    criterion[1], and so on.
 * 3. The first criterion that strictly prefers one actor determines the order.
 * 4. If all criteria are exhausted and the pair is still tied, their relative
 *    order is unspecified but stable.
 *
 * The caller receives a vector sorted from **most preferred** (first candidate
 * to move) to least preferred.
 *
 * ### Usage example
 *
 * @code
 *   #include "gmActor/formation/FormationCriteria.hpp"
 *   #include "gmActor/formation/FormationResolver.hpp"
 *
 *   gmActor::FormationResolver resolver({
 *       gmActor::FormationCriteria::by_highest_hp(),
 *       gmActor::FormationCriteria::by_lowest_timeline(),
 *       gmActor::FormationCriteria::random(42)
 *   });
 *
 *   std::vector<gmActor::ActorId> to_move =
 *       resolver.sort_candidates(backline_ids, store);
 *
 *   // Move the first `overflow` entries to the frontline.
 *   int overflow = validator.backline_overflow(fl_count, bl_count);
 *   for (int i = 0; i < overflow; ++i)
 *       move_to_frontline(to_move[i], store);
 * @endcode
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/core/Ids.hpp"
#include "gmActor/formation/FormationCriteria.hpp"

#include <vector>

namespace gmActor {

/**
 * @class FormationResolver
 * @brief Ranks backline actors by movement priority using a criteria chain.
 *
 * The resolver owns an ordered list of `FormationCriterion` comparators.
 * Criteria are evaluated in registration order; the first one that
 * discriminates between two actors determines their relative position.
 */
class FormationResolver
{
public:
	// ── Constructor ───────────────────────────────────────────────────────────

	/**
	 * @brief Constructs the resolver with the given priority-ordered criteria.
	 *
	 * @param criteria  Non-empty list of comparators.  Order matters: the
	 *                  first criterion has the highest priority.
	 *                  May be empty (all candidates are treated as equal and
	 *                  returned in their original order).
	 */
	explicit FormationResolver(std::vector<FormationCriterion> criteria);

	// ── Sorting ───────────────────────────────────────────────────────────────

	/**
	 * @brief Returns `backline_actors` sorted from most to least preferred for movement.
	 *
	 * "Most preferred" means the actor that should be moved to the frontline
	 * first (or removed from the backline first if the game does not allow
	 * frontline moves).
	 *
	 * The input vector is not modified.  The returned vector is a permutation
	 * of the input actors.
	 *
	 * @param backline_actors  Unordered list of backline actor IDs to rank.
	 * @param store            Read-only actor store used by the criteria
	 *                         functions to access actor state.
	 * @return Actors sorted by movement priority (index 0 = first to move).
	 */
	std::vector<ActorId> sort_candidates(const std::vector<ActorId>& backline_actors,
	                                     const ActorStore&            store) const;

	// ── Accessors ─────────────────────────────────────────────────────────────

	/**
	 * @brief Returns the number of criteria registered in this resolver.
	 */
	int criteria_count() const;

private:
	std::vector<FormationCriterion> _criteria;

	/// Returns true if actor `a` should come before actor `b`.
	bool compare(const ActorId& a, const ActorId& b,
	             const ActorStore& store) const;
};

} // namespace gmActor

#endif // GMACTOR_FORMATION_FORMATIONRESOLVER_HPP
