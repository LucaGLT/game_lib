#ifndef GMLOG_SYNCDISPATCHER_HPP
#define GMLOG_SYNCDISPATCHER_HPP

/**
 * @file dispatchers/SyncDispatcher.hpp
 * @brief Thread-safe synchronous log dispatcher (V1).
 */

#include "../ILogDispatcher.hpp"
#include "../ILogFormatter.hpp"
#include "../ILogSink.hpp"

#include <memory>
#include <mutex>

namespace GmLog {

/**
 * @brief Thread-safe synchronous log dispatcher.
 *
 * SyncDispatcher is the V1 concrete implementation of @ref ILogDispatcher.
 * On each @ref dispatch() call it:
 *
 * 1. Acquires an internal @c std::mutex.
 * 2. Calls @c ILogFormatter::format() to convert the record to a string.
 * 3. Calls @c ILogSink::write() to deliver the string to the output channel.
 *
 * The mutex is held for the entire format + write sequence, guaranteeing that
 * two threads writing to the same logger will never interleave their output.
 *
 * ### Migrating to async
 * When asynchronous logging is needed, replace SyncDispatcher with a future
 * @c AsyncDispatcher.  Because @ref Logger holds a
 * @c std::unique_ptr<ILogDispatcher>, the change is confined to the
 * construction site (or the @ref LoggerFactory helper).
 */
class SyncDispatcher : public ILogDispatcher {
public:
    /**
     * @brief Constructs a SyncDispatcher with the given sink and formatter.
     *
     * @param sink      Unique-ownership pointer to the output sink.
     * @param formatter Unique-ownership pointer to the record formatter.
     */
    SyncDispatcher(std::unique_ptr<ILogSink>       sink,
                   std::unique_ptr<ILogFormatter>  formatter);

    ~SyncDispatcher() = default;

    /**
     * @brief Formats @p record and writes it to the sink under mutex protection.
     *
     * This call blocks on the calling thread until @c ILogSink::write()
     * returns.
     *
     * @param record The log event to dispatch.
     */
    void dispatch(const LogRecord& record) override;

    /**
     * @brief Flushes the underlying sink under mutex protection.
     */
    void flush() override;

private:
    std::mutex                    mutex_;      ///< Serialises format + write across threads.
    std::unique_ptr<ILogSink>      sink_;      ///< Owned output channel.
    std::unique_ptr<ILogFormatter> formatter_; ///< Owned record-to-string converter.
};

} // namespace GmLog

#endif // GMLOG_SYNCDISPATCHER_HPP
