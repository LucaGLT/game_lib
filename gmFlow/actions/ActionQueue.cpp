/**
 * @file actions/ActionQueue.cpp
 * @brief Implementation of gmFlow::ActionQueue.
 */

#include "gmFlow/actions/ActionQueue.hpp"

#include <algorithm>
#include <stdexcept>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Returns true when `a` has strictly higher priority than `b`.
static bool higher_priority(ActionPriority a, ActionPriority b)
{
    // Enum values are ordered: IMMEDIATE(0) > REACTION(1) > NORMAL(2) > DEFERRED(3)
    // Lower numeric value = higher priority.
    return static_cast<int>(a) < static_cast<int>(b);
}

// ─────────────────────────────────────────────────────────────────────────────
// ActionQueue
// ─────────────────────────────────────────────────────────────────────────────

void ActionQueue::push(std::unique_ptr<IAction> action, ActionPriority priority)
{
    // TODO: Phase 4.3 — log the enqueue event via gmLog
    Entry e;
    e.priority = priority;
    e.action   = std::move(action);

    // Insert before the first entry with a strictly lower priority to preserve FIFO.
    const auto it = std::find_if(entries_.begin(), entries_.end(),
        [&](const Entry& existing) {
            return higher_priority(priority, existing.priority);
        });
    entries_.insert(it, std::move(e));
}

IAction& ActionQueue::front()
{
    if (entries_.empty()) {
        throw std::runtime_error("ActionQueue::front(): queue is empty");
    }
    return *entries_.front().action;
}

const IAction& ActionQueue::front() const
{
    if (entries_.empty()) {
        throw std::runtime_error("ActionQueue::front(): queue is empty");
    }
    return *entries_.front().action;
}

void ActionQueue::pop()
{
    if (entries_.empty()) {
        throw std::runtime_error("ActionQueue::pop(): queue is empty");
    }
    entries_.erase(entries_.begin());
}

bool ActionQueue::empty() const
{
    return entries_.empty();
}

std::size_t ActionQueue::size() const
{
    return entries_.size();
}

void ActionQueue::clear()
{
    entries_.clear();
}

} // namespace gmFlow
