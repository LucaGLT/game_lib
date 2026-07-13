/**
 * @file flow/TimelineMilestoneSystem.cpp
 * @brief Implementation of TimelineMilestoneSystem.
 */

#include "gmFlow/flow/TimelineMilestoneSystem.hpp"

#include <algorithm>

namespace gmFlow {

// ── Registration ──────────────────────────────────────────────────────────────

void TimelineMilestoneSystem::add_milestone(TimelineValue threshold,
                                            Callback      cb,
                                            bool          one_shot)
{
	_entries.push_back(Entry{ threshold, std::move(cb), one_shot });
	sort_entries();
}

void TimelineMilestoneSystem::remove_milestone(TimelineValue threshold)
{
	_entries.erase(
		std::remove_if(_entries.begin(), _entries.end(),
			[threshold](const Entry& e) { return e.threshold == threshold; }),
		_entries.end());
}

void TimelineMilestoneSystem::clear()
{
	_entries.clear();
}

// ── Firing ────────────────────────────────────────────────────────────────────

void TimelineMilestoneSystem::advance(TimelineValue old_time,
                                      TimelineValue new_time,
                                      GameContext&  ctx)
{
	// No forward advance — nothing fires.
	if (new_time <= old_time)
		return;

	// Collect indices of entries that should fire: old_time < threshold <= new_time.
	// We iterate a snapshot of the current entries so that callbacks that add
	// new milestones do not interfere with this pass.
	std::vector<std::size_t> to_fire;
	to_fire.reserve(_entries.size());

	for (std::size_t i = 0; i < _entries.size(); ++i)
	{
		const TimelineValue t = _entries[i].threshold;
		if (t > old_time && t <= new_time)
			to_fire.push_back(i);
	}

	// Fire in ascending index order (already sorted by threshold).
	// After firing, mark one-shot entries for removal.
	std::vector<std::size_t> to_remove;
	for (std::size_t idx : to_fire)
	{
		// The entry might have been removed by a previous callback via
		// remove_milestone(); guard against out-of-bounds.
		if (idx >= _entries.size())
			continue;

		_entries[idx].callback(_entries[idx].threshold, ctx);

		if (_entries[idx].one_shot)
			to_remove.push_back(idx);
	}

	// Remove one-shot entries in reverse index order to keep indices valid.
	std::sort(to_remove.rbegin(), to_remove.rend());
	for (std::size_t idx : to_remove)
	{
		if (idx < _entries.size())
			_entries.erase(_entries.begin() + static_cast<std::ptrdiff_t>(idx));
	}
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<TimelineValue>
TimelineMilestoneSystem::next_threshold(TimelineValue after) const
{
	for (const Entry& e : _entries)
	{
		if (e.threshold > after)
			return e.threshold;
	}
	return std::nullopt;
}

int TimelineMilestoneSystem::milestone_count() const
{
	return static_cast<int>(_entries.size());
}

// ── Private helpers ───────────────────────────────────────────────────────────

void TimelineMilestoneSystem::sort_entries()
{
	// Stable sort preserves registration order within the same threshold.
	std::stable_sort(_entries.begin(), _entries.end(),
		[](const Entry& a, const Entry& b)
		{
			return a.threshold < b.threshold;
		});
}

} // namespace gmFlow
