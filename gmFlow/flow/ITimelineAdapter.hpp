#ifndef GMFLOW_ITIMELINEADAPTER_HPP
#define GMFLOW_ITIMELINEADAPTER_HPP

/**
 * @file flow/ITimelineAdapter.hpp
 * @brief Adapter interface that supplies game-specific timeline data to
 *        @ref TimelineFlowController.
 *
 * The controller deliberately knows nothing about hit points, initiative
 * cards, map state, or any other game concept.  The adapter bridges the gap:
 * it reads game-specific state and translates it into the three numeric/boolean
 * values the controller needs.
 *
 * ### Responsibilities
 * **Adapter (game code):**
 * - Which actors participate in the timeline.
 * - Each actor's current numeric position.
 * - Whether an actor is enabled (eligible to act).
 * - Tie-break rank when positions are equal.
 * - Whether the active actor should retain control after an action.
 * - Whether the session is over.
 * - Reacting to `on_actor_selected()` and `on_time_advanced()` callbacks.
 *
 * **Controller (@ref TimelineFlowController):**
 * - Selection order (lowest position wins, then lowest rank, then stable order).
 * - Opening and closing @ref ActionWindow instances.
 * - Publishing timeline events.
 * - Delegating all game-rule resolution back to the adapter or to @ref IAction.
 *
 * @note The adapter receives mutable `GameContext&` in callbacks so it can
 *       publish its own events or mutate game state in response to timeline
 *       transitions.  Read-only queries receive `const GameContext&`.
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"

#include <vector>

namespace gmFlow {

/**
 * @class ITimelineAdapter
 * @brief Pure-virtual adapter that supplies timeline data to
 *        @ref TimelineFlowController.
 */
class ITimelineAdapter
{
public:
	virtual ~ITimelineAdapter() = default;

	// ── Query interface (const) ───────────────────────────────────────────

	/**
	 * @brief Returns all actor IDs participating in this timeline.
	 *
	 * The order of the returned vector is used as a stable tie-break when
	 * `TimelinePolicy::stable_tie_break` is true and position + rank are
	 * equal.  The controller treats the vector as the definitive actor list
	 * and does **not** consult the @ref ActorRegistry directly.
	 *
	 * @param ctx Read-only session context.
	 * @return Ordered list of actor IDs.
	 */
	virtual std::vector<ActorId>
	timeline_actors(const GameContext& ctx) const = 0;

	/**
	 * @brief Returns the current timeline position of the given actor.
	 *
	 * Lower values mean the actor acts sooner.  The controller never
	 * modifies positions; all position updates are the responsibility of
	 * game-specific actions that call back into the adapter or game state.
	 *
	 * @param ctx   Read-only session context.
	 * @param actor Actor whose position is requested.
	 * @return Signed 64-bit timeline position.
	 */
	virtual TimelineValue
	timeline_position(const GameContext& ctx,
	                  const ActorId& actor) const = 0;

	/**
	 * @brief Returns whether the given actor is eligible to act this cycle.
	 *
	 * Disabled actors are excluded from selection entirely.  Use this to
	 * represent dead actors, stunned actors, or actors whose turn is
	 * deferred for game-specific reasons.
	 *
	 * @param ctx   Read-only session context.
	 * @param actor Actor to check.
	 * @return true if the actor may be selected as active.
	 */
	virtual bool
	is_actor_enabled(const GameContext& ctx,
	                 const ActorId& actor) const = 0;

	/**
	 * @brief Returns the tie-break rank for the given actor.
	 *
	 * Used when two or more actors share the same lowest timeline position.
	 * Lower rank wins.  Ties on rank too are resolved by insertion order
	 * (position in the list returned by `timeline_actors()`) when
	 * `TimelinePolicy::stable_tie_break` is true.
	 *
	 * @param ctx   Read-only session context.
	 * @param actor Actor whose tie-break rank is requested.
	 * @return Integer rank; lower value = higher priority.
	 */
	virtual int
	tie_break_rank(const GameContext& ctx,
	               const ActorId& actor) const = 0;

	/**
	 * @brief Returns whether the active actor should retain control after
	 *        completing an action.
	 *
	 * When this returns true, the controller reopens a main action window
	 * for the same actor without advancing to the next selection.  Game
	 * code uses this for multi-part combos, card chain sequences, or any
	 * scenario where one actor acts multiple times before yielding.
	 *
	 * @param ctx   Read-only session context.
	 * @param actor Currently active actor.
	 * @return true to keep the actor active after action completion.
	 */
	virtual bool
	actor_keeps_control(const GameContext& ctx,
	                    const ActorId& actor) const = 0;

	/**
	 * @brief Returns whether the session should be considered complete.
	 *
	 * The controller calls this before each selection and at the start
	 * of `process()`.  Once this returns true, the controller stops
	 * selecting actors and emits no further timeline events.
	 *
	 * @param ctx Read-only session context.
	 * @return true if the session is over.
	 */
	virtual bool
	is_session_complete(const GameContext& ctx) const = 0;

	// ── Callback interface (mutable) ──────────────────────────────────────

	/**
	 * @brief Called immediately after an actor is selected as active.
	 *
	 * The adapter may use this to apply selection-triggered state changes
	 * (e.g. resetting a per-turn action counter).  The controller calls
	 * this before opening the main action window.
	 *
	 * @param ctx   Mutable session context.
	 * @param actor Newly selected active actor.
	 */
	virtual void
	on_actor_selected(GameContext& ctx,
	                  const ActorId& actor) = 0;

	/**
	 * @brief Called when the minimum timeline position advances.
	 *
	 * The adapter may use this to resolve threshold events (e.g. triggering
	 * hazards at specific mission times) without the controller knowing
	 * what those events mean.
	 *
	 * @param ctx      Mutable session context.
	 * @param old_time Previous minimum position value.
	 * @param new_time New minimum position value.
	 */
	virtual void
	on_time_advanced(GameContext& ctx,
	                 TimelineValue old_time,
	                 TimelineValue new_time) = 0;
};

} // namespace gmFlow

#endif // GMFLOW_ITIMELINEADAPTER_HPP
