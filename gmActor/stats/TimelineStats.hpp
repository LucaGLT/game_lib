#ifndef GMACTOR_STATS_TIMELINESTATS_HPP
#define GMACTOR_STATS_TIMELINESTATS_HPP

/**
 * @file stats/TimelineStats.hpp
 * @brief Value object holding timeline-ordering statistics.
 *
 * `TimelineStats` is extracted from `ActorStateCommon` for contexts that need
 * only the ordering data without the full actor state.
 *
 * The game engine (typically `gmFlow`) uses `timeline_position` and
 * `tie_break_rank` to determine initiative / activation order.  Lower
 * position values typically activate earlier; ties are broken by rank.
 */

namespace gmActor {

/**
 * @brief Lightweight value struct holding the two timeline-ordering fields.
 *
 * Both fields mirror the same-named fields in `ActorStateCommon`.
 */
struct TimelineStats {
    int timeline_position = 0; ///< Primary initiative value (lower = earlier)
    int tie_break_rank    = 0; ///< Secondary ordering for equal positions
};

} // namespace gmActor

#endif // GMACTOR_STATS_TIMELINESTATS_HPP
