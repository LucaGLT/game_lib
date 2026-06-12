#include "SyncRouter.hpp"

#include <algorithm>

namespace GmDispatch {

void SyncRouter::subscribe(const std::string&        typeId,
                           std::shared_ptr<IChannel> channel)
{
    routes_[typeId].push_back(std::move(channel));
}

void SyncRouter::unsubscribe(const std::string&        typeId,
                             std::shared_ptr<IChannel> channel)
{
    std::map<std::string,
             std::vector<std::shared_ptr<IChannel>>>::iterator it = routes_.find(typeId);
    if (it == routes_.end()) return;

    std::vector<std::shared_ptr<IChannel>>& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), channel), vec.end());
    if (vec.empty()) routes_.erase(it);
}

void SyncRouter::route(const Envelope& envelope)
{
    // Exact-match subscribers
    std::map<std::string,
             std::vector<std::shared_ptr<IChannel>>>::iterator it =
        routes_.find(envelope.typeId);

    if (it != routes_.end()) {
        for (std::shared_ptr<IChannel>& ch : it->second) {
            ch->send(envelope);
        }
    }

    // Wildcard "*" subscribers — skip if typeId already is "*" to avoid
    // a double send for channels subscribed only to "*".
    if (envelope.typeId != "*") {
        std::map<std::string,
                 std::vector<std::shared_ptr<IChannel>>>::iterator wild =
            routes_.find("*");
        if (wild != routes_.end()) {
            for (std::shared_ptr<IChannel>& ch : wild->second) {
                ch->send(envelope);
            }
        }
    }
}

void SyncRouter::flush()
{
    // Flush each unique channel exactly once, even if subscribed to multiple keys.
    std::unordered_set<IChannel*> seen;
    for (std::pair<const std::string,
                   std::vector<std::shared_ptr<IChannel>>>& kv : routes_) {
        for (std::shared_ptr<IChannel>& ch : kv.second) {
            if (seen.insert(ch.get()).second) {
                ch->flush();
            }
        }
    }
}

} // namespace GmDispatch
