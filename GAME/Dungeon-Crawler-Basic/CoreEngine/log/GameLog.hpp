#ifndef GMDUNGEONBASIC_GAMELOG_HPP
#define GMDUNGEONBASIC_GAMELOG_HPP

/**
 * @file log/GameLog.hpp
 * @brief Structured session logger for the dungeon engine.
 *
 * GameLog wraps a @c gmLog::Logger to emit structured log records for every
 * significant game event. Log records are written at runtime to the configured
 * sinks (console, file). They are separate from the GUI event stream and are
 * intended for debugging and post-session analysis.
 *
 * @note Not thread-safe.
 */

#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Dungeon session logger backed by gmLog.
 *
 * Provides a small set of domain-level methods that map game events to
 * structured log messages. Underlying format and sink are configured in
 * FASE B. In FASE A all methods are stubs.
 */
class GameLog
{
public:
	/// @brief Constructs the logger and initialises gmLog internals.
	GameLog();

	/**
	 * @brief Logs the start of a dungeon session.
	 *
	 * @param session_id   Unique session identifier string.
	 * @param map_file     Path of the loaded map file.
	 */
	void log_session_start(const std::string& session_id,
	                       const std::string& map_file);

	/**
	 * @brief Logs an action performed by an actor.
	 *
	 * @param actor_id    Actor performing the action.
	 * @param action      Action name (e.g. "move", "heal", "equip").
	 * @param detail      Optional detail string (e.g. destination room id).
	 */
	void log_action(const std::string& actor_id,
	                const std::string& action,
	                const std::string& detail = "");

	/**
	 * @brief Logs a rejected action with its reason.
	 *
	 * @param actor_id  Actor whose action was rejected.
	 * @param command   Command typeId that was rejected.
	 * @param reason    Human-readable rejection reason.
	 */
	void log_rejection(const std::string& actor_id,
	                   const std::string& command,
	                   const std::string& reason);

	/**
	 * @brief Logs the end of a dungeon session with its outcome.
	 *
	 * @param outcome  String representation of GameOutcome.
	 */
	void log_session_end(const std::string& outcome);

	/**
	 * @brief Logs a generic informational message.
	 *
	 * @param message  Free-text message string.
	 */
	void log_info(const std::string& message);
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_GAMELOG_HPP
