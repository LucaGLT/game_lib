#include "EventBusChannel.hpp"

namespace GmDispatch {

EventBusChannel::EventBusChannel(const std::string& channelName)
    : name_(channelName)
{}

std::string EventBusChannel::name() const
{
    return name_;
}

void EventBusChannel::addHandler(Handler handler)
{
    handlers_.push_back(std::move(handler));
}

void EventBusChannel::send(const Envelope& envelope)
{
    for (const Handler& h : handlers_) {
        h(envelope);
    }
}

void EventBusChannel::flush()
{
    // no-op — in-process channel has no buffering
}

} // namespace GmDispatch
