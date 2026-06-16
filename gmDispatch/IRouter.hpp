#ifndef GMDISPATCH_IROUTER_HPP
#define GMDISPATCH_IROUTER_HPP

/**
 * @file IRouter.hpp
 * @brief Abstract routing interface for the gmDispatch library.
 */

#include "Envelope.hpp"
#include "IChannel.hpp"

#include <memory>
#include <string>

namespace gmDispatch {

/**
 * @brief Abstract base class for envelope routers.
 *
 * A router maintains a subscription map (typeId → list of channels) and
 * delivers each incoming @ref Envelope to all channels whose subscription
 * key matches the envelope's @c typeId.
 *
 * This is the primary extension point for routing strategies:
 * - V1 @ref SyncRouter: exact-match and @c "*" wildcard.
 * - Phase 4: pattern-matching router (@c "engine.*").
 *
 * ### Thread safety
 * @ref IRouter implementations do **not** need their own mutex.  The owning
 * @ref SyncDispatcher acquires a lock before calling any @c IRouter method,
 * guaranteeing serialised access.
 *
 * ### Subscription key conventions
 * | Key | Meaning |
 * |-----|---------|
 * | @c "engine.tick" | Exact match — only envelopes with @c typeId == @c "engine.tick" |
 * | @c "*" | Broadcast — receives all envelopes regardless of @c typeId |
 *
 * ### No gmLog analog
 * @c IRouter has no equivalent in gmLog because gmLog's routing is fixed at
 * construction (one logger → one sink).  gmDispatch needs dynamic 1:N routing.
 */
class IRouter {
public:
	virtual ~IRouter() = default;

	/**
	 * @brief Registers a channel to receive envelopes matching @p typeId.
	 *
	 * The same channel may be subscribed to multiple @p typeId keys.
	 * Registering the same channel to the same @p typeId twice results in
	 * the channel receiving duplicate deliveries (implementation-defined
	 * whether duplicates are collapsed).
	 *
	 * @param typeId  Subscription key.  Use @c "*" for a broadcast subscription.
	 * @param channel Channel to add; ownership is shared (the router holds a
	 *                @c std::shared_ptr).
	 */
	virtual void subscribe(const std::string&        typeId,
						   std::shared_ptr<IChannel> channel) = 0;

	/**
	 * @brief Removes a previously registered subscription.
	 *
	 * Removes the first occurrence of @p channel under @p typeId.
	 * Does nothing if the subscription does not exist.
	 *
	 * @param typeId  The subscription key used when subscribing.
	 * @param channel The channel to remove.
	 */
	virtual void unsubscribe(const std::string&        typeId,
							 std::shared_ptr<IChannel> channel) = 0;

	/**
	 * @brief Routes the envelope to all matching channels.
	 *
	 * Calls @c IChannel::send() on every channel subscribed to
	 * @c envelope.typeId and every channel subscribed to @c "*".
	 *
	 * @param envelope The envelope to route.
	 */
	virtual void route(const Envelope& envelope) = 0;
	/**
	 * @brief Flushes all registered channels.
	 *
	 * Iterates every unique channel across all subscriptions and calls
	 * @c IChannel::flush() exactly once per channel instance.
	 * Called by the owning dispatcher inside its mutex lock.
	 */
	virtual void flush() = 0;};

} // namespace gmDispatch

#endif // GMDISPATCH_IROUTER_HPP
