#include "StdoutChannel.hpp"

#include <iostream>

namespace GmDispatch {

StdoutChannel::StdoutChannel(std::unique_ptr<ISerializer> serializer)
    : serializer_(std::move(serializer))
{
    // TODO: Phase 2 — if (!serializer_) { serializer_ = std::make_unique<JsonSerializer>(); }
}

void StdoutChannel::send(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 — std::cout << serializer_->serialize(envelope) << std::endl;
}

void StdoutChannel::flush()
{
    std::cout.flush();
}

} // namespace GmDispatch
