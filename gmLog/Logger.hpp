#ifndef GMLOG_LOGGER_HPP
#define GMLOG_LOGGER_HPP

/**
 * @file Logger.hpp
 * @brief Main logger class — the primary application-facing entry point.
 */

#include "ILogDispatcher.hpp"
#include "LogLevel.hpp"
#include "LoggerConfig.hpp"
#include "LogRecord.hpp"

#include <memory>
#include <string>

namespace gmLog {

/**
 * @brief Core logger class.
 *
 * A GmLogger owns a @ref LoggerConfig (name, minimum level, source-location
 * flag) and delegates all I/O concerns to an @ref ILogDispatcher.  It is
 * intentionally ignorant of file paths, JSON formatting, and thread locking.
 *
 * GmLogger is **non-copyable** and **move-constructible**.
 *
 * ### Recommended construction (via factory)
 * @code
 *   auto db = gmLog::LoggerFactory::create_file_logger(
 *       "Database", "db.log", gmLog::LogLevel::DEBUG);
 *
 *   auto ui = gmLog::LoggerFactory::create_stdout_logger(
 *       "UI", gmLog::LogLevel::WARNING);
 * @endcode
 *
 * ### Direct construction
 * @code
 *   gmLog::LoggerConfig cfg;
 *   cfg.name     = "Network";
 *   cfg.min_level = gmLog::LogLevel::INFO;
 *
 *   gmLog::GmLogger net(
 *       cfg,
 *       std::make_unique<gmLog::SyncDispatcher>(
 *           std::make_unique<gmLog::FileSink>("network.log"),
 *           std::make_unique<gmLog::JsonFormatter>()
 *       )
 *   );
 * @endcode
 *
 * ### Logging via macros (recommended — includes source location)
 * @code
 *   logInfo (db, "Connection established");
 *   logErr(db, "Query failed: " + query);
 * @endcode
 *
 * ### Logging via convenience methods (no source location)
 * @code
 *   db.info ("Connection established");
 *   db.error("Query failed");
 * @endcode
 *
 * ### Lazy evaluation (avoids expensive string construction when level is off)
 * @code
 *   db.debug([&]{ return "row count = " + std::to_string(count); });
 * @endcode
 */
class GmLogger {
public:
	/**
	 * @brief Constructs a GmLogger with a given configuration and dispatcher.
	 *
	 * @param config     Identity and filtering parameters.
	 * @param dispatcher Ownership-transferred dispatcher (sync or future async).
	 */
	GmLogger(LoggerConfig                    config,
		   std::unique_ptr<ILogDispatcher> dispatcher);

	GmLogger(const GmLogger&)            = delete;
	GmLogger& operator=(const GmLogger&) = delete;

	GmLogger(GmLogger&&)                 = default;
	GmLogger& operator=(GmLogger&&)      = default;

	/**
	 * @brief Destructor — flushes the dispatcher before releasing resources.
	 */
	~GmLogger();

	// ── Configuration ─────────────────────────────────────────────────────────

	/**
	 * @brief Returns the logger's name.
	 * @return Const reference to the name string (e.g. @c "Database").
	 */
	const std::string& name() const;

	/**
	 * @brief Returns the current minimum log level.
	 * @return The minimum @ref LogLevel accepted; events below it are dropped.
	 */
	LogLevel min_level() const;

	/**
	 * @brief Updates the minimum log level at runtime.
	 * @param level New minimum level.
	 */
	void set_level(LogLevel level);

	/**
	 * @brief Returns @c true if @p level would be accepted by this logger.
	 * @param level Level to test.
	 * @return @c true when @p level >= @ref min_level().
	 */
	bool is_enabled(LogLevel level) const;

	// ── Core log method ───────────────────────────────────────────────────────

