#ifndef GMLOG_ILOGSINK_HPP
#define GMLOG_ILOGSINK_HPP

/**
 * @file ILogSink.hpp
 * @brief Abstract output-channel interface for the GmLog library.
 */

#include <string>

namespace GmLog {

/**
 * @brief Abstract base class for all log output channels (sinks).
 *
 * A sink receives a pre-formatted string (produced by an @ref ILogFormatter)
 * and writes it to the underlying output channel.  The channel may be
 * standard output, a file on disk, a serial port, a network socket, etc.
 *
 * The @ref Logger does not interact with sinks directly; it delegates to an
 * @ref ILogDispatcher, which owns the sink.
 *
 * ### Implementing a custom sink
 * @code
 *   class ConsoleSink : public GmLog::ILogSink {
 *   public:
 *       void write(const std::string& message) override { ... }
 *       void flush()                            override { ... }
 *   };
 * @endcode
 *
 * ### Thread safety
 * The V1 @ref SyncDispatcher serialises all calls to @c write() and
 * @c flush() via its internal mutex, so concrete sink implementations do not
 * need their own locking.  Future async dispatchers must provide the same
 * guarantee or document otherwise.
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;

    /**
     * @brief Writes a single, fully-formatted log line to the output channel.
     *
     * The message string already contains everything needed (timestamp, level,
     * logger name, source location, …).  The sink should simply deliver it to
     * the underlying channel without any additional formatting.
     *
     * @param message Pre-formatted log line (typically a JSON object string).
     */
    virtual void write(const std::string& message) = 0;

    /**
     * @brief Flushes any internal write buffers to the underlying channel.
     *
     * Must be called before application exit and whenever a
     * @ref LogLevel::Critical event is logged to prevent data loss.
     */
    virtual void flush() = 0;
};

} // namespace GmLog

#endif // GMLOG_ILOGSINK_HPP
