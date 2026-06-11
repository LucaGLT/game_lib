/**
 * @file bridges/LogDispatchBridge.cpp
 * @brief Implementation of LogDispatchBridge.
 */

#include "LogDispatchBridge.hpp"
#include "../../gmLog/LogLevel.hpp"

namespace GmDispatch {

LogDispatchBridge::LogDispatchBridge(Dispatcher& bus)
    : bus_(bus)
{}

void LogDispatchBridge::dispatch(const GmLog::LogRecord& record)
{
    Envelope env;

    env.typeId    = std::string("log.") + GmLog::levelToString(record.level);
    env.source    = record.loggerName;
    env.timestamp = record.timestamp;
    env.messageId = (record.function != nullptr) ? std::string(record.function) : "";
    env.payload   = record.message;   // stored as std::string

    // targets is intentionally empty: log events are always broadcast
    bus_.dispatch(env);
}

void LogDispatchBridge::flush()
{
    bus_.flush();
}

} // namespace GmDispatch
