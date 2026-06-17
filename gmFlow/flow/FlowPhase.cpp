/**
 * @file flow/FlowPhase.cpp
 * @brief Implementation of gmFlow::FlowPhase.
 */

#include "gmFlow/flow/FlowPhase.hpp"
#include "gmFlow/flow/SequentialFlowController.hpp"

#include <stdexcept>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────

FlowPhase::FlowPhase(std::string                      scope_prefix,
                     std::unique_ptr<IFlowController> controller)
    : _scope_prefix(std::move(scope_prefix))
    , _controller(std::move(controller))
{
    if (_scope_prefix.empty())
    {
        throw std::invalid_argument(
            "FlowPhase: scope_prefix must not be empty");
    }
    if (!_controller)
    {
        throw std::invalid_argument(
            "FlowPhase: controller must not be null");
    }
}

// ── IPhase interface ──────────────────────────────────────────────────────────

PhaseId FlowPhase::id() const
{
    return _scope_prefix;
}

void FlowPhase::on_enter(GameContext& parent_ctx)
{
    _phase_ctx.emplace(parent_ctx, _scope_prefix);
    _controller->start(*_phase_ctx);
    _entered = true;
}

void FlowPhase::on_exit(GameContext& /*ctx*/)
{
    _entered = false;
    // _phase_ctx remains valid for post-exit inspection; reset on next on_enter().
}

bool FlowPhase::is_complete(const GameContext& /*ctx*/) const
{
    if (!_entered || !_phase_ctx.has_value())
        return false;
    return _controller->is_session_complete(*_phase_ctx);
}

std::vector<std::unique_ptr<IAction>>
FlowPhase::available_actions(const GameContext& /*ctx*/,
                             const ActorId&     actor) const
{
    if (!_entered || !_phase_ctx.has_value())
        return {};

    // Requires the inner controller to be a SequentialFlowController to
    // retrieve the currently active inner phase.
    const SequentialFlowController* seq =
        dynamic_cast<const SequentialFlowController*>(_controller.get());
    if (!seq)
        return {};

    IPhase* inner = seq->current_phase();
    if (!inner)
        return {};

    return inner->available_actions(*_phase_ctx, actor);
}

// ── FlowPhase-specific API ─────────────────────────────────────────────────────

void FlowPhase::tick(GameContext& /*ctx*/)
{
    if (!_entered || !_phase_ctx.has_value())
        return;
    _controller->process(*_phase_ctx);
}

ValidationResult FlowPhase::accept_action(GameContext&             /*ctx*/,
                                          const ActorId&           actor,
                                          std::unique_ptr<IAction> action)
{
    if (!_entered || !_phase_ctx.has_value())
    {
        return ValidationResult::fail(
            ValidationError::ACTION_WINDOW_CLOSED,
            "FlowPhase is not active.");
    }
    return _controller->accept_action(*_phase_ctx, actor, std::move(action));
}

bool FlowPhase::can_actor_act(const GameContext& /*ctx*/,
                              const ActorId&     actor) const
{
    if (!_entered || !_phase_ctx.has_value())
        return false;
    return _controller->can_actor_act(*_phase_ctx, actor);
}

const PhaseContext& FlowPhase::phase_context() const
{
    if (!_phase_ctx.has_value())
    {
        throw std::logic_error(
            "FlowPhase::phase_context() called before on_enter()");
    }
    return *_phase_ctx;
}

const std::string& FlowPhase::scope_prefix() const
{
    return _scope_prefix;
}

} // namespace gmFlow
