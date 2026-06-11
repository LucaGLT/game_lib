#ifndef GMDISPATCH_LOGDISPATCHBRIDGE_HPP
#define GMDISPATCH_LOGDISPATCHBRIDGE_HPP

/**
 * @file bridges/LogDispatchBridge.hpp
 * @brief Adapter that feeds GmLog log records into the GmDispatch event bus.
 *
 * ### Overview
 * LogDispatchBridge implements @c GmLog::ILogDispatcher so it can be plugged
 * directly into any @c GmLog::Logger as its dispatcher.  Every log record
 * that reaches the bridge is converted to a @ref GmDispatch::Envelope and
 * dispatched on the provided @ref GmDispatch::Dispatcher.
 *
 * ### Field mapping
 * | LogRecord field          | Envelope field          | Notes                          |
 * |--------------------------|-------------------------|--------------------------------|
 * | @c record.timestamp      | @c env.timestamp        | Direct copy                    |
 * | @c record.loggerName     | @c env.source           | e.g. @c "Database"             |
 * | @c "log." + levelString  | @c env.typeId           | e.g. @c "log.ERROR"            |
 * | @c record.message        | @c env.payload          | stored as @c std::string       |
 * | @c record.function       | @c env.messageId        | empty when source loc disabled |
 * | @c {}                    | @c env.targets          | always broadcast               |
 *
 * ### Enabling the bridge
 * @code
 *   // Create the dispatch bus
 *   GmDispatch::Dispatcher bus =
 *       GmDispatch::DispatcherFactory::createSyncDispatcher("GameBus");
 *
 *   // Subscribe a handler for all log events
 *   std::shared_ptr<GmDispatch::EventBusChannel> logCh =
 *       std::make_shared<GmDispatch::EventBusChannel>("LogListener");
 *   logCh->addHandler([](const GmDispatch::Envelope& env) {
 *       std::string msg = std::any_cast<std::string>(env.payload);
 *       // forward to UI, remote monitor, etc.
 *   });
 *   bus.subscribe("log.*", logCh);  // requires PatternRouter
 *
 *   // Wire the bridge as the gmLog dispatcher
 *   GmLog::Logger db(
 *       cfg,
 *       std::make_unique<GmDispatch::LogDispatchBridge>(bus));
 *
 *   db.info("Connected");   // → Envelope{typeId="log.INFO", payload="Connected"}
 * @endcode
 *
 * ### No modification to gmLog
 * Both gmLog and gmDispatch remain unchanged.  The bridge lives in
 * @c gmDispatch/bridges/ and depends on both libraries.  gmLog does not
 * depend on gmDispatch.
 */

#include "../Dispatcher.hpp"
#include "../../gmLog/ILogDispatcher.hpp"
#include "../../gmLog/LogRecord.hpp"

namespace GmDispatch {

/**
 * @brief Implements @c GmLog::ILogDispatcher and forwards records to a
 *        @ref GmDispatch::Dispatcher.
 */
class LogDispatchBridge : public GmLog::ILogDispatcher {
public:
    /**
     * @brief Constructs a bridge that dispatches to @p bus.
     *
     * @param bus Reference to a @ref Dispatcher.  The Dispatcher must outlive
     *            this bridge.  Use @c std::shared_ptr if lifetime is unclear.
     */
    explicit LogDispatchBridge(Dispatcher& bus);

    /**
     * @brief Converts @p record to an @ref Envelope and dispatches it.
     *
     * Mapping:
     * - @c env.typeId   = @c "log." + levelToString(record.level)
     * - @c env.source   = @c record.loggerName
     * - @c env.payload  = @c record.message (as @c std::string)
     * - @c env.messageId = @c record.function (empty when not set)
     * - @c env.timestamp = @c record.timestamp
     *
     * @param record The log event to forward.
     */
    void dispatch(const GmLog::LogRecord& record) override;

    /**
     * @brief Flushes the underlying @ref Dispatcher.
     */
    void flush() override;

private:
    Dispatcher& bus_;
};

} // namespace GmDispatch

#endif // GMDISPATCH_LOGDISPATCHBRIDGE_HPP
