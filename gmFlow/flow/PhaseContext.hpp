#ifndef GMFLOW_PHASECONTEXT_HPP
#define GMFLOW_PHASECONTEXT_HPP

/**
 * @file flow/PhaseContext.hpp
 * @brief Local context for a nested flow scope (FlowPhase).
 *
 * PhaseContext extends GameContext so that an internal IFlowController
 * (owned by a FlowPhase) can write phase/round/turn IDs into its own
 * isolated slots without corrupting the parent session's IDs.
 *
 * Services that are inherently global — GameState, ActorRegistry, EventBus —
 * are **borrowed** (by reference) from the parent context and are never
 * duplicated.  Mutations to GameState are therefore visible at every nesting
 * level, while `current_round_id()` / `current_turn_id()` return values that
 * are local to this scope.
 *
 * ### Usage inside FlowPhase::on_enter()
 * @code
 *   void EpochPhase::on_enter(gmFlow::GameContext& parent_ctx) override
 *   {
 *       _phase_ctx.emplace(parent_ctx, "epoch_1");
 *       _controller->start(*_phase_ctx);
 *   }
 * @endcode
 *
 * @see FlowPhase
 * @see GameContext
 */

#include "gmFlow/core/GameContext.hpp"

#include <string>

namespace gmFlow {

/**
 * @class PhaseContext
 * @brief GameContext subclass with isolated phase/round/turn IDs.
 *
 * One PhaseContext is owned by each FlowPhase instance.  It is constructed
 * lazily inside @ref FlowPhase::on_enter() when the parent GameContext is
 * first available, and destroyed (or re-initialised) on @ref FlowPhase::on_exit().
 *
 * The @p scope_prefix identifies the nesting level in log output and in
 * event payloads (e.g. `"epoch_1"`, `"encounter_2"`).
 */
class PhaseContext : public GameContext
{
public:
	/**
	 * @brief Constructs a PhaseContext that borrows services from @p parent.
	 *
	 * The new context shares the same SessionId, GameState, ActorRegistry and
	 * EventBus as @p parent.  Its own phase/round/turn IDs start empty and
	 * are written exclusively by the sub-controller.
	 *
	 * @param parent       Parent (root or enclosing) GameContext; must outlive
	 *                     this object.
	 * @param scope_prefix Short identifier for this scope (e.g. @c "epoch_1").
	 *                     Must not be empty.
	 */
	PhaseContext(GameContext& parent, std::string scope_prefix);

	/// @brief Returns the scope prefix supplied at construction.
	const std::string& scope_prefix() const;

private:
	std::string _scope_prefix;
};

} // namespace gmFlow

#endif // GMFLOW_PHASECONTEXT_HPP
