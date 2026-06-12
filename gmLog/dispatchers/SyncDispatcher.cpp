/**
 * @file dispatchers/SyncDispatcher.cpp
 * @brief Implementation of SyncDispatcher.
 */

#include "SyncDispatcher.hpp"

#include <mutex>

namespace gmLog {

SyncDispatcher::SyncDispatcher(std::unique_ptr<ILogSink>       sink,
							   std::unique_ptr<ILogFormatter>  formatter
)
	: _sink(std::move(sink))
	, _formatter(std::move(formatter))
{}

void SyncDispatcher::dispatch(const LogRecord& record)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sink->write(_formatter->format(record));
}

void SyncDispatcher::flush()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sink->flush();
}

} // namespace gmLog
