#ifndef GMFLOW_FLOWPHASE_HPP
#define GMFLOW_FLOWPHASE_HPP

/**
 * @file flow/FlowPhase.hpp
 * @brief IPhase implementation that owns an inner IFlowController.
 *
 * FlowPhase is the central primitive for nested game-flow hierarchies.  From
 * the enclosing (parent) IFlowController's perspective it behaves exactly like
 * any other IPhase — the parent calls on_enter(), on_exit(), is_complete() and
 * available_actions() without knowing anything about the internal sub-flow.
 *
 * Internally, FlowPhase owns:
 *  - an IFlowController that manages the sub-phases (sub-rounds, sub-turns…),
 *  - a PhaseContext that isolates local IDs (phase/round/turn) from the parent
 *    session context while sharing GameState, ActorRegistry and EventBus.
 *
 * ### Design guarantees
 *  - GameState, ActorRegistry and EventBus are **never duplicated**.  Any
 *    mutation made by a sub-action is immediately visible at all nesting levels.
 *  - current_round_id() / current_turn_id() on the parent GameContext are
 *    untouched while the inner controller writes into the PhaseContext.
 *  - SequentialFlowController recognises FlowPhase instances (via dynamic_cast)
 *    and routes tick() / accept_action() / can_actor_act() transparently.
 *
 * ### Typical usage
 * @code
 *   // Build inner sub-phases for an "Epoch" level.
 *   std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
 *   inner.push_back(std::make_unique<MorningPhase>());
 *   inner.push_back(std::make_unique<EveningPhase>());
 *
 *   auto epoch = std::make_unique<gmFlow::FlowPhase>(
 *       "epoch_1",
 *       std::make_unique<gmFlow::SequentialFlowController>(std::move(inner)));
 *
 *   // Pass it to the outer controller as a plain IPhase.
 *   outer_phases.push_back(std::move(epoch));
 * @endcode
 *
 * @see PhaseContext
 * @see SequentialFlowController
 */

#include "gmFlow/flow/IPhase.hpp"
#include "gmFlow/flow/IFlowController.hpp"
#include "gmFlow/flow/PhaseContext.hpp"
#include "gmFlow/core/Result.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gmFlow {

/**
 * @class FlowPhase
 * @brief IPhase that encapsulates a complete inner IFlowController.
 *
 * One FlowPhase corresponds to one named nesting level (e.g. "epoch_1").
 * The inner controller is started in on_enter() and ticked every game-loop
 * iteration through the transparent integration in SequentialFlowController.
 *
 * The PhaseContext is constructed lazily inside on_enter() because the parent
 * GameContext is not available at construction time.
 *
 * @note Not thread-safe; all calls must originate from the same game-loop thread.
 */
class FlowPhase : public IPhase
{
public:
	/**
	 * @brief Constructs a FlowPhase with a pre-built inner controller.
	 *
	 * The caller constructs the inner controller (typically
	 * SequentialFlowController) with its sub-phases and transfers ownership
	 * here.  The scope_prefix identifies this nesting level in logs and
	 * event payloads.
	 *
	 * @param scope_prefix Short non-empty label for this level (e.g. @c "epoch_1").
	 * @param controller   Fully configured inner controller; must not be null.
	 * @throws std::invalid_argument if @p scope_prefix is empty or
	 *         @p controller is null.
	 */
	FlowPhase(std::string                      scope_prefix,
	          std::unique_ptr<IFlowController> controller);

	// ── IPhase interface ──────────────────────────────────────────────────

	/// @brief Returns the scope_prefix as the phase identifier.
	PhaseId id() const override;

	/**
	 * @brief Builds the PhaseContext and starts the inner controller.
	 *
	 * Constructs a PhaseContext borrowing GameState, ActorRegistry and
	 * EventBus from @p parent_ctx, then calls IFlowController::start() on
	 * the inner controller.
	 *
	 * @param parent_ctx Mutable parent session context.
	 */
	void on_enter(GameContext& parent_ctx) override;

	/**
	 * @brief Marks this FlowPhase as exited.
	 *
	 * The inner PhaseContext is kept alive for inspection but the inner
	 * controller will no longer receive tick() calls after this point.
	 *
	 * @param ctx Mutable session context (unused but required by IPhase).
	 */
	void on_exit(GameContext& ctx) override;

	/**
	 * @brief Returns the actions available on the active inner phase.
	 *
	 * Obtains the currently active inner phase from the inner controller
	 * (requires the controller to be a SequentialFlowController) and
	 * delegates available_actions() on the PhaseContext.  Returns an empty
	 * vector if this FlowPhase has not been entered or the inner controller
	 * does not support current_phase().
	 *
	 * @param ctx   Read-only session context (ignored; inner PhaseContext is used).
	 * @param actor The querying actor.
	 * @return Actions available to @p actor on the active inner phase.
	 */
	std::vector<std::unique_ptr<IAction>>
	    available_actions(const GameContext& ctx,
	                      const ActorId&     actor) const override;

	/**
	 * @brief Returns true when the inner controller has finished all its phases.
	 *
	 * @param ctx Read-only session context (unused; inner state is queried).
	 * @return true if entered and IFlowController::is_session_complete() is true.
	 */
	bool is_complete(const GameContext& ctx) const override;

	// ── FlowPhase-specific API ────────────────────────────────────────────

	/**
	 * @brief Advances the inner controller by one tick.
	 *
	 * Called by SequentialFlowController::process() each game-loop iteration
	 * when this FlowPhase is the active outer phase.  May also be called
	 * directly by game-specific code when using a custom outer controller.
	 *
	 * @param ctx Mutable session context (ignored; inner PhaseContext is used).
	 */
	void tick(GameContext& ctx);

	/**
	 * @brief Routes an action submission to the inner controller.
	 *
	 * Called by SequentialFlowController::accept_action() when this FlowPhase
	 * is the active outer phase.
	 *
	 * @param ctx    Mutable session context (ignored; inner PhaseContext is used).
	 * @param actor  The submitting actor.
	 * @param action The action to submit.
	 * @return Validation result from the inner controller.
	 */
	ValidationResult accept_action(GameContext&               ctx,
	                               const ActorId&             actor,
	                               std::unique_ptr<IAction>   action);

	/**
	 * @brief Returns whether the given actor can act in the inner controller.
	 *
	 * @param ctx   Read-only session context (ignored; inner PhaseContext used).
	 * @param actor The querying actor.
	 * @return true if the inner controller allows the actor to act.
	 */
	bool can_actor_act(const GameContext& ctx, const ActorId& actor) const;

	/// @brief Returns the PhaseContext owned by this FlowPhase.
	/// @throws std::logic_error if called before on_enter().
	const PhaseContext& phase_context() const;

	/// @brief Returns the scope prefix supplied at construction.
	const std::string& scope_prefix() const;

private:
	std::string                      _scope_prefix;
	std::unique_ptr<IFlowController> _controller;
	std::optional<PhaseContext>      _phase_ctx; // constructed lazily in on_enter()
	bool                             _entered = false;
};

} // namespace gmFlow

#endif // GMFLOW_FLOWPHASE_HPP
