/**
 * @file bridges/LogDispatchBridge.cpp
 * @brief Implementation of LogDispatchBridge.
 */

#include "LogDispatchBridge.hpp"
#include "../gmLog/LogLevel.hpp"

namespace gmDispatch {

LogDispatchBridge::LogDispatchBridge(GmDispatcher& bus)
	: _bus(bus)
{}

void LogDispatchBridge::dispatch(const gmLog::LogRecord& record)
{
	Envelope env;

	env.typeId    = std::string("log.") + gmLog::level_to_string(record.level);
	env.source    = record.logger_name;
	env.timestamp = record.timestamp;
	env.messageId = (record.function != nullptr) ? std::string(record.function) : "";
	env.payload   = record.message;   // stored as std::string

	// targets is intentionally empty: log events are always broadcast
	_bus.dispatch(env);
}

void LogDispatchBridge::flush()
{
	_bus.flush();
}

} // namespace gmDispatch
