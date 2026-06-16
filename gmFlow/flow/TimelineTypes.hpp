#ifndef GMFLOW_TIMELINETYPES_HPP
#define GMFLOW_TIMELINETYPES_HPP

/**
 * @file flow/TimelineTypes.hpp
 * @brief Fundamental numeric type for continuous-timeline flow control.
 *
 * `TimelineValue` represents an actor's position on a continuous mission
 * timeline.  Lower values mean the actor acts sooner.  The signed 64-bit
 * integer range is deliberately generous so game-specific code is free to
 * scale, offset, or negate values without risk of overflow.
 *
 * Game-specific code may restrict values to non-negative numbers via
 * `ITimelineAdapter::is_actor_enabled()` or adapter-side validation.
 */

#include <cstdint>

namespace gmFlow {

/// @brief Position of an actor on the continuous mission timeline.
///
/// Actors with a lower `TimelineValue` act before actors with a higher value.
/// Negative values are permitted at the framework level; game-specific
/// adapters may reject them.
using TimelineValue = std::int64_t;

} // namespace gmFlow

#endif // GMFLOW_TIMELINETYPES_HPP
