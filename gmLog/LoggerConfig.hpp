#ifndef GMLOG_LOGGERCONFIG_HPP
#define GMLOG_LOGGERCONFIG_HPP

/**
 * @file LoggerConfig.hpp
 * @brief Configuration parameters for a single GmLogger instance.
 */

#include "LogLevel.hpp"

#include <string>

namespace gmLog {

/**
 * @brief Holds the static configuration of a @ref GmLogger.
 *
 * Pass a populated LoggerConfig to the @ref GmLogger constructor (or to a
 * @ref LoggerFactory helper) to define the logger's identity and filtering
 * behaviour without coupling configuration concerns to the GmLogger
 * implementation.
 *
 * Future extensions (async-queue size, file-rotation settings, flush-on-error
 * flag, etc.) should be added here as optional fields rather than as
 * additional GmLogger constructor parameters.
 */
struct LoggerConfig {
	/// Human-readable logger name; appears in every emitted log line.
	std::string name;

	/// Minimum severity level accepted; events below this level are silently dropped.
	LogLevel    min_level             = LogLevel::DEBUG;

	/// When @c true, @c __FILE__ / @c __LINE__ / @c __func__ are attached to records.
	bool        enable_source_location = true;
};

} // namespace gmLog

#endif // GMLOG_LOGGERCONFIG_HPP
