/**
 * @file formation/FormationValidator.cpp
 * @brief Implementation of FormationValidator.
 */

#include "gmActor/formation/FormationValidator.hpp"

#include <algorithm>

namespace gmActor {

// ── Constructor ───────────────────────────────────────────────────────────────

FormationValidator::FormationValidator(FormationRules rules)
	: _rules(rules)
{}

// ── Private helper ────────────────────────────────────────────────────────────

int FormationValidator::max_legal_backline(int frontline_count) const
{
	if (_rules.backline_requires_frontline && frontline_count == 0)
		return 0;

	// When frontline == 0 and backline_requires_frontline == false,
	// max_backline_per_frontline ratio does not apply (0 * N = 0 would
	// wrongly prohibit all backline).  In that case the only constraint is
	// the absolute max_backline cap.
	int ratio_cap = (frontline_count > 0)
		? frontline_count * _rules.max_backline_per_frontline
		: INT32_MAX;

	int result = ratio_cap;
	if (_rules.max_backline >= 0)
		result = std::min(result, _rules.max_backline);

	return result;
}

// ── Validation ────────────────────────────────────────────────────────────────

bool FormationValidator::is_valid(int frontline_count, int backline_count) const
{
	if (frontline_count < 0 || backline_count < 0)
		return false;

	if (_rules.max_frontline >= 0 && frontline_count > _rules.max_frontline)
		return false;

	if (_rules.max_backline >= 0 && backline_count > _rules.max_backline)
		return false;

	if (backline_count > max_legal_backline(frontline_count))
		return false;

	return true;
}

int FormationValidator::backline_overflow(int frontline_count, int backline_count) const
{
	if (frontline_count < 0 || backline_count < 0)
		return 0;

	int limit = max_legal_backline(frontline_count);

	// Absolute backline cap can further restrict the limit.
	if (_rules.max_backline >= 0)
		limit = std::min(limit, _rules.max_backline);

	int overflow = backline_count - limit;
	return (overflow > 0) ? overflow : 0;
}

// ── Accessor ──────────────────────────────────────────────────────────────────

const FormationRules& FormationValidator::rules() const
{
	return _rules;
}

} // namespace gmActor
