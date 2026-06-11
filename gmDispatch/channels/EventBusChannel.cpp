#include "EventBusChannel.hpp"

namespace GmDispatch {

void EventBusChannel::addHandler(Handler /*handler*/)
{
    // TODO: Phase 2 — handlers_.push_back(std::move(handler));
}

void EventBusChannel::send(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 — for (const Handler& h : handlers_) { h(envelope); }
}

void EventBusChannel::flush()
{
    // no-op — in-process channel has no buffering
}

} // namespace GmDispatch
