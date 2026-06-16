#ifndef GMFLOW_TIMELINEEVENTS_HPP
#define GMFLOW_TIMELINEEVENTS_HPP

/**
 * @file events/TimelineEvents.hpp
 * @brief Concrete event structs for @ref TimelineFlowController lifecycle events.
 *
 * All structs inherit from @ref IEvent and correspond to one of the
 * `EVT_TIMELINE_*` constants declared in @ref EventType.hpp.
 *
 * Events are **non-owning value types**: they carry IDs and numeric snapshots
 * only — never pointers or references to live objects.
 *
 * ### Subscribing
 * @code
 *   session.event_bus().subscribe(gmFlow::EVT_TIMELINE_ACTOR_SELECTED,
 *       [](const gmFlow::IEvent& e) {
 *           const auto& ev =
 *               static_cast<const gmFlow::TimelineActorSelectedEvent&>(e);
 *           // ev.actor_id, ev.timeline_position
 *       });
 * @endcode
 */

#include "gmFlow/events/IEvent.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/core/Ids.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"

#include <vector>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// TimelineActorSelectedEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Published when the controller selects the next active timeline actor.
 *
 * Fires once per selection, after `ITimelineAdapter::on_actor_selected()` is
 * called and before the main @ref ActionWindow is opened.
 */
struct TimelineActorSelectedEvent : public IEvent
{
	EventType type() const override { return EVT_TIMELINE_ACTOR_SELECTED; }

	/// ID of the newly selected active actor.
	ActorId actor_id;

	/// Timeline position of the selected actor at the moment of selection.
	TimelineValue timeline_position = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// TimelineTimeAdvancedEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Published when the minimum timeline position advances.
 *
 * Fires after `ITimelineAdapter::on_time_advanced()` has been called,
 * so subscribers receive the event after the adapter has already reacted.
 */
struct TimelineTimeAdvancedEvent : public IEvent
{
	EventType type() const override { return EVT_TIMELINE_TIME_ADVANCED; }

	/// Previous minimum timeline position.
	TimelineValue old_time = 0;

	/// New minimum timeline position (always strictly greater than `old_time`).
	TimelineValue new_time = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// TimelineTieDetectedEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Published when two or more enabled actors share the same lowest
 *        timeline position (before tie-break rank is applied).
 *
 * The event is informational: the controller has already resolved the tie
 * deterministically before publishing.  Game code may use this event to
 * display UI feedback (e.g. "simultaneous initiative!") without needing to
 * re-implement tie detection.
 */
struct TimelineTieDetectedEvent : public IEvent
{
	EventType type() const override { return EVT_TIMELINE_TIE_DETECTED; }

	/// All actors that share the lowest position (unordered, pre tie-break).
	std::vector<ActorId> tied_actors;

	/// The shared timeline position value.
	TimelineValue timeline_position = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// TimelineNoActorAvailableEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Published when no enabled actor is available for selection.
 *
 * This may happen when all actors are disabled simultaneously (e.g. all
 * eliminated).  The session is not automatically ended; game code should
 * handle this event and decide whether to end the session or re-enable actors.
 */
struct TimelineNoActorAvailableEvent : public IEvent
{
	EventType type() const override { return EVT_TIMELINE_NO_ACTOR_AVAILABLE; }
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEEVENTS_HPP
