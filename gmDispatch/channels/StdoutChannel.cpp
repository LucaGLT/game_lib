#include "StdoutChannel.hpp"
#include "serializers/JsonSerializer.hpp"

#include <iostream>

namespace gmDispatch {

StdoutChannel::StdoutChannel(const std::string&           channelName,
							 std::unique_ptr<ISerializer> serializer)
	: _name(channelName)
	, _serializer(std::move(serializer))
{
	if (!_serializer)
	{
		_serializer = std::make_unique<JsonSerializer>();
	}
}

std::string StdoutChannel::name() const
{
	return _name;
}

void StdoutChannel::send(const Envelope& envelope)
{
	std::cout << _serializer->serialize(envelope) << std::endl;
}

void StdoutChannel::flush()
{
	std::cout.flush();
}

} // namespace gmDispatch
