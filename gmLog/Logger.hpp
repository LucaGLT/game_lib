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

namespace GmLog {

/**
 * @brief Core logger class.
 *
 * A Logger owns a @ref LoggerConfig (name, minimum level, source-location
 * flag) and delegates all I/O concerns to an @ref ILogDispatcher.  It is
 * intentionally ignorant of file paths, JSON formatting, and thread locking.
 *
 * Logger is **non-copyable** and **move-constructible**.
 *
 * ### Recommended construction (via factory)
 * @code
 *   auto db = GmLog::LoggerFactory::createFileLogger(
 *       "Database", "db.log", GmLog::LogLevel::Debug);
 *
 *   auto ui = GmLog::LoggerFactory::createStdoutLogger(
 *       "UI", GmLog::LogLevel::Warning);
 * @endcode
 *
 * ### Direct construction
 * @code
 *   GmLog::LoggerConfig cfg;
 *   cfg.name     = "Network";
 *   cfg.minLevel = GmLog::LogLevel::Info;
 *
 *   GmLog::Logger net(
 *       cfg,
 *       std::make_unique<GmLog::SyncDispatcher>(
 *           std::make_unique<GmLog::FileSink>("network.log"),
 *           std::make_unique<GmLog::JsonFormatter>()
 *       )
 *   );
 * @endcode
 *
 * ### Logging via macros (recommended — includes source location)
 * @code
 *   LOG_INFO (db, "Connection established");
 *   LOG_ERROR(db, "Query failed: " + query);
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
class Logger {
public:
    /**
     * @brief Constructs a Logger with a given configuration and dispatcher.
     *
     * @param config     Identity and filtering parameters.
     * @param dispatcher Ownership-transferred dispatcher (sync or future async).
     */
    Logger(LoggerConfig                    config,
           std::unique_ptr<ILogDispatcher> dispatcher);

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(Logger&&)                 = default;
    Logger& operator=(Logger&&)      = default;

    /**
     * @brief Destructor — flushes the dispatcher before releasing resources.
     */
    ~Logger();

    // ── Configuration ─────────────────────────────────────────────────────────

    /**
     * @brief Returns the logger's name.
     * @return Const reference to the name string (e.g. @c "Database").
     */
    const std::string& name() const;

    /**
     * @brief Returns the current minimum log level.
     */
    LogLevel minLevel() const;

    /**
     * @brief Updates the minimum log level at runtime.
     * @param level New minimum level.
     */
    void setLevel(LogLevel level);

    /**
     * @brief Returns @c true if @p level would be accepted by this logger.
     * @param level Level to test.
     * @return @c true when @p level >= @ref minLevel().
     */
    bool isEnabled(LogLevel level) const;

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

    /** @brief Logs at @ref LogLevel::Debug.    @param message Event message. */
    void debug(const std::string& message);

    /** @brief Logs at @ref LogLevel::Info.     @param message Event message. */
    void info(const std::string& message);

    /** @brief Logs at @ref LogLevel::Warning.  @param message Event message. */
    void warning(const std::string& message);

    /** @brief Logs at @ref LogLevel::Error.    @param message Event message. */
    void error(const std::string& message);

    /**
     * @brief Logs at @ref LogLevel::Critical and flushes immediately.
     * @param message Event message.
     */
    void critical(const std::string& message);

    // ── Lazy-evaluation overloads ─────────────────────────────────────────────

    /**
     * @brief Logs at @ref LogLevel::Debug using a lazy message factory.
     *
     * The factory is invoked only when @ref LogLevel::Debug is enabled,
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
        if (isEnabled(LogLevel::Debug))
            log(LogLevel::Debug, factory());
    }

    /**
     * @brief Logs at @ref LogLevel::Info using a lazy message factory.
     * @tparam F Callable with signature @c std::string().
     * @param factory Lambda or functor returning the message string.
     */
    template <typename F>
    void info(F&& factory)
    {
        if (isEnabled(LogLevel::Info))
            log(LogLevel::Info, factory());
    }

    /**
     * @brief Logs at @ref LogLevel::Warning using a lazy message factory.
     * @tparam F Callable with signature @c std::string().
     * @param factory Lambda or functor returning the message string.
     */
    template <typename F>
    void warning(F&& factory)
    {
        if (isEnabled(LogLevel::Warning))
            log(LogLevel::Warning, factory());
    }

    /**
     * @brief Logs at @ref LogLevel::Error using a lazy message factory.
     * @tparam F Callable with signature @c std::string().
     * @param factory Lambda or functor returning the message string.
     */
    template <typename F>
    void error(F&& factory)
    {
        if (isEnabled(LogLevel::Error))
            log(LogLevel::Error, factory());
    }

    /**
     * @brief Logs at @ref LogLevel::Critical using a lazy message factory.
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
        if (isEnabled(LogLevel::Critical)) {
            log(LogLevel::Critical, factory());
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
    LoggerConfig                    config_;      ///< Name, level, and source-location flag.
    std::unique_ptr<ILogDispatcher> dispatcher_;  ///< Owned dispatcher (sync or future async).
};

} // namespace GmLog

#endif // GMLOG_LOGGER_HPP
