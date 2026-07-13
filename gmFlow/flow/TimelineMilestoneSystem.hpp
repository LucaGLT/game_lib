#ifndef GMFLOW_TIMELINEMILESTONESYSTEM_HPP
#define GMFLOW_TIMELINEMILESTONESYSTEM_HPP

/**
 * @file flow/TimelineMilestoneSystem.hpp
 * @brief Threshold-based event scheduler for continuous-timeline games.
 *
 * `TimelineMilestoneSystem` fires registered callbacks whenever the minimum
 * timeline position crosses a pre-set threshold.  It is a standalone,
 * ownable object — not a base class and not a subclass of
 * @ref ITimelineAdapter.
 *
 * ### Integration pattern
 * The game adapter owns one instance and delegates to it inside
 * `ITimelineAdapter::on_time_advanced()`:
 *
 * @code
 *   class MyAdapter : public gmFlow::ITimelineAdapter
 *   {
 *   public:
 *       MyAdapter()
 *       {
 *           // "at time 12, open the steam valve"
 *           _milestones.add_milestone(12,
 *               [](gmFlow::TimelineValue t, gmFlow::GameContext& ctx)
 *               {
 *                   static_cast<MyState&>(ctx.state()).open_steam_valve();
 *               });
 *
 *           // "at time 60, mission fails — fires once"
 *           _milestones.add_milestone(60,
 *               [](gmFlow::TimelineValue, gmFlow::GameContext& ctx)
 *               {
 *                   static_cast<MyState&>(ctx.state()).mission_failed = true;
 *               });
 *       }
 *
 *       void on_time_advanced(gmFlow::GameContext& ctx,
 *                             gmFlow::TimelineValue old_time,
 *                             gmFlow::TimelineValue new_time) override
 *       {
 *           _milestones.advance(old_time, new_time, ctx);
 *       }
 *
 *   private:
 *       gmFlow::TimelineMilestoneSystem _milestones;
 *   };
 * @endcode
 *
 * ### Firing semantics
 * A milestone fires when `old_time < threshold <= new_time`.  The lower
 * bound is **exclusive** so a milestone at time 5 does not fire again when
 * an actor whose position was already 5 advances further from that value.
 *
 * Milestones at the same threshold fire in registration order.
 * Multiple `advance()` calls per threshold fire persistent milestones each
 * time they are crossed.
 *
 * ### Thread safety
 * Not thread-safe.  External synchronisation is required if `advance()` or
 * `add_milestone()` are called from multiple threads.
 */

#include "gmFlow/flow/TimelineTypes.hpp"
#include "gmFlow/core/GameContext.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace gmFlow {

/**
 * @class TimelineMilestoneSystem
 * @brief Fires registered callbacks when the timeline crosses a threshold.
 */
class TimelineMilestoneSystem
{
public:
	// ── Callback type ─────────────────────────────────────────────────────

	/**
	 * @brief Callback invoked when the timeline crosses a milestone threshold.
	 *
	 * @param threshold  The threshold value that was crossed.
	 * @param ctx        Mutable session context at the moment of firing.
	 */
	using Callback = std::function<void(TimelineValue threshold, GameContext& ctx)>;

	// ── Registration ──────────────────────────────────────────────────────

	/**
	 * @brief Registers a callback to fire when the timeline reaches `threshold`.
	 *
	 * Multiple callbacks may be registered at the same threshold; they fire
	 * in registration order.
	 *
	 * @param threshold  Timeline position that triggers the callback.
	 * @param cb         Callable to invoke.  Must not be null.
	 * @param one_shot   If true (default), the entry is removed after the
	 *                   first firing.  If false, it fires on every crossing.
	 */
	void add_milestone(TimelineValue threshold, Callback cb, bool one_shot = true);

	/**
	 * @brief Removes all milestone entries registered at `threshold`.
	 *
	 * No-op if no entry exists at that threshold.
	 *
	 * @param threshold  Threshold whose entries should be removed.
	 */
	void remove_milestone(TimelineValue threshold);

	/**
	 * @brief Removes all registered milestones.
	 */
	void clear();

	// ── Firing ────────────────────────────────────────────────────────────

	/**
	 * @brief Fires all milestones in the half-open interval `(old_time, new_time]`.
	 *
	 * Called by the game adapter from `ITimelineAdapter::on_time_advanced()`.
	 * Milestones are fired in ascending threshold order.  One-shot entries
	 * are removed after firing.
	 *
	 * If `new_time <= old_time` (no forward advance), no milestone fires.
	 *
	 * @param old_time  Previous minimum timeline position (exclusive bound).
	 * @param new_time  New minimum timeline position (inclusive bound).
	 * @param ctx       Mutable session context passed to each callback.
	 */
	void advance(TimelineValue old_time, TimelineValue new_time, GameContext& ctx);

	// ── Queries ───────────────────────────────────────────────────────────

	/**
	 * @brief Returns the threshold of the next milestone that has not yet fired.
	 *
	 * "Next" is defined as the smallest registered threshold that is strictly
	 * greater than `after`.  If no such milestone exists, returns
	 * `std::nullopt`.
	 *
	 * @param after  Only milestones with `threshold > after` are considered.
	 *               Pass the current timeline time to get the next upcoming
	 *               milestone.
	 * @return Threshold of the next milestone, or `std::nullopt` if none.
	 */
	std::optional<TimelineValue> next_threshold(TimelineValue after = 0) const;

	/**
	 * @brief Returns the total number of registered milestone entries.
	 *
	 * Multiple entries at the same threshold each count as one.
	 */
	int milestone_count() const;

private:
	// ── Internal entry ────────────────────────────────────────────────────

	struct Entry
	{
		TimelineValue threshold;
		Callback      callback;
		bool          one_shot;
	};

	// Entries are kept in ascending threshold order; stable_sort is used
	// when entries are added to maintain registration order within the
	// same threshold.
	std::vector<Entry> _entries;

	void sort_entries();
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEMILESTONESYSTEM_HPP
