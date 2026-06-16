#ifndef GMFLOW_ROUND_HPP
#define GMFLOW_ROUND_HPP

/**
 * @file flow/Round.hpp
 * @brief Represents a single round within a game session.
 *
 * A Round groups one complete pass of turns across all actors.
 * Rounds are optional; they are tracked only when @ref RoundPolicy::enabled
 * is set to true in the @ref SessionConfig.
 */

#include "gmFlow/core/Ids.hpp"

namespace gmFlow {

/**
 * @class Round
 * @brief A numbered container for one full cycle of turns.
 *
 * The @ref IFlowController creates a new Round at the start of each cycle and
 * publishes @ref EVT_ROUND_STARTED / @ref EVT_ROUND_ENDED events on the
 * @ref EventBus.
 *
 * @code
 *   gmFlow::Round r("round_3", 3);
 *   std::cout << "Starting " << r.id()
 *             << " (index " << r.index() << ")\n";
 * @endcode
 */
class Round {
public:
    /**
     * @brief Constructs a Round with the given identifier and 1-based index.
     * @param id    Unique identifier (e.g. "round_3").
     * @param index 1-based position of this round within the session.
     */
    explicit Round(RoundId id, int index);

    /// @brief Returns the unique identifier for this round.
    const RoundId& id() const;

    /// @brief Returns the 1-based round index within the session.
    int index() const;

private:
    RoundId _id;
    int     _index;
};

} // namespace gmFlow

#endif // GMFLOW_ROUND_HPP
