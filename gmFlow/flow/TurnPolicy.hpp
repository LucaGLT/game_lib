#ifndef GMFLOW_TURNPOLICY_HPP
#define GMFLOW_TURNPOLICY_HPP

/**
 * @file flow/TurnPolicy.hpp
 * @brief Configurable behaviour flags for turn management within a session.
 */

namespace gmFlow {

/**
 * @struct TurnPolicy
 * @brief Flags that control how turns are structured within a session.
 *
 * Populate a TurnPolicy and pass it inside @ref SessionConfig to customise
 * turn behaviour without subclassing @ref IFlowController.
 *
 * @par Example — simultaneous planning (Gloomhaven style)
 * @code
 *   gmFlow::TurnPolicy tp;
 *   tp.allow_simultaneous_turns    = true;
 *   tp.require_all_actors_to_pass  = false;
 * @endcode
 *
 * @par Example — strict sequential turns (HeroQuest style)
 * @code
 *   gmFlow::TurnPolicy tp;
 *   // All fields keep their defaults — sequential, synchronous, single action.
 * @endcode
 */
struct TurnPolicy {
    /// If true, all eligible actors may act simultaneously inside one ActionWindow.
    bool allow_simultaneous_turns = false;

    /// If true, out-of-turn async actions are accepted by open ActionWindows.
    bool allow_async_actions = false;

    /// If true, the turn ends only after every eligible actor has explicitly passed.
    bool require_all_actors_to_pass = false;

    /// If true, one actor may submit more than one action per turn.
    bool allow_multiple_actions_per_turn = false;
};

} // namespace gmFlow

#endif // GMFLOW_TURNPOLICY_HPP
