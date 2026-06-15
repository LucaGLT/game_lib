#include "EventBusChannel.hpp"

namespace gmDispatch {

EventBusChannel::EventBusChannel(const std::string& channelName)
	: _name(channelName)
{}

std::string EventBusChannel::name() const
{
	return _name;
}

void EventBusChannel::add_handler(Handler handler)
{
	_handlers.push_back(std::move(handler));
}

void EventBusChannel::send(const Envelope& envelope)
{
	for (const Handler& h : _handlers) {
		h(envelope);
	}
}

void EventBusChannel::flush()
{
	// no-op — in-process channel has no buffering
}

} // namespace gmDispatch
