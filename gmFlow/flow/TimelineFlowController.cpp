/**
 * @file flow/TimelineFlowController.cpp
 * @brief Implementation of gmFlow::TimelineFlowController.
 */

#include "gmFlow/flow/TimelineFlowController.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/TimelineEvents.hpp"

#include <algorithm>
#include <stdexcept>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TimelineFlowController::TimelineFlowController(
    std::unique_ptr<ITimelineAdapter> adapter,
    TimelinePolicy policy)
    : adapter_(std::move(adapter))
    , policy_(policy)
{
	if (!adapter_) {
		throw std::invalid_argument(
		    "TimelineFlowController: adapter must not be null");
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// IFlowController — start
// ─────────────────────────────────────────────────────────────────────────────

void TimelineFlowController::start(GameContext& ctx)
{
	current_time_ = compute_current_time(ctx);
	select_next_actor(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
// IFlowController — process
// ─────────────────────────────────────────────────────────────────────────────

void TimelineFlowController::process(GameContext& ctx)
{
	if (adapter_->is_session_complete(ctx)) {
		return;
	}

	if (current_window_.has_value() && !current_window_->is_closed()) {
		return;
	}

	if (!active_actor_.has_value() && policy_.auto_select_next_actor) {
		select_next_actor(ctx);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// IFlowController — can_actor_act
// ─────────────────────────────────────────────────────────────────────────────

bool TimelineFlowController::can_actor_act(const GameContext& ctx,
                                           const ActorId& actor) const
{
	if (!adapter_->is_actor_enabled(ctx, actor)) {
		return false;
	}

	if (current_window_.has_value() &&
	    !current_window_->is_closed() &&
	    current_window_->can_submit(actor)) {
		return true;
	}

	return active_actor_.has_value() && active_actor_.value() == actor;
}

// ─────────────────────────────────────────────────────────────────────────────
// IFlowController — on_action_completed
// ─────────────────────────────────────────────────────────────────────────────

void TimelineFlowController::on_action_completed(GameContext& ctx,
                                                 const ActionResult& result)
{
	if (!result.succeeded()) {
		if (current_window_.has_value()) {
			current_window_->force_close();
		}
		return;
	}

	const TimelineValue old_time = current_time_;
	const TimelineValue new_time = compute_current_time(ctx);

	if (new_time > old_time) {
		current_time_ = new_time;
		adapter_->on_time_advanced(ctx, old_time, new_time);
		publish_time_advanced(ctx, old_time, new_time);
	}

	// Retain control: keep the same actor and reopen main window if needed.
	if (active_actor_.has_value() &&
	    adapter_->actor_keeps_control(ctx, active_actor_.value())) {
		if (policy_.open_main_action_window &&
		    (!current_window_.has_value() || current_window_->is_closed())) {
			current_window_.emplace(
			    std::vector<ActorId>{active_actor_.value()},
			    CompletionPolicy::MANUAL_CLOSE);
		}
		return;
	}

	// Release control: clear actor and window, then auto-select if policy allows.
	if (current_window_.has_value()) {
		current_window_->force_close();
	}
	current_window_.reset();
	active_actor_.reset();

	if (policy_.auto_select_next_actor &&
	    !adapter_->is_session_complete(ctx)) {
		select_next_actor(ctx);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// IFlowController — is_session_complete
// ─────────────────────────────────────────────────────────────────────────────

ValidationResult TimelineFlowController::accept_action(
    GameContext& /*ctx*/,
    const ActorId& actor,
    std::unique_ptr<IAction> action)
{
	if (!current_window_.has_value() || current_window_->is_closed()) {
		return ValidationResult::fail(
		    ValidationError::ACTION_WINDOW_CLOSED,
		    "No active action window for actor '" + actor + "'.");
	}
	return current_window_->submit(actor, std::move(action));
}

bool TimelineFlowController::is_session_complete(const GameContext& ctx) const
{
	return adapter_->is_session_complete(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

const std::optional<ActorId>& TimelineFlowController::active_actor() const
{
	return active_actor_;
}

TimelineValue TimelineFlowController::current_time() const
{
	return current_time_;
}

bool TimelineFlowController::has_action_window() const
{
	return current_window_.has_value() && !current_window_->is_closed();
}

void TimelineFlowController::force_close_action_window()
{
	if (current_window_.has_value()) {
		current_window_->force_close();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reaction window
// ─────────────────────────────────────────────────────────────────────────────

void TimelineFlowController::open_reaction_window(
    std::vector<ActorId> eligible_actors,
    CompletionPolicy policy)
{
	if (!policy_.allow_reactions) {
		return;
	}
	current_window_.emplace(std::move(eligible_actors), policy);
}

// ─────────────────────────────────────────────────────────────────────────────
// Protected helpers
// ─────────────────────────────────────────────────────────────────────────────

std::optional<ActorId>
TimelineFlowController::select_next_actor(GameContext& ctx)
{
	const std::vector<ActorId> actors = sorted_enabled_actors(ctx);

	if (actors.empty()) {
		active_actor_.reset();
		publish_no_actor_available(ctx);
		return std::nullopt;
	}

	const ActorId& selected     = actors.front();
	active_actor_               = selected;
	const TimelineValue sel_pos = adapter_->timeline_position(ctx, selected);

	// Detect tie: all enabled actors sharing the lowest position.
	std::vector<ActorId> tied;
	for (const ActorId& id : actors) {
		if (adapter_->timeline_position(ctx, id) == sel_pos) {
			tied.push_back(id);
		}
	}
	if (tied.size() > 1u) {
		publish_tie_detected(ctx, tied, sel_pos);
	}

	adapter_->on_actor_selected(ctx, selected);
	publish_actor_selected(ctx, selected, sel_pos);

	if (policy_.open_main_action_window) {
		current_window_.emplace(
		    std::vector<ActorId>{selected},
		    CompletionPolicy::MANUAL_CLOSE);
	}

	return selected;
}

// ─────────────────────────────────────────────────────────────────────────────

std::vector<ActorId>
TimelineFlowController::sorted_enabled_actors(const GameContext& ctx) const
{
	const std::vector<ActorId> all = adapter_->timeline_actors(ctx);

	// Build index list for stable sort.
	std::vector<std::size_t> indices;
	indices.reserve(all.size());
	for (std::size_t i = 0u; i < all.size(); ++i) {
		if (adapter_->is_actor_enabled(ctx, all[i])) {
			indices.push_back(i);
		}
	}

	if (indices.empty()) {
		return {};
	}

	// Stable sort: preserves original insertion order for equal keys.
	std::stable_sort(indices.begin(), indices.end(),
	    [&](std::size_t a, std::size_t b) {
	        const TimelineValue pa = adapter_->timeline_position(ctx, all[a]);
	        const TimelineValue pb = adapter_->timeline_position(ctx, all[b]);
	        if (pa != pb) return pa < pb;
	        const int ra = adapter_->tie_break_rank(ctx, all[a]);
	        const int rb = adapter_->tie_break_rank(ctx, all[b]);
	        if (ra != rb) return ra < rb;
	        // For stable_tie_break: original index order is preserved by
	        // std::stable_sort when keys are equal.
	        return false;
	    });

	std::vector<ActorId> result;
	result.reserve(indices.size());
	for (std::size_t i : indices) {
		result.push_back(all[i]);
	}
	return result;
}

// ─────────────────────────────────────────────────────────────────────────────

TimelineValue
TimelineFlowController::compute_current_time(const GameContext& ctx) const
{
	const std::vector<ActorId> all = adapter_->timeline_actors(ctx);
	bool found = false;
	TimelineValue min_val = 0;

	for (const ActorId& id : all) {
		if (!adapter_->is_actor_enabled(ctx, id)) {
			continue;
		}
		const TimelineValue pos = adapter_->timeline_position(ctx, id);
		if (!found || pos < min_val) {
			min_val = pos;
			found   = true;
		}
	}

	return found ? min_val : current_time_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Event publishing
// ─────────────────────────────────────────────────────────────────────────────

void TimelineFlowController::publish_actor_selected(GameContext& ctx,
                                                    const ActorId& actor,
                                                    TimelineValue position) const
{
	if (!policy_.publish_timeline_events) return;

	TimelineActorSelectedEvent ev;
	ev.actor_id         = actor;
	ev.timeline_position = position;
	ctx.event_bus().publish(ev);
}

void TimelineFlowController::publish_time_advanced(GameContext& ctx,
                                                   TimelineValue old_time,
                                                   TimelineValue new_time) const
{
	if (!policy_.publish_timeline_events) return;

	TimelineTimeAdvancedEvent ev;
	ev.old_time = old_time;
	ev.new_time = new_time;
	ctx.event_bus().publish(ev);
}

void TimelineFlowController::publish_tie_detected(
    GameContext& ctx,
    const std::vector<ActorId>& tied,
    TimelineValue position) const
{
	if (!policy_.publish_timeline_events) return;

	TimelineTieDetectedEvent ev;
	ev.tied_actors      = tied;
	ev.timeline_position = position;
	ctx.event_bus().publish(ev);
}

void TimelineFlowController::publish_no_actor_available(GameContext& ctx) const
{
	if (!policy_.publish_timeline_events) return;

	TimelineNoActorAvailableEvent ev;
	ctx.event_bus().publish(ev);
}

} // namespace gmFlow
