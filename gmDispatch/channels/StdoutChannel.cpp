#include "StdoutChannel.hpp"
#include "../serializers/JsonSerializer.hpp"

#include <iostream>

namespace GmDispatch {

StdoutChannel::StdoutChannel(std::unique_ptr<ISerializer> serializer)
    : serializer_(std::move(serializer))
{
    if (!serializer_) {
        serializer_ = std::make_unique<JsonSerializer>();
    }
}

void StdoutChannel::send(const Envelope& envelope)
{
    std::cout << serializer_->serialize(envelope) << std::endl;
}

void StdoutChannel::flush()
{
    std::cout.flush();
}

} // namespace GmDispatch
