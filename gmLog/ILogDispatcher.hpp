#ifndef GMLOG_ILOGDISPATCHER_HPP
#define GMLOG_ILOGDISPATCHER_HPP

/**
 * @file ILogDispatcher.hpp
 * @brief Abstract dispatcher interface — the key extension point for async logging.
 */

#include "LogRecord.hpp"

namespace gmLog {

/**
 * @brief Abstract base class for log dispatchers.
 *
 * A dispatcher receives a @ref LogRecord from the @ref GmLogger, formats it
 * via an @ref ILogFormatter, and delivers it to an @ref ILogSink.  This
 * abstraction decouples the GmLogger from the question of *when* and *how* the
 * record reaches the sink.
 *
 * ### V1 vs future
 * | Concrete type           | Behaviour                                          |
 * |---|---|
 * | @ref SyncDispatcher     | Formats and writes on the calling thread (V1).      |
 * | AsyncDispatcher (future)| Enqueues the record; a worker thread delivers it.   |
 *
 * The @ref GmLogger holds a @c std::unique_ptr<ILogDispatcher>, so switching
 * from synchronous to asynchronous requires only changing the object passed
 * at construction time — no GmLogger API changes are needed.
 *
 * ### Architecture diagram
 * @code
 *   GmLogger
 *     ↓  dispatch(LogRecord)
 *   ILogDispatcher
 *     ↓  format(record) → string
 *   ILogFormatter
 *     ↓  write(string)
 *   ILogSink
 * @endcode
 */
class ILogDispatcher {
public:
	virtual ~ILogDispatcher() = default;

	/**
	 * @brief Accepts a log record and delivers it to the configured sink.
	 *
	 * In @ref SyncDispatcher this call blocks until the record has been
	 * written.  In a future @c AsyncDispatcher it enqueues the record and
	 * returns immediately.
	 *
	 * @param record The log event to dispatch.
	 */
	virtual void dispatch(const LogRecord& record) = 0;

	/**
	 * @brief Flushes any buffered output to the underlying sink.
	 *
	 * Must be called before application shutdown to prevent data loss.  In an
	 * async dispatcher, this call may block until the internal queue is fully
	 * drained and written.
	 */
	virtual void flush() = 0;
};

} // namespace gmLog

#endif // GMLOG_ILOGDISPATCHER_HPP
