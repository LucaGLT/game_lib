#ifndef GMFLOW_CAMPAIGNSTATE_HPP
#define GMFLOW_CAMPAIGNSTATE_HPP

/**
 * @file campaign/CampaignState.hpp
 * @brief Serializable record of persistent campaign progress.
 *
 * CampaignState accumulates results across multiple sessions (scenarios) and
 * is the primary artefact persisted between play sessions via gmSave.
 *
 * It tracks:
 * - Which sessions have been completed (and their outcomes).
 * - Which sessions are currently unlocked (available to play).
 * - Application-defined persistent key/value data (e.g. hero XP, unlocked items).
 *
 * ### Serialization note
 * In Phase 4, `CampaignState` will implement gmSave's serialization interface
 * so that `Campaign::save()` can write it to disk.
 *
 * @code
 *   gmFlow::CampaignState s;
 *   s.mark_completed("scenario_1", true);
 *   s.unlock("scenario_2");
 *   s.set_data("barbarian_xp", "450");
 *   bool won = s.is_completed("scenario_1");
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace gmFlow {

/**
 * @class CampaignState
 * @brief Accumulates and persists cross-session campaign progress.
 */
class CampaignState {
public:
    CampaignState() = default;

    /**
     * @brief Marks a session as completed with a win/loss result.
     * @param session_id ID of the completed session.
     * @param victory    true if the players won; false if they lost.
     */
    void mark_completed(const SessionId& session_id, bool victory);

    /**
     * @brief Returns true if the given session has been completed.
     * @param session_id Session to query.
     * @return true if mark_completed() was called for this session.
     */
    bool is_completed(const SessionId& session_id) const;

    /**
     * @brief Returns true if the given session was completed with a victory.
     * @param session_id Session to query.
     * @return true if the session was completed with victory == true.
     */
    bool is_victory(const SessionId& session_id) const;

    /**
     * @brief Adds a session to the unlocked set (available to play).
     * @param session_id Session to unlock.
     */
    void unlock(const SessionId& session_id);

    /**
     * @brief Returns true if the given session is in the unlocked set.
     * @param session_id Session to query.
     * @return true if unlock() was called for this session.
     */
    bool is_unlocked(const SessionId& session_id) const;

    /**
     * @brief Stores a named persistent data value.
     *
     * Used for game-specific cross-session state (hero XP, unlocked items, etc.).
     * Values are stored as strings; callers convert to/from their actual type.
     *
     * @param key   Unique data key.
     * @param value String-encoded value to store.
     */
    void set_data(const std::string& key, std::string value);

    /**
     * @brief Retrieves a named persistent data value.
     * @param key          Key to look up.
     * @param default_val  Value returned if the key does not exist.
     * @return Stored value, or `default_val` if not found.
     */
    std::string get_data(const std::string& key,
                         const std::string& default_val = "") const;

    /**
     * @brief Returns true if the persistent data map contains the given key.
     * @param key Key to check.
     */
    bool has_data(const std::string& key) const;

private:
    struct SessionResult {
        bool completed = false;
        bool victory   = false;
    };

    std::unordered_map<SessionId, SessionResult> _results;
    std::unordered_set<SessionId>                _unlocked;
    std::unordered_map<std::string, std::string> _data;
};

} // namespace gmFlow

#endif // GMFLOW_CAMPAIGNSTATE_HPP
