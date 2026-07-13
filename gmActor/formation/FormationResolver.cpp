/**
 * @file formation/FormationResolver.cpp
 * @brief Implementation of FormationResolver.
 */

#include "gmActor/formation/FormationResolver.hpp"

#include <algorithm>

namespace gmActor {

// ── Constructor ───────────────────────────────────────────────────────────────

FormationResolver::FormationResolver(std::vector<FormationCriterion> criteria)
	: _criteria(std::move(criteria))
{}

// ── Private helper ────────────────────────────────────────────────────────────

bool FormationResolver::compare(const ActorId&    a,
                                const ActorId&    b,
                                const ActorStore& store) const
{
	for (const FormationCriterion& criterion : _criteria)
	{
		bool a_before_b = criterion(a, b, store);
		bool b_before_a = criterion(b, a, store);

		if (a_before_b && !b_before_a)
			return true;   // a is strictly preferred

		if (b_before_a && !a_before_b)
			return false;  // b is strictly preferred

		// Both returned the same value — criterion is not discriminating; try next.
	}
	// All criteria exhausted and still tied — maintain original order (stable_sort).
	return false;
}

// ── Sorting ───────────────────────────────────────────────────────────────────

std::vector<ActorId>
FormationResolver::sort_candidates(const std::vector<ActorId>& backline_actors,
                                   const ActorStore&            store) const
{
	std::vector<ActorId> result(backline_actors);

	std::stable_sort(result.begin(), result.end(),
		[this, &store](const ActorId& a, const ActorId& b)
		{
			return compare(a, b, store);
		});

	return result;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

int FormationResolver::criteria_count() const
{
	return static_cast<int>(_criteria.size());
}

} // namespace gmActor
