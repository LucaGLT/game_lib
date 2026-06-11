#include "SyncDispatcher.hpp"

namespace GmDispatch {

SyncDispatcher::SyncDispatcher(std::unique_ptr<IRouter> router)
    : router_(std::move(router))
{}

void SyncDispatcher::dispatch(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 —
    //   std::lock_guard<std::mutex> lock(mutex_);
    //   router_->route(envelope);
}

void SyncDispatcher::subscribe(const std::string&        /*typeId*/,
                               std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 —
    //   std::lock_guard<std::mutex> lock(mutex_);
    //   router_->subscribe(typeId, std::move(channel));
}

void SyncDispatcher::unsubscribe(const std::string&        /*typeId*/,
                                 std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 —
    //   std::lock_guard<std::mutex> lock(mutex_);
    //   router_->unsubscribe(typeId, channel);
}

void SyncDispatcher::flush()
{
    // TODO: Phase 2 —
    //   std::lock_guard<std::mutex> lock(mutex_);
    //   Collect all unique channels from router_ and call flush() on each.
}

} // namespace GmDispatch
