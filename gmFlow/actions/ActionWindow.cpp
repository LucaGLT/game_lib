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
    : eligible_actors_(std::move(eligible_actors))
    , policy_(policy)
    , closed_(false)
{}

bool ActionWindow::can_submit(const ActorId& actor_id) const
{
    if (closed_) return false;

    // Actor must be in the eligible list.
    const bool eligible = std::find(
        eligible_actors_.begin(), eligible_actors_.end(), actor_id)
        != eligible_actors_.end();
    if (!eligible) return false;

    // Actor must not have already submitted (one submission per actor per window).
    const bool already_submitted = std::any_of(
        submissions_.begin(), submissions_.end(),
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
    submissions_.push_back(std::move(s));
    return ValidationResult::ok();
}

void ActionWindow::pass(const ActorId& actor_id)
{
    const bool eligible = std::find(
        eligible_actors_.begin(), eligible_actors_.end(), actor_id)
        != eligible_actors_.end();
    if (!eligible) return;

    const bool already_passed = std::find(
        passed_actors_.begin(), passed_actors_.end(), actor_id)
        != passed_actors_.end();
    if (!already_passed) {
        passed_actors_.push_back(actor_id);
    }
}

bool ActionWindow::is_complete(const GameContext& /*ctx*/) const
{
    if (closed_) return true;

    // TODO: Phase 4.4 — implement per-policy completion checks
    switch (policy_) {
        case CompletionPolicy::ALL_SUBMITTED:
            return submissions_.size() >= eligible_actors_.size();

        case CompletionPolicy::ANY_SUBMITTED:
            return !submissions_.empty();

        case CompletionPolicy::MANUAL_CLOSE:
            return false;  // Only closes via force_close().

        case CompletionPolicy::UNTIL_ALL_PASSED:
            return passed_actors_.size() >= eligible_actors_.size();

        case CompletionPolicy::PRIORITY_RESOLVED:
            return submissions_.size() >= eligible_actors_.size();
    }
    return false;
}

void ActionWindow::resolve(GameContext& ctx)
{
    if (!is_complete(ctx)) {
        throw std::runtime_error("ActionWindow::resolve(): window is not yet complete");
    }
    closed_ = true;

    // Execute all stored actions in submission order.
    // PRIORITY_RESOLVED: a future version should sort submissions by priority here.
    for (Submission& sub : submissions_) {
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
    submissions_.clear();

    WindowClosedEvent wcev;
    ctx.event_bus().publish(wcev);
}

void ActionWindow::force_close()
{
    closed_ = true;
}

bool ActionWindow::is_closed() const
{
    return closed_;
}

const std::vector<ActorId>& ActionWindow::eligible_actors() const
{
    return eligible_actors_;
}

std::size_t ActionWindow::submission_count() const
{
    return submissions_.size();
}

} // namespace gmFlow
