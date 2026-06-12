#include "Dispatcher.hpp"

#include <chrono>

namespace gmDispatch {

GmDispatcher::GmDispatcher(DispatcherConfig             config,
					   std::unique_ptr<IDispatcher> dispatcher)
	: _config(std::move(config))
	, _dispatcher(std::move(dispatcher))
{}

GmDispatcher::~GmDispatcher()
{
	if (_dispatcher) {
		_dispatcher->flush();
	}
}

const std::string& GmDispatcher::name() const
{
	return _config.name;
}

void GmDispatcher::dispatch(const Envelope& envelope)
{
	if (!_dispatcher) return;

	if (_config.auto_timestamp &&
		envelope.timestamp == std::chrono::system_clock::time_point{})
	{
		Envelope stamped   = envelope;
		stamped.timestamp  = std::chrono::system_clock::now();
		_dispatcher->dispatch(stamped);
	}
	else
	{
		_dispatcher->dispatch(envelope);
	}
}

void GmDispatcher::subscribe(const std::string&        typeId,
						   std::shared_ptr<IChannel> channel)
{
	if (_dispatcher)
	{
		_dispatcher->subscribe(typeId, std::move(channel));
	}
}

void GmDispatcher::unsubscribe(const std::string&        typeId,
							 std::shared_ptr<IChannel> channel)
{
	if (_dispatcher)
	{
		_dispatcher->unsubscribe(typeId, channel);
	}
}

void GmDispatcher::flush()
{
	if (_dispatcher)
	{
		_dispatcher->flush();
	}
}

} // namespace gmDispatch
