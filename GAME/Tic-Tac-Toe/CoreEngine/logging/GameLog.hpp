#ifndef GMTRIS_GAMELOG_HPP
#define GMTRIS_GAMELOG_HPP

/**
 * @file logging/GameLog.hpp
 * @brief Thin wrapper over gmLog used to record match events.
 */

#include "gmLog/Logger.hpp"

#include <string>

namespace gmTris
{

/**
 * @class GameLog
 * @brief Records match lifecycle and move events through a gmLog logger.
 */
class GameLog
{
  public:
	/// @brief Creates a stdout-backed logger named "Tris".
	GameLog();

	/// @brief Logs an informational message.
	void info(const std::string& message);

	/// @brief Logs a warning message (e.g. an invalid move).
	void warn(const std::string& message);

  private:
	gmLog::GmLogger _logger;
};

} // namespace gmTris

#endif // GMTRIS_GAMELOG_HPP
