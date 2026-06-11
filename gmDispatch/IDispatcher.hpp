#ifndef GMDISPATCH_IDISPATCHER_HPP
#define GMDISPATCH_IDISPATCHER_HPP

/**
 * @file IDispatcher.hpp
 * @brief Abstract dispatcher interface — the key extension point for async dispatch.
 */

#include "Envelope.hpp"
#include "IChannel.hpp"

#include <memory>
#include <string>

namespace GmDispatch {

/**
 * @brief Abstract base class for dispatch implementations.
 *
 * A dispatcher receives an @ref Envelope from the @ref Dispatcher facade,
 * routes it to all subscribed @ref IChannel instances, and manages the
 * subscription lifecycle.
 *
 * This abstraction decouples the @ref Dispatcher facade from the question of
 * *when* and *how* the envelope reaches the channels.
 *
 * ### V1 vs future
 * | Concrete type              | Behaviour                                         |
 * |----------------------------|---------------------------------------------------|
 * | @ref SyncDispatcher        | Routes and writes on the calling thread (V1).      |
 * | @c AsyncDispatcher (Phase 4)| Enqueues the envelope; a worker thread routes it. |
 *
 * The @ref Dispatcher facade holds a @c std::unique_ptr<IDispatcher>, so
 * switching from synchronous to asynchronous requires only changing the
 * object passed at construction time — no @ref Dispatcher API changes.
 *
 * ### Architecture diagram
 * @code
 *   Dispatcher (facade)
 *       ↓  dispatch(Envelope)
 *   IDispatcher
 *       ↓  IRouter::route(Envelope)
 *   IRouter
 *       ↓  IChannel::send(Envelope) ×N
 *   IChannel
 * @endcode
 */
class IDispatcher {
public:
    virtual ~IDispatcher() = default;

    /**
     * @brief Routes an envelope to all subscribed channels.
     *
     * In @ref SyncDispatcher this call blocks until all matched channels have
     * processed the envelope.  In a future @c AsyncDispatcher it enqueues
     * the envelope and returns immediately.
     *
     * @param envelope The event to dispatch.
     */
    virtual void dispatch(const Envelope& envelope) = 0;

    /**
     * @brief Registers a channel to receive envelopes with the given @p typeId.
     *
     * @param typeId  Subscription key.  Use @c "*" for a broadcast subscription.
     * @param channel The channel to register.
     */
    virtual void subscribe(const std::string&        typeId,
                           std::shared_ptr<IChannel> channel) = 0;

    /**
     * @brief Removes a subscription.
     *
     * @param typeId  The key used at subscription time.
     * @param channel The channel to remove.
     */
    virtual void unsubscribe(const std::string&        typeId,
                             std::shared_ptr<IChannel> channel) = 0;

    /**
     * @brief Flushes all registered channels.
     *
     * For synchronous dispatchers, flushes immediately.
     * For future async dispatchers, drains the pending queue first.
     */
    virtual void flush() = 0;
};

} // namespace GmDispatch

#endif // GMDISPATCH_IDISPATCHER_HPP
