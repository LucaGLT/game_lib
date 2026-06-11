/**
 * @file actions/ActionWindow.cpp
 * @brief Implementation of gmFlow::ActionWindow.
 */

#include "gmFlow/actions/ActionWindow.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/Result.hpp"

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
    // TODO: Phase 4.4 — validate actor is eligible before recording pass
    passed_actors_.push_back(actor_id);
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

    // TODO: Phase 4.4 — sort submissions by action priority, then call execute()
    //   on each action in order.  Emit EVT_WINDOW_CLOSED event via ctx.event_bus().
    (void)ctx;
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
