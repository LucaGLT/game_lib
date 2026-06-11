/**
 * @file campaign/Campaign.cpp
 * @brief Implementation of gmFlow::Campaign and CampaignState.
 */

#include "gmFlow/campaign/Campaign.hpp"
#include "gmFlow/events/EventType.hpp"

#include <algorithm>
#include <stdexcept>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// CampaignError
// ─────────────────────────────────────────────────────────────────────────────

CampaignError::CampaignError(const std::string& message)
    : std::runtime_error("Campaign: " + message)
{}

// ─────────────────────────────────────────────────────────────────────────────
// CampaignState
// ─────────────────────────────────────────────────────────────────────────────

void CampaignState::mark_completed(const SessionId& session_id, bool victory)
{
    results_[session_id] = {true, victory};
}

bool CampaignState::is_completed(const SessionId& session_id) const
{
    const auto it = results_.find(session_id);
    return it != results_.end() && it->second.completed;
}

bool CampaignState::is_victory(const SessionId& session_id) const
{
    const auto it = results_.find(session_id);
    return it != results_.end() && it->second.victory;
}

void CampaignState::unlock(const SessionId& session_id)
{
    unlocked_.insert(session_id);
}

bool CampaignState::is_unlocked(const SessionId& session_id) const
{
    return unlocked_.count(session_id) > 0;
}

void CampaignState::set_data(const std::string& key, std::string value)
{
    data_[key] = std::move(value);
}

std::string CampaignState::get_data(const std::string& key,
                                    const std::string& default_val) const
{
    const auto it = data_.find(key);
    return it != data_.end() ? it->second : default_val;
}

bool CampaignState::has_data(const std::string& key) const
{
    return data_.count(key) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Campaign
// ─────────────────────────────────────────────────────────────────────────────

Campaign::Campaign(std::vector<SessionDefinition> definitions)
    : sessions_(std::move(definitions))
{
    if (sessions_.empty()) {
        throw CampaignError("session definitions list must not be empty");
    }

    // Unlock sessions flagged as initial.
    for (const SessionDefinition& def : sessions_) {
        if (def.initial_unlock) {
            state_.unlock(def.session_id);
        }
    }
}

void Campaign::set_event_callback(EventCallback callback)
{
    event_callback_ = std::move(callback);
}

const SessionDefinition& Campaign::start_session(const SessionId& session_id)
{
    if (!state_.is_unlocked(session_id)) {
        throw CampaignError("session '" + session_id + "' is not unlocked");
    }

    const auto it = std::find_if(sessions_.begin(), sessions_.end(),
        [&](const SessionDefinition& d) { return d.session_id == session_id; });
    if (it == sessions_.end()) {
        throw CampaignError("session '" + session_id + "' not found in definitions");
    }

    current_session_id_ = session_id;
    return *it;
}

void Campaign::complete_current_session(bool victory)
{
    if (!current_session_id_.has_value()) {
        throw CampaignError("complete_current_session() called with no active session");
    }

    const SessionId sid = current_session_id_.value();
    state_.mark_completed(sid, victory);
    current_session_id_.reset();

    // TODO: Phase 4.8 — log completion via gmLog
    evaluate_unlocks();

    if (is_complete()) {
        if (event_callback_) {
            event_callback_(EVT_CAMPAIGN_COMPLETED, "");
        }
    }
}

bool Campaign::is_complete() const
{
    return std::all_of(sessions_.begin(), sessions_.end(),
        [&](const SessionDefinition& def) {
            return state_.is_completed(def.session_id);
        });
}

const CampaignState& Campaign::state() const { return state_; }
CampaignState&       Campaign::state()       { return state_; }

const std::vector<SessionDefinition>& Campaign::sessions() const { return sessions_; }

std::optional<SessionId> Campaign::current_session_id() const
{
    return current_session_id_;
}

void Campaign::evaluate_unlocks()
{
    for (const SessionDefinition& def : sessions_) {
        if (state_.is_unlocked(def.session_id)) continue;

        const bool prereqs_met = std::all_of(
            def.unlock_requires.begin(), def.unlock_requires.end(),
            [&](const SessionId& req) { return state_.is_completed(req); });

        if (prereqs_met) {
            state_.unlock(def.session_id);
            if (event_callback_) {
                event_callback_(EVT_CAMPAIGN_SESSION_UNLOCKED, def.session_id);
            }
        }
    }
}

} // namespace gmFlow
