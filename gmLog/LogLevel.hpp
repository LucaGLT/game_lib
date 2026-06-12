#ifndef GMLOG_LOGLEVEL_HPP
#define GMLOG_LOGLEVEL_HPP

/**
 * @file LogLevel.hpp
 * @brief Log severity level enumeration and utility functions.
 */

#include <string>

namespace gmLog {

/**
 * @brief Log severity level.
 *
 * Levels are ordered numerically so that minimum-level filtering is a simple
 * integer comparison:
 * @code
 *   if (record.level < config.min_level) return;
 * @endcode
 *
 * Typical use:
 * @code
 *   logger.set_level(gmLog::LogLevel::WARNING);
 *   // Debug and Info are now suppressed; Warning, Error, Critical pass through.
 * @endcode
 */
enum class LogLevel : int {
	DEBUG    = 0, ///< Verbose diagnostic information; intended for development only.
	INFO     = 1, ///< General operational messages confirming normal behaviour.
	WARNING  = 2, ///< Unexpected but recoverable condition; something worth investigating.
	ERROR    = 3, ///< Failure in a specific operation; the application can continue.
	CRITICAL = 4, ///< Fatal condition; the application may not be able to continue.
	OFF      = 5  ///< Sentinel value: disables all logging when set as minimum level.
};

/**
 * @brief Converts a @ref LogLevel to its uppercase string label.
 *
 * @param level The level to convert.
 * @return One of: @c "DEBUG", @c "INFO", @c "WARNING", @c "ERROR",
 *         @c "CRITICAL", @c "OFF", or @c "UNKNOWN".
 */
const char* level_to_string(LogLevel level);

/**
 * @brief Parses a string into a @ref LogLevel.
 *
 * Comparison is case-insensitive.  Recognised tokens: @c "debug", @c "info",
 * @c "warning", @c "error", @c "critical", @c "off".
 *
 * @param str  Input string to parse.
 * @param out  Output level; written only on success.
 * @return @c true if @p str was recognised, @c false otherwise.
 */
bool level_from_string(const std::string& str, LogLevel& out);

} // namespace gmLog

#endif // GMLOG_LOGLEVEL_HPP
