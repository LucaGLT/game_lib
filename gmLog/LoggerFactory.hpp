#ifndef GMLOG_LOGGERFACTORY_HPP
#define GMLOG_LOGGERFACTORY_HPP

/**
 * @file LoggerFactory.hpp
 * @brief Convenience factory for creating pre-configured Logger instances.
 */

#include "Logger.hpp"
#include "LogLevel.hpp"

#include <string>

namespace GmLog {

/**
 * @brief Static factory that creates ready-to-use @ref Logger instances.
 *
 * LoggerFactory reduces the construction boilerplate required to assemble a
 * Logger with its dispatcher, formatter, and sink.  All factory methods
 * return a fully-constructed, move-only Logger.
 *
 * @par Example
 * @code
 *   auto db = GmLog::LoggerFactory::createFileLogger(
 *       "Database", "database.log", GmLog::LogLevel::Debug);
 *
 *   auto ui = GmLog::LoggerFactory::createStdoutLogger(
 *       "UI", GmLog::LogLevel::Warning);
 * @endcode
 *
 * @note This is a static-only utility class; it cannot be instantiated.
 *
 * ### Extending the factory
 * Add new static methods here when new sink or dispatcher types are
 * introduced.  The @ref Logger class itself does not need to change.
 */
class LoggerFactory {
public:
    LoggerFactory()  = delete;
    ~LoggerFactory() = delete;

    /**
     * @brief Creates a Logger that writes to standard output.
     *
     * Assembles: @ref SyncDispatcher → @ref StdoutSink + @ref JsonFormatter.
     *
     * @param name                 Logger name (appears in every log line).
     * @param level                Minimum accepted log level.
     * @param enableSourceLocation Attach @c __FILE__ / @c __LINE__ / @c __func__.
     * @return A fully-configured, move-only @ref Logger.
     */
    static Logger createStdoutLogger(
        const std::string& name,
        LogLevel           level                = LogLevel::Debug,
        bool               enableSourceLocation = true);

    /**
     * @brief Creates a Logger that appends to a file on disk.
     *
     * Assembles: @ref SyncDispatcher → @ref FileSink + @ref JsonFormatter.
     *
     * @param name                 Logger name (appears in every log line).
     * @param filePath             Path to the log file (created or appended to).
     * @param level                Minimum accepted log level.
     * @param enableSourceLocation Attach @c __FILE__ / @c __LINE__ / @c __func__.
     * @return A fully-configured, move-only @ref Logger.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static Logger createFileLogger(
        const std::string& name,
        const std::string& filePath,
        LogLevel           level                = LogLevel::Debug,
        bool               enableSourceLocation = true);
};

} // namespace GmLog

#endif // GMLOG_LOGGERFACTORY_HPP
