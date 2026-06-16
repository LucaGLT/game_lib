#ifndef GMFLOW_TIMELINEFLOWCONTROLLER_HPP
#define GMFLOW_TIMELINEFLOWCONTROLLER_HPP

/**
 * @file flow/TimelineFlowController.hpp
 * @brief Continuous-timeline flow controller for turn-based games.
 *
 * `TimelineFlowController` is an alternative to @ref SequentialFlowController
 * for games where the *next* actor is always whoever has the **lowest**
 * numeric timeline position, rather than a fixed round-based order.
 *
 * ### Design principles
 * - **Generic**: the controller never references hit points, cards, maps,
 *   monsters, player classes, or any other game concept.
 * - **Adapter-driven**: all game-specific data is supplied through
 *   @ref ITimelineAdapter.
 * - **Phase-free**: rounds are optional and unused; the session ends when
 *   `ITimelineAdapter::is_session_complete()` returns true.
 * - **Atomic actions**: V1 actions are atomic; there is no mid-action
 *   suspension at the controller level.
 *
 * ### Typical usage
 * @code
 *   class MyAdapter : public gmFlow::ITimelineAdapter { ... };
 *
 *   auto ctrl = std::make_unique<gmFlow::TimelineFlowController>(
 *       std::make_unique<MyAdapter>()
 *   );
 *   gmFlow::GameSession session(cfg, std::move(ctrl), std::move(state), disp);
 *   session.start();
 *
 *   // Each tick:
 *   session.tick();
 * @endcode
 *
 * ### Selection algorithm
 * Actors are sorted by:
 * 1. `timeline_position` ascending.
 * 2. `tie_break_rank` ascending.
 * 3. Insertion order in `ITimelineAdapter::timeline_actors()` when
 *    `TimelinePolicy::stable_tie_break` is true.
 *
 * @note Timer-based window expiry is deferred to V2.
 */

#include "gmFlow/flow/IFlowController.hpp"
#include "gmFlow/flow/ITimelineAdapter.hpp"
#include "gmFlow/flow/TimelinePolicy.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"
#include "gmFlow/actions/ActionWindow.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace gmFlow {

/**
 * @class TimelineFlowController
 * @brief IFlowController implementation that selects actors by timeline position.
 *
 * One instance is injected into @ref GameSession at construction time.
 * The session calls `start()` once and `process()` every tick.
 */
class TimelineFlowController : public IFlowController
{
public:
	// ── Construction ─────────────────────────────────────────────────────

	/**
	 * @brief Constructs the controller with the given adapter and policy.
	 *
	 * @param adapter Non-null adapter supplying game-specific timeline data.
	 *                Ownership is transferred to the controller.
	 * @param policy  Behaviour flags; defaults to the recommended settings.
	 *
	 * @throws std::invalid_argument if `adapter` is null.
	 */
	explicit TimelineFlowController(
	    std::unique_ptr<ITimelineAdapter> adapter,
	    TimelinePolicy policy = TimelinePolicy{});

	// ── IFlowController interface ─────────────────────────────────────────

	/**
	 * @brief Initialises the timeline and selects the first active actor.
	 *
	 * Computes the initial `current_time_` from the lowest enabled position,
	 * then calls `select_next_actor()`.  Must be called exactly once, by
	 * @ref GameSession::start().
	 *
	 * @param ctx Mutable session context.
	 */
	void start(GameContext& ctx) override;

	/**
	 * @brief Advances flow state if the current action window is complete.
	 *
	 * Called every session tick by @ref GameSession::tick().  If the adapter
	 * reports the session is complete, does nothing.  If the current window
	 * is open and not yet complete, does nothing.  Otherwise selects the
	 * next actor (if `auto_select_next_actor` is enabled).
	 *
	 * @param ctx Mutable session context.
	 */
	void process(GameContext& ctx) override;

	/**
	 * @brief Returns true if the given actor may submit an action right now.
	 *
	 * Returns true when:
	 * - The actor is enabled, AND
	 * - Either the current @ref ActionWindow lists this actor as eligible, OR
	 *   this actor is the currently active actor.
	 *
	 * This is a structural flow check only; game-rule legality is handled
	 * in `IAction::validate()`.
	 *
	 * @param ctx   Read-only session context.
	 * @param actor Actor to check.
	 * @return true if the actor may act right now.
	 */
	bool can_actor_act(const GameContext& ctx,
	                   const ActorId& actor) const override;

