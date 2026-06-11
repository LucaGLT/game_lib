#ifndef GMDISPATCH_SYNCROUTER_HPP
#define GMDISPATCH_SYNCROUTER_HPP

/**
 * @file routers/SyncRouter.hpp
 * @brief Synchronous 1:N router — V1 concrete implementation of IRouter.
 */

#include "../IRouter.hpp"
#include "../IChannel.hpp"
#include "../Envelope.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace GmDispatch {

/**
 * @brief Synchronous 1:N router with exact-match and wildcard @c "*" support.
 *
 * SyncRouter maintains an internal subscription map:
 * @code
 *   std::map<std::string, std::vector<std::shared_ptr<IChannel>>>
 * @endcode
 *
 * ### Routing rules (V1)
 * On each @ref route() call, @c send() is invoked on:
 * 1. All channels subscribed to @c envelope.typeId (exact match).
 * 2. All channels subscribed to @c "*" (broadcast subscription).
 *
 * If the same channel is subscribed to both the exact typeId and @c "*",
 * it will receive two calls.  Use separate channel instances to avoid this.
 *
 * ### Thread safety
 * SyncRouter has **no internal mutex**.  All locking is provided by the
 * owning @ref SyncDispatcher, which acquires its mutex before calling any
 * @c IRouter method.  This prevents double-locking.
 *
 * ### Phase 4 extensions
 * - Pattern-matching router: subscribe with @c "engine.*" wildcards.
 * - Targeted delivery: route only to channels listed in @c Envelope::targets.
 */
class SyncRouter : public IRouter {
public:
    SyncRouter() = default;

    /**
     * @brief Adds @p channel to the subscription list for @p typeId.
     *
     * @param typeId  Subscription key; use @c "*" for broadcast.
     * @param channel Channel to add.
     */
    void subscribe(const std::string&        typeId,
                   std::shared_ptr<IChannel> channel) override;

    /**
     * @brief Removes the first occurrence of @p channel under @p typeId.
     *
     * Does nothing if the subscription does not exist.
     *
     * @param typeId  The key used at subscription time.
     * @param channel The channel to remove.
     */
    void unsubscribe(const std::string&        typeId,
                     std::shared_ptr<IChannel> channel) override;

    /**
     * @brief Dispatches @p envelope to all matching channels.
     *
     * Calls @c IChannel::send() on channels subscribed to
     * @c envelope.typeId and on channels subscribed to @c "*".
     *
     * @param envelope The event to route.
     */
    void route(const Envelope& envelope) override;

private:
    /// Subscription map: typeId key → ordered list of subscribed channels.
    std::map<std::string, std::vector<std::shared_ptr<IChannel>>> routes_;
};

} // namespace GmDispatch

#endif // GMDISPATCH_SYNCROUTER_HPP
