#ifndef GMLOG_JSONFORMATTER_HPP
#define GMLOG_JSONFORMATTER_HPP

/**
 * @file formatters/JsonFormatter.hpp
 * @brief JSON Lines formatter for the GmLog library.
 */

#include "../ILogFormatter.hpp"

#include <chrono>
#include <string>

namespace GmLog {

/**
 * @brief Formats a log record as a single-line JSON object (JSON Lines format).
 *
 * Each @ref format() call produces exactly one JSON object on a single line
 * with no trailing newline.  The owning @ref ILogSink appends the line
 * terminator.
 *
 * ### Minimal output (source location absent)
 * @code{.json}
 * {"time":"2026-06-10T17:42:11.235","logger":"Database","level":"INFO","message":"Ready"}
 * @endcode
 *
 * ### Extended output (source location present)
 * @code{.json}
 * {"time":"2026-06-10T17:42:11.235","logger":"Database","level":"ERROR","file":"db.cpp","line":87,"function":"connect","message":"Connection failed"}
 * @endcode
 *
 * ### String escaping
 * All string fields are escaped via @ref escapeJsonString to correctly handle
 * embedded quotes, backslashes, and control characters.
 *
 * ### Extending output formats
 * To add plain-text or CSV output, implement @ref ILogFormatter in a new
 * class — no existing code needs to change.
 */
class JsonFormatter : public ILogFormatter {
public:
    JsonFormatter()  = default;
    ~JsonFormatter() = default;

    /**
     * @brief Formats @p record as a JSON Lines object.
     * @param record The log event to format.
     * @return Single-line JSON string without a trailing newline.
     */
    std::string format(const LogRecord& record) override;

    /**
     * @brief Escapes a string value for safe embedding inside a JSON string.
     *
     * Characters replaced:
     * | Input   | Output   |
     * |---|---|
     * | @c \\   | @c \\\\  |
     * | @c "    | @c \\"   |
     * | @c \\n  | @c \\n   |
     * | @c \\r  | @c \\r   |
     * | @c \\t  | @c \\t   |
     * | @c < 0x20 (other) | @c \\uXXXX |
     *
     * @param value Raw string to escape.
     * @return JSON-safe string (without surrounding quotes).
     */
    static std::string escapeJsonString(const std::string& value);

private:
    /**
     * @brief Formats a time point as an ISO-8601 UTC string with milliseconds.
     *
     * Output pattern: @c YYYY-MM-DDTHH:MM:SS.mmm
     *
     * @param tp Time point to format.
     * @return Formatted timestamp string.
     */
    static std::string formatTimestamp(
        const std::chrono::system_clock::time_point& tp);
};

} // namespace GmLog

#endif // GMLOG_JSONFORMATTER_HPP
