/**
 * @file dispatchers/SyncDispatcher.cpp
 * @brief Implementation of SyncDispatcher.
 */

#include "SyncDispatcher.hpp"

#include <mutex>

namespace GmLog {

SyncDispatcher::SyncDispatcher(std::unique_ptr<ILogSink>       sink,
                               std::unique_ptr<ILogFormatter>  formatter)
    : sink_(std::move(sink))
    , formatter_(std::move(formatter))
{}

void SyncDispatcher::dispatch(const LogRecord& record)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sink_->write(formatter_->format(record));
}

void SyncDispatcher::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sink_->flush();
}

} // namespace GmLog
