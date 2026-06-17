#ifndef GMFLOW_ACTIONGATEWAY_HPP
#define GMFLOW_ACTIONGATEWAY_HPP

/**
 * @file bridges/ActionGateway.hpp
 * @brief IAction decorator that adds rules-engine pre/post hooks.
 *
 * `ActionGateway` wraps any existing `IAction` and injects two checkpoints:
 *
 * 1. **Pre-check** — inside `validate()`, *after* the inner action validates.
 *    If the pre-check callback returns `ValidationResult::fail(...)`, the
 *    action is rejected before it ever reaches `execute()`.
 *
 * 2. **Post-hook** — inside `execute()`, *after* the inner action completes.
 *    The post-hook is purely informational: it receives the `ActionResult`
 *    but cannot alter the execution outcome.
 *
 * ### Why pre-check only, no post-block?
 * An action in `execute()` is already running: state mutations and events may
 * have been emitted.  Blocking at that stage would leave the session in an
 * inconsistent state.  Use the pre-check (i.e. `validate()`) as the single
 * enforcement point.
 *
 * ### Usage
 * @code
 *   gmFlow::FlowRulesPayload base_payload;
 *   base_payload.actor_id   = "player_1";
 *   base_payload.action_id  = action->id();
 *   base_payload.phase_id   = ctx.current_phase_id();
 *   base_payload.round_id   = ctx.current_round_id();
 *   base_payload.turn_id    = ctx.current_turn_id();
 *
 *   auto wrapped = std::make_unique<gmFlow::ActionGateway>(
 *       std::move(action),
 *       base_payload,
 *       // pre-check: ask the rules engine
 *       [&engine](const gmFlow::FlowRulesPayload& p) {
 *           if (!engine.allows(p.action_id, p.actor_id))
 *               return gmFlow::ValidationResult::fail(
 *                   gmFlow::ValidationError::RULE_VIOLATION,
 *                   "Action blocked by rule: " + engine.last_reason());
 *           return gmFlow::ValidationResult::ok();
 *       },
 *       // post-hook: record result
 *       [&log](const gmFlow::FlowRulesPayload& p,
 *              const gmFlow::ActionResult&     r) {
 *           log.record(p.action_id, r.succeeded());
 *       });
 *
 *   session.submit_action("player_1", std::move(wrapped));
 * @endcode
 *
 * @see FlowRulesGateway
 * @see FlowRulesPayload
 */

#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/bridges/FlowRulesGateway.hpp"

#include <functional>
#include <memory>

namespace gmFlow {

// ── Callback types ─────────────────────────────────────────────────────────────

/**
 * @brief Pre-check callback: called inside `validate()` after the inner action validates.
 *
 * Return `ValidationResult::ok()` to allow the action or
 * `ValidationResult::fail(RULE_VIOLATION, reason)` to block it.
 * If this callback is `nullptr` no pre-check is performed.
 */
using ActionPreCheck = std::function<ValidationResult(const FlowRulesPayload&)>;

/**
 * @brief Post-hook callback: called inside `execute()` after the inner action completes.
 *
 * Receives the payload and the `ActionResult` from the inner action.
 * The return value is ignored; this hook is purely informational.
 * If this callback is `nullptr` no post-hook is invoked.
 */
using ActionPostHook = std::function<void(const FlowRulesPayload&,
                                          const ActionResult&)>;

// ── ActionGateway ──────────────────────────────────────────────────────────────

/**
 * @class ActionGateway
 * @brief IAction decorator with rules-engine pre-check and post-hook.
 *
 * All `IAction` interface methods are delegated to the wrapped inner action.
 * Only `validate()` and `execute()` add additional behaviour.
 *
 * @note Thread safety: same as the wrapped inner action (typically none).
 */
class ActionGateway : public IAction
{
public:
	/**
	 * @brief Constructs an ActionGateway wrapping @p inner.
	 *
	 * @param inner      The real action to wrap; must not be null.
	 * @param payload    Base payload snapshot (actor_id, phase_id, etc.) to
	 *                   include in all callback invocations.  The `action_id`
	 *                   and `event_type` fields are overwritten at runtime.
	 * @param pre_check  Optional pre-validation callback.  Null = no pre-check.
	 * @param post_hook  Optional post-execution callback.  Null = no post-hook.
	 * @throws std::invalid_argument if @p inner is null.
	 */
	ActionGateway(std::unique_ptr<IAction> inner,
	              FlowRulesPayload         payload,
	              ActionPreCheck           pre_check,
	              ActionPostHook           post_hook);

	// ── IAction overrides ─────────────────────────────────────────────────

	/**
	 * @brief Validates the inner action, then runs the pre-check callback.
	 *
	 * Validation is two-stage:
	 * 1. `_inner->validate(ctx)` — returns fail immediately if it fails.
	 * 2. `_pre_check(_payload)` — returns its result (may be fail).
	 * If `_pre_check` is null, stage 2 is skipped and the inner result is returned.
	 *
	 * @param ctx Read-only session context.
	 * @return First failing ValidationResult encountered, or ok() if both pass.
	 */
	ValidationResult validate(const GameContext& ctx) const override;

	/**
	 * @brief Executes the inner action, then invokes the post-hook.
	 *
	 * The post-hook is called unconditionally (success or failure) and cannot
	 * alter the returned `ActionResult`.
	 *
	 * @param ctx Mutable session context.
	 * @return The result from the inner action's `execute()`.
	 */
	ActionResult execute(GameContext& ctx) override;

	/// @brief Delegates to the inner action.
	ActionId     id()           const override;

	/// @brief Delegates to the inner action.
	ActorId      owner()        const override;

	/// @brief Delegates to the inner action.
	ActionStatus status()       const override;

	/// @brief Delegates to the inner action.
	bool         is_async()     const override;

	/// @brief Delegates to the inner action.
	bool         requires_turn()const override;

	/// @brief Delegates to the inner action.
	bool         is_multi_step()const override;

private:
	std::unique_ptr<IAction> _inner;
	FlowRulesPayload         _payload;
	ActionPreCheck           _pre_check;
	ActionPostHook           _post_hook;
};

} // namespace gmFlow

#endif // GMFLOW_ACTIONGATEWAY_HPP