	/**
	 * @brief Creates a @ref LogRecord and dispatches it.
	 *
	 * This is the single method that all convenience methods and macros
	 * ultimately call.  It is @c public so that the @c LOG_* macros can
	 * inject source-location information.
	 *
	 * @param level    Severity of the event.
	 * @param message  Human-readable event description.
	 * @param file     Source file (from @c __FILE__); pass @c nullptr to omit.
	 * @param line     Source line (from @c __LINE__); pass @c 0 to omit.
	 * @param function Function name (from @c __func__); pass @c nullptr to omit.
	 */
	void log(LogLevel           level,
			 const std::string& message,
			 const char*        file     = nullptr,
			 int                line     = 0,
			 const char*        function = nullptr);

	// ── Convenience methods (string overloads) ────────────────────────────────

	/** @brief Logs at @ref LogLevel::DEBUG.    @param message Event message. */
	void debug(const std::string& message);

	/** @brief Logs at @ref LogLevel::INFO.     @param message Event message. */
	void info(const std::string& message);

	/** @brief Logs at @ref LogLevel::WARNING.  @param message Event message. */
	void warning(const std::string& message);

	/** @brief Logs at @ref LogLevel::ERROR.    @param message Event message. */
	void error(const std::string& message);

	/**
	 * @brief Logs at @ref LogLevel::CRITICAL and flushes immediately.
	 * @param message Event message.
	 */
	void critical(const std::string& message);

	// ── Lazy-evaluation overloads ─────────────────────────────────────────────

	/**
	 * @brief Logs at @ref LogLevel::DEBUG using a lazy message factory.
	 *
	 * The factory is invoked only when @ref LogLevel::DEBUG is enabled,
	 * avoiding the cost of string construction when debug output is off.
	 *
	 * @tparam F Callable with signature @c std::string().
	 * @param factory Lambda or functor returning the message string.
	 *
	 * @code
	 *   logger.debug([&]{ return "count = " + std::to_string(n); });
	 * @endcode
	 */
	template <typename F>
	void debug(F&& factory)
	{
		if (is_enabled(LogLevel::DEBUG))
			log(LogLevel::DEBUG, factory());
	}

	/**
	 * @brief Logs at @ref LogLevel::INFO using a lazy message factory.
	 * @tparam F Callable with signature @c std::string().
	 * @param factory Lambda or functor returning the message string.
	 */
	template <typename F>
	void info(F&& factory)
	{
		if (is_enabled(LogLevel::INFO))
			log(LogLevel::INFO, factory());
	}

	/**
	 * @brief Logs at @ref LogLevel::WARNING using a lazy message factory.
	 * @tparam F Callable with signature @c std::string().
	 * @param factory Lambda or functor returning the message string.
	 */
	template <typename F>
	void warning(F&& factory)
	{
		if (is_enabled(LogLevel::WARNING))
			log(LogLevel::WARNING, factory());
	}

	/**
	 * @brief Logs at @ref LogLevel::ERROR using a lazy message factory.
	 * @tparam F Callable with signature @c std::string().
	 * @param factory Lambda or functor returning the message string.
	 */
	template <typename F>
	void error(F&& factory)
	{
		if (is_enabled(LogLevel::ERROR))
			log(LogLevel::ERROR, factory());
	}

	/**
	 * @brief Logs at @ref LogLevel::CRITICAL using a lazy message factory.
	 *
	 * The dispatcher is flushed immediately after the record is written to
	 * minimise data loss on fatal events.
	 *
	 * @tparam F Callable with signature @c std::string().
	 * @param factory Lambda or functor returning the message string.
	 */
	template <typename F>
	void critical(F&& factory)
	{
		if (is_enabled(LogLevel::CRITICAL)) {
			log(LogLevel::CRITICAL, factory());
			flush();
		}
	}

	// ── Flush ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Flushes the dispatcher and its underlying sink.
	 *
	 * Call this before application exit to ensure all buffered records reach
	 * the output channel.
	 */
	void flush();

private:
	LoggerConfig                    _config;      ///< Name, level, and source-location flag.
	std::unique_ptr<ILogDispatcher> _dispatcher;  ///< Owned dispatcher (sync or future async).
};

} // namespace gmLog

#endif // GMLOG_LOGGER_HPP
