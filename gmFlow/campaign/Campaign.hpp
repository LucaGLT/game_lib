#ifndef GMFLOW_CAMPAIGN_HPP
#define GMFLOW_CAMPAIGN_HPP

/**
 * @file campaign/Campaign.hpp
 * @brief Orchestrates a sequence of game sessions with persistent state.
 *
 * Campaign manages the progression from one scenario to the next.  It:
 * - Holds all @ref SessionDefinition objects (the campaign script).
 * - Tracks @ref CampaignState (completed sessions, unlocked sessions, data).
 * - Evaluates unlock conditions after each session completes.
 * - Publishes @ref EVT_CAMPAIGN_SESSION_UNLOCKED and @ref EVT_CAMPAIGN_COMPLETED
 *   events via a lightweight event callback.
 *
 * ### Typical usage
 * @code
 *   // 1. Define all sessions.
 *   std::vector<gmFlow::SessionDefinition> defs = build_campaign_script();
 *
 *   // 2. Create the campaign.
 *   gmFlow::Campaign campaign(std::move(defs));
 *
 *   // 3. Start the first unlocked session.
 *   const gmFlow::SessionDefinition& def = campaign.start_session("scenario_1");
 *
 *   // … run the game session …
 *
 *   // 4. Record the outcome.
 *   campaign.complete_current_session(true);   // true = victory
 * @endcode
 */

#include "gmFlow/campaign/CampaignState.hpp"
#include "gmFlow/campaign/SessionDefinition.hpp"
#include "gmFlow/core/Ids.hpp"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gmFlow {

/**
 * @class CampaignError
 * @brief Exception thrown for invalid campaign operations.
 */
class CampaignError : public std::runtime_error {
public:
    /// @brief Constructs the error with a descriptive message.
    explicit CampaignError(const std::string& message);
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class Campaign
 * @brief Manages a scripted sequence of sessions with persistent progress.
 *
 * Campaign is non-copyable.  One Campaign instance exists for the lifetime
 * of a campaign run; it is serialized/deserialized via gmSave between play
 * sessions.
 */
class Campaign {
public:
    /// @brief Callback type for campaign-level notifications.
    using EventCallback = std::function<void(const std::string& event_type,
                                             const SessionId&   session_id)>;

    /**
     * @brief Constructs a Campaign from the given session definitions.
     *
     * Definitions are stored in the order provided; the Campaign does not
     * reorder them.  Initial sessions (where `initial_unlock == true`) are
     * unlocked automatically.
     *
     * @param definitions  Ordered list of session definitions.  Must not be empty.
     * @throws CampaignError if definitions is empty.
     */
    explicit Campaign(std::vector<SessionDefinition> definitions);

    /**
     * @brief Registers a callback invoked on campaign-level events.
     *
     * @param callback Callable with signature `void(event_type, session_id)`.
     *                 event_type is one of the `EVT_CAMPAIGN_*` constants from
     *                 EventType.hpp.
     */
    void set_event_callback(EventCallback callback);

    /**
     * @brief Marks the given session as the current active session.
     *
     * @param session_id ID of the session to start.
     * @return Const reference to the matching SessionDefinition.
     * @throws CampaignError if the session is not found or not yet unlocked.
     */
    const SessionDefinition& start_session(const SessionId& session_id);

    /**
     * @brief Records the outcome of the current active session and evaluates unlocks.
     *
     * After recording the result, Campaign evaluates every locked session's
     * `unlock_requires` list.  Any session whose prerequisites are now all
     * completed is unlocked; the callback is invoked with
     * `EVT_CAMPAIGN_SESSION_UNLOCKED`.
     *
     * If all sessions are completed, the callback is invoked with
     * `EVT_CAMPAIGN_COMPLETED`.
     *
     * @param victory true if the players won; false for a loss/draw.
     * @throws CampaignError if no session is currently active.
     */
    void complete_current_session(bool victory);

    /**
     * @brief Returns true if all sessions have been completed.
     * @return true if every SessionDefinition's session_id is in the completed set.
     */
    bool is_complete() const;

    /// @brief Returns a const reference to the current campaign state.
    const CampaignState& state() const;

    /// @brief Returns a mutable reference to the campaign state (for gmSave restore).
    CampaignState& state();

    /// @brief Returns all session definitions in the campaign.
    const std::vector<SessionDefinition>& sessions() const;

    /**
     * @brief Returns the ID of the currently active session, if any.
     * @return The active SessionId, or an empty optional if no session is active.
     */
    std::optional<SessionId> current_session_id() const;

private:
    /// @brief Evaluates unlock conditions for all locked sessions.
    void evaluate_unlocks();

    std::vector<SessionDefinition>  sessions_;
    CampaignState                   state_;
    std::optional<SessionId>        current_session_id_;
    EventCallback                   event_callback_;
};

} // namespace gmFlow

#endif // GMFLOW_CAMPAIGN_HPP
