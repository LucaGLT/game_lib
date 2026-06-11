#ifndef GMFLOW_ROUNDPOLICY_HPP
#define GMFLOW_ROUNDPOLICY_HPP

/**
 * @file flow/RoundPolicy.hpp
 * @brief Configuration flags for round management within a session.
 */

namespace gmFlow {

/**
 * @struct RoundPolicy
 * @brief Controls whether rounds are tracked and when a session ends by round limit.
 *
 * Rounds are optional. Set `enabled = false` for games that track only phases
 * or turns without a formal round counter (e.g. real-time or phase-based games).
 *
 * @par Example — five-round wargame
 * @code
 *   gmFlow::RoundPolicy rp;
 *   rp.enabled    = true;
 *   rp.max_rounds = 5;
 * @endcode
 *
 * @par Example — open-ended dungeon crawl
 * @code
 *   gmFlow::RoundPolicy rp;
 *   rp.enabled    = true;
 *   rp.max_rounds = -1;  // unlimited
 * @endcode
 */
struct RoundPolicy {
    /// Enable round counting. If false, Round objects are not created.
    bool enabled = true;

    /// Maximum number of rounds before the session automatically ends.
    /// Use -1 (default) for an unlimited number of rounds.
    int max_rounds = -1;
};

} // namespace gmFlow

#endif // GMFLOW_ROUNDPOLICY_HPP
