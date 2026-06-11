#include "SyncDispatcher.hpp"

namespace GmDispatch {

SyncDispatcher::SyncDispatcher(std::unique_ptr<IRouter> router)
    : router_(std::move(router))
{}

void SyncDispatcher::dispatch(const Envelope& envelope)
{
    std::lock_guard<std::mutex> lock(mutex_);
    router_->route(envelope);
}

void SyncDispatcher::subscribe(const std::string&        typeId,
                               std::shared_ptr<IChannel> channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    router_->subscribe(typeId, std::move(channel));
}

void SyncDispatcher::unsubscribe(const std::string&        typeId,
                                 std::shared_ptr<IChannel> channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    router_->unsubscribe(typeId, channel);
}

void SyncDispatcher::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    router_->flush();
}

} // namespace GmDispatch
