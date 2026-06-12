#ifndef GMDISPATCH_LOGDISPATCHBRIDGE_HPP
#define GMDISPATCH_LOGDISPATCHBRIDGE_HPP

/**
 * @file bridges/LogDispatchBridge.hpp
 * @brief Adapter that feeds GmLog log records into the gmDispatch event bus.
 *
 * ### Overview
 * LogDispatchBridge implements @c gmLog::ILogDispatcher so it can be plugged
 * directly into any @c gmLog::GmLogger as its dispatcher.  Every log record
 * that reaches the bridge is converted to a @ref gmDispatch::Envelope and
 * dispatched on the provided @ref gmDispatch::GmDispatcher.
 *
 * ### Field mapping
 * | LogRecord field          | Envelope field          | Notes                          |
 * |--------------------------|-------------------------|--------------------------------|
 * | @c record.timestamp      | @c env.timestamp        | Direct copy                    |
 * | @c record.logger_name     | @c env.source           | e.g. @c "Database"             |
 * | @c "log." + levelString  | @c env.typeId           | e.g. @c "log.ERROR"            |
 * | @c record.message        | @c env.payload          | stored as @c std::string       |
 * | @c record.function       | @c env.messageId        | empty when source loc disabled |
 * | @c {}                    | @c env.targets          | always broadcast               |
 *
 * ### Enabling the bridge
 * @code
 *   // Create the dispatch bus
 *   gmDispatch::GmDispatcher bus =
 *       gmDispatch::DispatcherFactory::create_sync_dispatcher("GameBus");
 *
 *   // Subscribe a handler for all log events
 *   std::shared_ptr<gmDispatch::EventBusChannel> logCh =
 *       std::make_shared<gmDispatch::EventBusChannel>("LogListener");
 *   logCh->add_handler([](const gmDispatch::Envelope& env) {
 *       std::string msg = std::any_cast<std::string>(env.payload);
 *       // forward to UI, remote monitor, etc.
 *   });
 *   bus.subscribe("log.*", logCh);  // requires PatternRouter
 *
 *   // Wire the bridge as the gmLog dispatcher
 *   gmLog::GmLogger db(
 *       cfg,
 *       std::make_unique<gmDispatch::LogDispatchBridge>(bus));
 *
 *   db.info("Connected");   // → Envelope{typeId="log.INFO", payload="Connected"}
 * @endcode
 *
 * ### No modification to gmLog
 * Both gmLog and gmDispatch remain unchanged.  The bridge lives in
 * @c gmDispatch/bridges/ and depends on both libraries.  gmLog does not
 * depend on gmDispatch.
 */

#include "Dispatcher.hpp"
#include "../gmLog/ILogDispatcher.hpp"
#include "../gmLog/LogRecord.hpp"

namespace gmDispatch {

/**
 * @brief Implements @c gmLog::ILogDispatcher and forwards records to a
 *        @ref gmDispatch::GmDispatcher.
 */
class LogDispatchBridge : public gmLog::ILogDispatcher {
public:
	/**
	 * @brief Constructs a bridge that dispatches to @p bus.
	 *
	 * @param bus Reference to a @ref GmDispatcher.  The GmDispatcher must outlive
	 *            this bridge.  Use @c std::shared_ptr if lifetime is unclear.
	 */
	explicit LogDispatchBridge(GmDispatcher& bus);

	/**
	 * @brief Converts @p record to an @ref Envelope and dispatches it.
	 *
	 * Mapping:
	 * - @c env.typeId   = @c "log." + level_to_string(record.level)
	 * - @c env.source   = @c record.logger_name
	 * - @c env.payload  = @c record.message (as @c std::string)
	 * - @c env.messageId = @c record.function (empty when not set)
	 * - @c env.timestamp = @c record.timestamp
	 *
	 * @param record The log event to forward.
	 */
	void dispatch(const gmLog::LogRecord& record) override;

	/**
	 * @brief Flushes the underlying @ref GmDispatcher.
	 */
	void flush() override;

private:
	GmDispatcher& _bus;
};

} // namespace gmDispatch

#endif // GMDISPATCH_LOGDISPATCHBRIDGE_HPP
