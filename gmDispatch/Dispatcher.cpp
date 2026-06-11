#include "Dispatcher.hpp"

#include <chrono>

namespace GmDispatch {

Dispatcher::Dispatcher(DispatcherConfig             config,
                       std::unique_ptr<IDispatcher> dispatcher)
    : config_(std::move(config))
    , dispatcher_(std::move(dispatcher))
{}

Dispatcher::~Dispatcher()
{
    if (dispatcher_) {
        dispatcher_->flush();
    }
}

const std::string& Dispatcher::name() const
{
    return config_.name;
}

void Dispatcher::dispatch(const Envelope& envelope)
{
    if (!dispatcher_) return;

    if (config_.autoTimestamp &&
        envelope.timestamp == std::chrono::system_clock::time_point{}) {
        Envelope stamped   = envelope;
        stamped.timestamp  = std::chrono::system_clock::now();
        dispatcher_->dispatch(stamped);
    } else {
        dispatcher_->dispatch(envelope);
    }
}

void Dispatcher::subscribe(const std::string&        typeId,
                           std::shared_ptr<IChannel> channel)
{
    if (dispatcher_) {
        dispatcher_->subscribe(typeId, std::move(channel));
    }
}

void Dispatcher::unsubscribe(const std::string&        typeId,
                             std::shared_ptr<IChannel> channel)
{
    if (dispatcher_) {
        dispatcher_->unsubscribe(typeId, channel);
    }
}

void Dispatcher::flush()
{
    if (dispatcher_) {
        dispatcher_->flush();
    }
}

} // namespace GmDispatch
