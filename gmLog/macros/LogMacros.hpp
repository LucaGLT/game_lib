#ifndef GMLOG_LOGMACROS_HPP
#define GMLOG_LOGMACROS_HPP

/**
 * @file macros/LogMacros.hpp
 * @brief Compile-time-filterable logging macros for the GmLog library.
 *
 * These macros are the **recommended** way to emit log messages from
 * application code.  They provide:
 *
 * - **Automatic source location** — @c __FILE__, @c __LINE__, @c __func__
 *   are injected into every record.
 * - **Runtime level guard** — the message expression is never evaluated when
 *   the level is disabled at runtime.
 * - **Compile-time level stripping** via @c LOG_COMPILED_LEVEL — call sites
 *   below the compiled level are replaced by empty @c do{}while(0) statements,
 *   producing zero binary overhead in release builds.
 *
 * ### Compile-time control
 * Set @c LOG_COMPILED_LEVEL via the build system or before including this
 * header:
 * @code
 *   // CMakeLists.txt:
 *   target_compile_definitions(myapp PRIVATE LOG_COMPILED_LEVEL=LOG_LEVEL_INFO)
 *
 *   // Or per translation unit:
 *   #define LOG_COMPILED_LEVEL LOG_LEVEL_INFO
 *   #include "macros/LogMacros.hpp"
 * @endcode
 * With @c LOG_LEVEL_INFO, every @c LOG_DEBUG call is compiled out entirely.
 *
 * ### Default
 * If @c LOG_COMPILED_LEVEL is not defined it defaults to @c LOG_LEVEL_DEBUG
 * (all levels active).
 *
 * ### Usage
 * @code
 *   #include "macros/LogMacros.hpp"
 *
 *   LOG_DEBUG   (logger, "Enter processRow()");
 *   LOG_INFO    (logger, "Server started on port " + std::to_string(port));
 *   LOG_WARNING (logger, "Retry attempt " + std::to_string(n));
 *   LOG_ERROR   (logger, "File not found: " + path);
 *   LOG_CRITICAL(logger, "Out of memory — aborting");
 * @endcode
 *
 * @note @p expr is an ordinary expression (not a lambda).  The macro wraps
 *       the runtime level check internally, so @p expr is only evaluated when
 *       the level is active at runtime.
 */

// ─── Numeric level constants ───────────────────────────────────────────────────

#define LOG_LEVEL_DEBUG    0 ///< Debug compile-time constant.
#define LOG_LEVEL_INFO     1 ///< Info compile-time constant.
#define LOG_LEVEL_WARNING  2 ///< Warning compile-time constant.
#define LOG_LEVEL_ERROR    3 ///< Error compile-time constant.
#define LOG_LEVEL_CRITICAL 4 ///< Critical compile-time constant.
#define LOG_LEVEL_OFF      5 ///< Disables all logging when set as compiled level.

// ─── Default compiled level ───────────────────────────────────────────────────

#ifndef LOG_COMPILED_LEVEL
    /**
     * @brief Compiled log level — override via build system or before including
     *        this header.  Defaults to @c LOG_LEVEL_DEBUG (all levels active).
     */
    #define LOG_COMPILED_LEVEL LOG_LEVEL_DEBUG
#endif

// ─── Internal helper ──────────────────────────────────────────────────────────

/// @cond INTERNAL
#define _GMLOG_DO_LOG(logger, gmLevel, expr)                                  \
    do {                                                                      \
        if ((logger).isEnabled(gmLevel)) {                                    \
            (logger).log((gmLevel), (expr), __FILE__, __LINE__, __func__);    \
        }                                                                     \
    } while (0)
/// @endcond

// ─── Public macros ────────────────────────────────────────────────────────────

#if LOG_COMPILED_LEVEL <= LOG_LEVEL_DEBUG
    /**
     * @brief Logs @p expr at @ref GmLog::LogLevel::Debug with source location.
     *
     * Compiled out entirely when @c LOG_COMPILED_LEVEL > @c LOG_LEVEL_DEBUG.
     *
     * @param logger A @ref GmLog::Logger lvalue.
     * @param expr   Expression returning @c std::string; evaluated lazily.
     */
    #define LOG_DEBUG(logger, expr) \
        _GMLOG_DO_LOG((logger), ::GmLog::LogLevel::Debug, (expr))
#else
    #define LOG_DEBUG(logger, expr) do {} while (0) ///< Compiled out.
#endif

#if LOG_COMPILED_LEVEL <= LOG_LEVEL_INFO
    /**
     * @brief Logs @p expr at @ref GmLog::LogLevel::Info with source location.
     *
     * Compiled out entirely when @c LOG_COMPILED_LEVEL > @c LOG_LEVEL_INFO.
     *
     * @param logger A @ref GmLog::Logger lvalue.
     * @param expr   Expression returning @c std::string; evaluated lazily.
     */
    #define LOG_INFO(logger, expr) \
        _GMLOG_DO_LOG((logger), ::GmLog::LogLevel::Info, (expr))
#else
    #define LOG_INFO(logger, expr) do {} while (0) ///< Compiled out.
#endif

#if LOG_COMPILED_LEVEL <= LOG_LEVEL_WARNING
    /**
     * @brief Logs @p expr at @ref GmLog::LogLevel::Warning with source location.
     *
     * Compiled out entirely when @c LOG_COMPILED_LEVEL > @c LOG_LEVEL_WARNING.
     *
     * @param logger A @ref GmLog::Logger lvalue.
     * @param expr   Expression returning @c std::string; evaluated lazily.
     */
    #define LOG_WARNING(logger, expr) \
        _GMLOG_DO_LOG((logger), ::GmLog::LogLevel::Warning, (expr))
#else
    #define LOG_WARNING(logger, expr) do {} while (0) ///< Compiled out.
#endif

#if LOG_COMPILED_LEVEL <= LOG_LEVEL_ERROR
    /**
     * @brief Logs @p expr at @ref GmLog::LogLevel::Error with source location.
     *
     * Compiled out entirely when @c LOG_COMPILED_LEVEL > @c LOG_LEVEL_ERROR.
     *
     * @param logger A @ref GmLog::Logger lvalue.
     * @param expr   Expression returning @c std::string; evaluated lazily.
     */
    #define LOG_ERROR(logger, expr) \
        _GMLOG_DO_LOG((logger), ::GmLog::LogLevel::Error, (expr))
#else
    #define LOG_ERROR(logger, expr) do {} while (0) ///< Compiled out.
#endif

#if LOG_COMPILED_LEVEL <= LOG_LEVEL_CRITICAL
    /**
     * @brief Logs @p expr at @ref GmLog::LogLevel::Critical with source location.
     *
     * After writing, the dispatcher is flushed immediately to minimise the
     * risk of data loss on fatal events.
     *
     * Compiled out entirely when @c LOG_COMPILED_LEVEL > @c LOG_LEVEL_CRITICAL.
     *
     * @param logger A @ref GmLog::Logger lvalue.
     * @param expr   Expression returning @c std::string; evaluated lazily.
     */
    #define LOG_CRITICAL(logger, expr)                                        \
        do {                                                                  \
            if ((logger).isEnabled(::GmLog::LogLevel::Critical)) {            \
                (logger).log(::GmLog::LogLevel::Critical, (expr),             \
                             __FILE__, __LINE__, __func__);                   \
                (logger).flush();                                             \
            }                                                                 \
        } while (0)
#else
    #define LOG_CRITICAL(logger, expr) do {} while (0) ///< Compiled out.
#endif

#endif // GMLOG_LOGMACROS_HPP
