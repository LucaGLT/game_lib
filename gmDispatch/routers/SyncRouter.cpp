#include "SyncRouter.hpp"

#include <algorithm>

namespace GmDispatch {

void SyncRouter::subscribe(const std::string&        /*typeId*/,
                           std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 — routes_[typeId].push_back(std::move(channel));
}

void SyncRouter::unsubscribe(const std::string&        /*typeId*/,
                             std::shared_ptr<IChannel> /*channel*/)
{
    // TODO: Phase 2 —
    //   auto it = routes_.find(typeId);
    //   if (it == routes_.end()) return;
    //   auto& vec = it->second;
    //   vec.erase(std::remove(vec.begin(), vec.end(), channel), vec.end());
    //   if (vec.empty()) routes_.erase(it);
}

void SyncRouter::route(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 —
    //   1. Find routes_[envelope.typeId] — call send() on each channel.
    //   2. Find routes_["*"]            — call send() on each channel.
    //   (No duplicate-call suppression in V1: if a channel is in both lists
    //    it will receive two calls.  Document as expected behaviour.)
}

} // namespace GmDispatch
