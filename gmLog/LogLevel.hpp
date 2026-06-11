#ifndef GMLOG_LOGLEVEL_HPP
#define GMLOG_LOGLEVEL_HPP

/**
 * @file LogLevel.hpp
 * @brief Log severity level enumeration and utility functions.
 */

#include <string>

namespace GmLog {

/**
 * @brief Log severity level.
 *
 * Levels are ordered numerically so that minimum-level filtering is a simple
 * integer comparison:
 * @code
 *   if (record.level < config.minLevel) return;
 * @endcode
 *
 * Typical use:
 * @code
 *   logger.setLevel(GmLog::LogLevel::Warning);
 *   // Debug and Info are now suppressed; Warning, Error, Critical pass through.
 * @endcode
 */
enum class LogLevel : int {
    Debug    = 0, ///< Verbose diagnostic information; intended for development only.
    Info     = 1, ///< General operational messages confirming normal behaviour.
    Warning  = 2, ///< Unexpected but recoverable condition; something worth investigating.
    Error    = 3, ///< Failure in a specific operation; the application can continue.
    Critical = 4, ///< Fatal condition; the application may not be able to continue.
    Off      = 5  ///< Sentinel value: disables all logging when set as minimum level.
};

/**
 * @brief Converts a @ref LogLevel to its uppercase string label.
 *
 * @param level The level to convert.
 * @return One of: @c "DEBUG", @c "INFO", @c "WARNING", @c "ERROR",
 *         @c "CRITICAL", @c "OFF", or @c "UNKNOWN".
 */
const char* levelToString(LogLevel level);

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
bool levelFromString(const std::string& str, LogLevel& out);

} // namespace GmLog

#endif // GMLOG_LOGLEVEL_HPP
