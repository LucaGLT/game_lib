#ifndef GMFLOW_TIMELINEPOLICY_HPP
#define GMFLOW_TIMELINEPOLICY_HPP

/**
 * @file flow/TimelinePolicy.hpp
 * @brief Configuration policy for @ref TimelineFlowController.
 *
 * All fields default to their recommended values so that constructing a
 * default-initialised `TimelinePolicy{}` gives a fully functional controller
 * with reactions, auto-advance, event publishing, and stable tie-break.
 *
 * Timer-based window expiry is explicitly absent (V2 feature).
 */

namespace gmFlow {

/**
 * @struct TimelinePolicy
 * @brief Behaviour flags for @ref TimelineFlowController.
 */
struct TimelinePolicy
{
	/// Allow the controller to manage reaction/instant action windows.
	bool allow_reactions = true;

	/// After an action completes and the active actor does not retain control,
	/// automatically select the next lowest-position actor.
	bool auto_select_next_actor = true;

	/// Publish timeline-specific events (actor_selected, time_advanced, etc.)
	/// on the session @ref EventBus.
	bool publish_timeline_events = true;

	/// Open a @ref ActionWindow (CompletionPolicy::MANUAL_CLOSE) for the
	/// selected active actor immediately after selection.
	bool open_main_action_window = true;

	/// When two actors share identical timeline position and tie-break rank,
	/// preserve the order returned by `ITimelineAdapter::timeline_actors()`.
	bool stable_tie_break = true;
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEPOLICY_HPP
