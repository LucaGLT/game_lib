#ifndef GMLOG_ILOGFORMATTER_HPP
#define GMLOG_ILOGFORMATTER_HPP

/**
 * @file ILogFormatter.hpp
 * @brief Abstract formatter interface for the gmLog library.
 */

#include "LogRecord.hpp"

#include <string>

namespace gmLog {

/**
 * @brief Abstract base class for log-record formatters.
 *
 * A formatter converts a @ref LogRecord into a string that is then passed
 * to an @ref ILogSink.  Separating formatting from output lets the same sink
 * support different text layouts (JSON Lines, plain text, CSV, …) without
 * any changes to the sink or the @ref GmLogger.
 *
 * ### V1 concrete implementation
 * @ref JsonFormatter — produces one JSON object per line (JSON Lines format).
 *
 * ### Implementing a custom formatter
 * @code
 *   class PlainTextFormatter : public gmLog::ILogFormatter {
 *   public:
 *       std::string format(const gmLog::LogRecord& record) override {
 *           return "[" + std::string(gmLog::level_to_string(record.level)) + "] "
 *                  + record.message;
 *       }
 *   };
 * @endcode
 */
class ILogFormatter {
public:
	virtual ~ILogFormatter() = default;

	/**
	 * @brief Converts a log record to its string representation.
	 *
	 * @param record The log event to format.
	 * @return Formatted string ready to be passed to an @ref ILogSink::write().
	 *         Implementations must NOT append a trailing newline; the sink is
	 *         responsible for line termination.
	 */
	virtual std::string format(const LogRecord& record) = 0;
};

} // namespace gmLog

#endif // GMLOG_ILOGFORMATTER_HPP
