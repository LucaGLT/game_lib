#ifndef GMLOG_LOGGERCONFIG_HPP
#define GMLOG_LOGGERCONFIG_HPP

/**
 * @file LoggerConfig.hpp
 * @brief Configuration parameters for a single Logger instance.
 */

#include "LogLevel.hpp"

#include <string>

namespace GmLog {

/**
 * @brief Holds the static configuration of a @ref Logger.
 *
 * Pass a populated LoggerConfig to the @ref Logger constructor (or to a
 * @ref LoggerFactory helper) to define the logger's identity and filtering
 * behaviour without coupling configuration concerns to the Logger
 * implementation.
 *
 * Future extensions (async-queue size, file-rotation settings, flush-on-error
 * flag, etc.) should be added here as optional fields rather than as
 * additional Logger constructor parameters.
 */
struct LoggerConfig {
    /// Human-readable logger name; appears in every emitted log line.
    std::string name;

    /// Minimum severity level accepted; events below this level are silently dropped.
    LogLevel    minLevel             = LogLevel::Debug;

    /// When @c true, @c __FILE__ / @c __LINE__ / @c __func__ are attached to records.
    bool        enableSourceLocation = true;
};

} // namespace GmLog

#endif // GMLOG_LOGGERCONFIG_HPP
