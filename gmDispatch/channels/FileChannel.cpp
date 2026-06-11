/**
 * @file channels/FileChannel.cpp
 * @brief Implementation of FileChannel.
 */

#include "FileChannel.hpp"
#include "../serializers/JsonSerializer.hpp"

#include <stdexcept>

namespace GmDispatch {

FileChannel::FileChannel(const std::string&           filePath,
                         const std::string&           channelName,
                         std::unique_ptr<ISerializer> serializer)
    : name_(channelName)
    , filePath_(filePath)
    , serializer_(std::move(serializer))
{
    if (!serializer_) {
        serializer_ = std::make_unique<JsonSerializer>();
    }

    file_.open(filePath_, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("FileChannel: cannot open file: " + filePath_);
    }
}

FileChannel::~FileChannel()
{
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::string FileChannel::name() const
{
    return name_;
}

void FileChannel::send(const Envelope& envelope)
{
    file_ << serializer_->serialize(envelope) << '\n';
}

void FileChannel::flush()
{
    file_.flush();
}

} // namespace GmDispatch
