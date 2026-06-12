#ifndef GMLOG_LOGRECORD_HPP
#define GMLOG_LOGRECORD_HPP

/**
 * @file LogRecord.hpp
 * @brief Immutable log-event record passed through the gmLog pipeline.
 */

#include "LogLevel.hpp"

#include <chrono>
#include <string>

namespace gmLog {

/**
 * @brief Represents a single log event.
 *
 * A LogRecord is created by @ref GmLogger each time a message passes the
 * minimum-level filter.  It is then forwarded to the @ref ILogDispatcher,
 * which hands it to the @ref ILogFormatter and ultimately to the
 * @ref ILogSink.
 *
 * Source-location fields (@c file, @c line, @c function) are populated only
 * when the call originates from the @c LOG_* macros and
 * @c LoggerConfig::enable_source_location is @c true.
 */
struct LogRecord {
	/// Severity of the event.
	LogLevel    level       = LogLevel::DEBUG;

	/// Name of the originating logger (e.g. @c "Database").
	std::string logger_name;

	/// Human-readable event description.
	std::string message;

	/// Wall-clock timestamp captured at the moment the record is created.
	std::chrono::system_clock::time_point timestamp;

	/// Source file name from @c __FILE__; @c nullptr if unknown.
	const char* file     = nullptr;

	/// Source line number from @c __LINE__; @c 0 if unknown.
	int         line     = 0;

	/// Function name from @c __func__; @c nullptr if unknown.
	const char* function = nullptr;
};

} // namespace gmLog

#endif // GMLOG_LOGRECORD_HPP
