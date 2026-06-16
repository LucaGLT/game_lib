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
// ECampaignError
// ─────────────────────────────────────────────────────────────────────────────

ECampaignError::ECampaignError(const std::string& message)
    : std::runtime_error("Campaign: " + message)
{}

// ─────────────────────────────────────────────────────────────────────────────
// CampaignState
// ─────────────────────────────────────────────────────────────────────────────

void CampaignState::mark_completed(const SessionId& session_id, bool victory)
{
    _results[session_id] = {true, victory};
}

bool CampaignState::is_completed(const SessionId& session_id) const
{
    const auto it = _results.find(session_id);
    return it != _results.end() && it->second.completed;
}

bool CampaignState::is_victory(const SessionId& session_id) const
{
    const auto it = _results.find(session_id);
    return it != _results.end() && it->second.victory;
}

void CampaignState::unlock(const SessionId& session_id)
{
    _unlocked.insert(session_id);
}

bool CampaignState::is_unlocked(const SessionId& session_id) const
{
    return _unlocked.count(session_id) > 0;
}

void CampaignState::set_data(const std::string& key, std::string value)
{
    _data[key] = std::move(value);
}

std::string CampaignState::get_data(const std::string& key,
                                    const std::string& default_val) const
{
    const auto it = _data.find(key);
    return it != _data.end() ? it->second : default_val;
}

bool CampaignState::has_data(const std::string& key) const
{
    return _data.count(key) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Campaign
// ─────────────────────────────────────────────────────────────────────────────

Campaign::Campaign(std::vector<SessionDefinition> definitions)
    : _sessions(std::move(definitions))
{
    if (_sessions.empty()) {
        throw ECampaignError("session definitions list must not be empty");
    }

    // Unlock sessions flagged as initial.
    for (const SessionDefinition& def : _sessions) {
        if (def.initial_unlock) {
            _state.unlock(def.session_id);
        }
    }
}

void Campaign::set_event_callback(EventCallback callback)
{
    _event_callback = std::move(callback);
}

const SessionDefinition& Campaign::start_session(const SessionId& session_id)
{
    if (!_state.is_unlocked(session_id)) {
        throw ECampaignError("session '" + session_id + "' is not unlocked");
    }

    const auto it = std::find_if(_sessions.begin(), _sessions.end(),
        [&](const SessionDefinition& d) { return d.session_id == session_id; });
    if (it == _sessions.end()) {
        throw ECampaignError("session '" + session_id + "' not found in definitions");
    }

    _current_session_id = session_id;
    return *it;
}

void Campaign::complete_current_session(bool victory)
{
    if (!_current_session_id.has_value()) {
        throw ECampaignError("complete_current_session() called with no active session");
    }

    const SessionId sid = _current_session_id.value();
    _state.mark_completed(sid, victory);
    _current_session_id.reset();

    // TODO: Phase 4.8 — log completion via gmLog
    evaluate_unlocks();

    if (is_complete()) {
        if (_event_callback) {
            _event_callback(EVT_CAMPAIGN_COMPLETED, "");
        }
    }
}

bool Campaign::is_complete() const
{
    return std::all_of(_sessions.begin(), _sessions.end(),
        [&](const SessionDefinition& def) {
            return _state.is_completed(def.session_id);
        });
}

const CampaignState& Campaign::state() const { return _state; }
CampaignState&       Campaign::state()       { return _state; }

const std::vector<SessionDefinition>& Campaign::sessions() const { return _sessions; }

std::optional<SessionId> Campaign::current_session_id() const
{
    return _current_session_id;
}

void Campaign::evaluate_unlocks()
{
    for (const SessionDefinition& def : _sessions) {
        if (_state.is_unlocked(def.session_id)) continue;

        const bool prereqs_met = std::all_of(
            def.unlock_requires.begin(), def.unlock_requires.end(),
            [&](const SessionId& req) { return _state.is_completed(req); });

        if (prereqs_met) {
            _state.unlock(def.session_id);
            if (_event_callback) {
                _event_callback(EVT_CAMPAIGN_SESSION_UNLOCKED, def.session_id);
            }
        }
    }
}

} // namespace gmFlow