	/**
	 * @brief Reacts to the completion of an action.
	 *
	 * - Recomputes `current_time_`; fires `on_time_advanced()` and publishes
	 *   @ref TimelineTimeAdvancedEvent if the minimum position has advanced.
	 * - If the active actor retains control, reopens (or keeps) the main window.
	 * - Otherwise clears `active_actor_`, resets the window, and auto-selects
	 *   the next actor if the policy allows.
	 *
	 * @param ctx    Mutable session context.
	 * @param result Result of the completed action.
	 */
	void on_action_completed(GameContext& ctx,
	                         const ActionResult& result) override;

	/**
	 * @brief Routes a validated action to the current ActionWindow.
	 *
	 * Called by @ref GameSession::submit_action() after eligibility and
	 * game-rule validation have passed.  Delivers the action to
	 * `current_window_` if one is open.
	 *
	 * @param ctx    Mutable session context.
	 * @param actor  The actor submitting the action.
	 * @param action The validated action; ownership is transferred.
	 * @return ValidationResult::ok() if accepted, or fail if no window is open.
	 */
	ValidationResult accept_action(GameContext&             ctx,
	                               const ActorId&           actor,
	                               std::unique_ptr<IAction> action) override;

	/**
	 * @brief Delegates to `ITimelineAdapter::is_session_complete()`.
	 *
	 * @param ctx Read-only session context.
	 * @return true if the session is over.
	 */
	bool is_session_complete(const GameContext& ctx) const override;

	// ── Timeline-specific accessors ───────────────────────────────────────

	/**
	 * @brief Returns the currently active actor, if any.
	 * @return Optional containing the active actor ID, or empty.
	 */
	const std::optional<ActorId>& active_actor() const;

	/**
	 * @brief Returns the current mission time (minimum enabled position).
	 * @return Current timeline value.
	 */
	TimelineValue current_time() const;

	/**
	 * @brief Returns true if an action window is currently open.
	 * @return true if `current_window_` has a value and is not closed.
	 */
	bool has_action_window() const;

	/**
	 * @brief Forcibly closes the current action window without executing actions.
	 *
	 * Used by game code that needs to cancel the window (e.g. session abort).
	 */
	void force_close_action_window();

	/**
	 * @brief Opens a reaction/instant-action window for the given actors.
	 *
	 * No-op if `TimelinePolicy::allow_reactions` is false.
	 * Replaces any existing window that is already closed.
	 *
	 * @param eligible_actors Actors allowed to submit to this window.
	 * @param policy          Completion policy for the window.
	 */
	void open_reaction_window(std::vector<ActorId> eligible_actors,
	                          CompletionPolicy policy);

protected:
	// ── Protected helpers (overrideable in tests / subclasses) ────────────

	/**
	 * @brief Selects the next active actor using the sorted timeline order.
	 *
	 * Calls `sorted_enabled_actors()`, detects ties, calls adapter hooks,
	 * publishes events, and optionally opens the main action window.
	 *
	 * @param ctx Mutable session context.
	 * @return Optional active actor ID, or empty if none available.
	 */
	std::optional<ActorId> select_next_actor(GameContext& ctx);

	/**
	 * @brief Returns all enabled actors sorted for selection.
	 *
	 * Sort order: `timeline_position` ASC → `tie_break_rank` ASC →
	 * insertion order (when `stable_tie_break` is true).
	 *
	 * @param ctx Read-only session context.
	 * @return Sorted list of enabled actor IDs.
	 */
	std::vector<ActorId> sorted_enabled_actors(const GameContext& ctx) const;

	/**
	 * @brief Computes the current time as the minimum enabled timeline position.
	 *
	 * @param ctx Read-only session context.
	 * @return Minimum position, or `current_time_` if no enabled actor exists.
	 */
	TimelineValue compute_current_time(const GameContext& ctx) const;

	/** @brief Publishes @ref TimelineActorSelectedEvent if events are enabled. */
	void publish_actor_selected(GameContext& ctx,
	                            const ActorId& actor,
	                            TimelineValue position) const;

	/** @brief Publishes @ref TimelineTimeAdvancedEvent if events are enabled. */
	void publish_time_advanced(GameContext& ctx,
	                           TimelineValue old_time,
	                           TimelineValue new_time) const;

	/** @brief Publishes @ref TimelineTieDetectedEvent if events are enabled. */
	void publish_tie_detected(GameContext& ctx,
	                          const std::vector<ActorId>& tied,
	                          TimelineValue position) const;

	/** @brief Publishes @ref TimelineNoActorAvailableEvent if events are enabled. */
	void publish_no_actor_available(GameContext& ctx) const;

private:
	std::unique_ptr<ITimelineAdapter> adapter_;
	TimelinePolicy                    policy_;
	std::optional<ActorId>            active_actor_;
	TimelineValue                     current_time_ = 0;
	std::optional<ActionWindow>       current_window_;
};

} // namespace gmFlow

#endif // GMFLOW_TIMELINEFLOWCONTROLLER_HPP
