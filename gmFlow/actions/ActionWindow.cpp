/**
 * @file actions/ActionWindow.cpp
 * @brief Implementation of gmFlow::ActionWindow.
 */

#include "gmFlow/actions/ActionWindow.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/FlowEvents.hpp"

#include <algorithm>
#include <stdexcept>

namespace gmFlow {

ActionWindow::ActionWindow(std::vector<ActorId> eligible_actors,
                           CompletionPolicy     policy)
    : _eligible_actors(std::move(eligible_actors))
    , _policy(policy)
    , _closed(false)
{}

bool ActionWindow::can_submit(const ActorId& actor_id) const
{
    if (_closed) return false;

    // Actor must be in the eligible list.
    const bool eligible = std::find(
        _eligible_actors.begin(), _eligible_actors.end(), actor_id)
        != _eligible_actors.end();
    if (!eligible) return false;

    // Actor must not have already submitted (one submission per actor per window).
    const bool already_submitted = std::any_of(
        _submissions.begin(), _submissions.end(),
        [&](const Submission& s) { return s.actor_id == actor_id; });
    return !already_submitted;
}

ValidationResult ActionWindow::submit(const ActorId& actor_id,
                                      std::unique_ptr<IAction> action)
{
    if (!can_submit(actor_id)) {
        return ValidationResult::fail(
            ValidationError::ACTION_WINDOW_CLOSED,
            "Actor '" + actor_id + "' cannot submit to this window.");
    }

    // TODO: Phase 4.4 — log the submission via gmLog
    Submission s;
    s.actor_id = actor_id;
    s.action   = std::move(action);
    _submissions.push_back(std::move(s));
    return ValidationResult::ok();
}

void ActionWindow::pass(const ActorId& actor_id)
{
    const bool eligible = std::find(
        _eligible_actors.begin(), _eligible_actors.end(), actor_id)
        != _eligible_actors.end();
    if (!eligible) return;

    const bool already_passed = std::find(
        _passed_actors.begin(), _passed_actors.end(), actor_id)
        != _passed_actors.end();
    if (!already_passed) {
        _passed_actors.push_back(actor_id);
    }
}

bool ActionWindow::is_complete(const GameContext& /*ctx*/) const
{
    if (_closed) return true;

    // TODO: Phase 4.4 — implement per-policy completion checks
    switch (_policy) {
        case CompletionPolicy::ALL_SUBMITTED:
            return _submissions.size() >= _eligible_actors.size();

        case CompletionPolicy::ANY_SUBMITTED:
            return !_submissions.empty();

        case CompletionPolicy::MANUAL_CLOSE:
            return false;  // Only closes via force_close().

        case CompletionPolicy::UNTIL_ALL_PASSED:
            return _passed_actors.size() >= _eligible_actors.size();

        case CompletionPolicy::PRIORITY_RESOLVED:
            return _submissions.size() >= _eligible_actors.size();
    }
    return false;
}

void ActionWindow::resolve(GameContext& ctx)
{
    if (!is_complete(ctx)) {
        throw std::runtime_error("ActionWindow::resolve(): window is not yet complete");
    }
    _closed = true;

    // Execute all stored actions in submission order.
    // PRIORITY_RESOLVED: a future version should sort submissions by priority here.
    for (Submission& sub : _submissions) {
        if (!sub.action) continue;

        ActionStartedEvent asev;
        asev.action_id = sub.action->id();
        asev.actor_id  = sub.actor_id;
        ctx.event_bus().publish(asev);

        // Safety re-validation before execute.
        const ValidationResult vr = sub.action->validate(ctx);
        if (!vr.valid()) {
            ActionFailedEvent afev;
            afev.action_id = sub.action->id();
            afev.actor_id  = sub.actor_id;
            afev.reason    = vr.message();
            ctx.event_bus().publish(afev);
            continue;
        }

        const ActionResult result = sub.action->execute(ctx);

        if (result.succeeded()) {
            ActionCompletedEvent acev;
            acev.action_id = sub.action->id();
            acev.actor_id  = sub.actor_id;
            ctx.event_bus().publish(acev);
        } else {
            ActionFailedEvent afev;
            afev.action_id = sub.action->id();
            afev.actor_id  = sub.actor_id;
            afev.reason    = result.reason();
            ctx.event_bus().publish(afev);
        }
    }
    _submissions.clear();

    WindowClosedEvent wcev;
    ctx.event_bus().publish(wcev);
}

void ActionWindow::force_close()
{
    _closed = true;
}

bool ActionWindow::is_closed() const
{
    return _closed;
}

const std::vector<ActorId>& ActionWindow::eligible_actors() const
{
    return _eligible_actors;
}

std::size_t ActionWindow::submission_count() const
{
    return _submissions.size();
}

} // namespace gmFlow
