#ifndef GMLOG_LOGGERFACTORY_HPP
#define GMLOG_LOGGERFACTORY_HPP

/**
 * @file LoggerFactory.hpp
 * @brief Convenience factory for creating pre-configured GmLogger instances.
 */

#include "Logger.hpp"
#include "LogLevel.hpp"

#include <string>

namespace gmLog {

/**
 * @brief Static factory that creates ready-to-use @ref GmLogger instances.
 *
 * LoggerFactory reduces the construction boilerplate required to assemble a
 * GmLogger with its dispatcher, formatter, and sink.  All factory methods
 * return a fully-constructed, move-only GmLogger.
 *
 * @par Example
 * @code
 *   auto db = gmLog::LoggerFactory::create_file_logger(
 *       "Database", "database.log", gmLog::LogLevel::DEBUG);
 *
 *   auto ui = gmLog::LoggerFactory::create_stdout_logger(
 *       "UI", gmLog::LogLevel::WARNING);
 * @endcode
 *
 * @note This is a static-only utility class; it cannot be instantiated.
 *
 * ### Extending the factory
 * Add new static methods here when new sink or dispatcher types are
 * introduced.  The @ref GmLogger class itself does not need to change.
 */
class LoggerFactory {
public:
	LoggerFactory()  = delete;
	~LoggerFactory() = delete;

	/**
	 * @brief Creates a GmLogger that writes to standard output.
	 *
	 * Assembles: @ref SyncDispatcher → @ref StdoutSink + @ref JsonFormatter.
	 *
	 * @param name                 GmLogger name (appears in every log line).
	 * @param level                Minimum accepted log level.
	 * @param enable_source_location Attach @c __FILE__ / @c __LINE__ / @c __func__.
	 * @return A fully-configured, move-only @ref GmLogger.
	 */
	static GmLogger create_stdout_logger(
		const std::string& name,
		LogLevel           level                = LogLevel::DEBUG,
		bool               enable_source_location = true);

	/**
	 * @brief Creates a GmLogger that appends to a file on disk.
	 *
	 * Assembles: @ref SyncDispatcher → @ref FileSink + @ref JsonFormatter.
	 *
	 * @param name                 GmLogger name (appears in every log line).
	 * @param filePath             Path to the log file (created or appended to).
	 * @param level                Minimum accepted log level.
	 * @param enable_source_location Attach @c __FILE__ / @c __LINE__ / @c __func__.
	 * @return A fully-configured, move-only @ref GmLogger.
	 * @throws ELogError if the file cannot be opened.
	 */
	static GmLogger create_file_logger(
		const std::string& name,
		const std::string& filePath,
		LogLevel           level                = LogLevel::DEBUG,
		bool               enable_source_location = true);
};

} // namespace gmLog

#endif // GMLOG_LOGGERFACTORY_HPP
