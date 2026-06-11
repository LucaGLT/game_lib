#include "Dispatcher.hpp"

namespace GmDispatch {

Dispatcher::Dispatcher(DispatcherConfig             config,
                       std::unique_ptr<IDispatcher> dispatcher)
    : config_(std::move(config))
    , dispatcher_(std::move(dispatcher))
{}

Dispatcher::~Dispatcher()
{
    // TODO: Phase 2 — call flush() before releasing dispatcher_
}

const std::string& Dispatcher::name() const
{
    return config_.name;
}

void Dispatcher::dispatch(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 — if autoTimestamp and envelope.timestamp == time_point{},
    //                  set timestamp = std::chrono::system_clock::now() on a copy;
    //                  then delegate to dispatcher_->dispatch(copy).
}

void Dispatcher::subscribe(const std::string& /*typeId*/,
                           std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 — delegate to dispatcher_->subscribe(typeId, channel)
}

void Dispatcher::unsubscribe(const std::string& /*typeId*/,
                             std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 — delegate to dispatcher_->unsubscribe(typeId, channel)
}

void Dispatcher::flush()
{
    // TODO: Phase 2 — delegate to dispatcher_->flush()
}

} // namespace GmDispatch
