#ifndef GMDISPATCH_SYNCDISPATCHER_HPP
#define GMDISPATCH_SYNCDISPATCHER_HPP

/**
 * @file dispatchers/SyncDispatcher.hpp
 * @brief Thread-safe synchronous dispatcher — V1 concrete implementation of IDispatcher.
 */

#include "IDispatcher.hpp"
#include "IRouter.hpp"
#include "IChannel.hpp"
#include "Envelope.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace gmDispatch {

/**
 * @brief Thread-safe synchronous dispatcher.
 *
 * SyncDispatcher is the V1 concrete implementation of @ref IDispatcher.
 * It owns an @ref IRouter and serialises **all** operations under a single
 * @c std::mutex:
 *
 * - @ref dispatch() — locks → @c IRouter::route(envelope) → unlock
 * - @ref subscribe() — locks → @c IRouter::subscribe() → unlock
 * - @ref unsubscribe() — locks → @c IRouter::unsubscribe() → unlock
 * - @ref flush() — locks → flush all channels via router → unlock
 *
 * Because the mutex is held for each entire operation, @ref IRouter
 * implementations do **not** need their own locking.
 *
 * ### Migrating to async
 * When asynchronous dispatch is needed, replace SyncDispatcher with a future
 * @c AsyncDispatcher (Phase 4).  Because @ref GmDispatcher holds a
 * @c std::unique_ptr<IDispatcher>, the change is confined to the construction
 * site (or the @ref DispatcherFactory helper).
 *
 * ### Re-entrant dispatch (request-response pattern)
 * When a channel handler calls @c dispatch() on the same @ref GmDispatcher
 * (e.g. a CoreEngine responding to a UI request), the same thread re-enters
 * @c SyncDispatcher::dispatch().  This is safe because @c _mutex  is a
 * @c std::recursive_mutex — the same thread may acquire it multiple times.
 *
 * ### Deadlock warning
 * Do **not** call @c subscribe() or @c unsubscribe() from within a channel
 * handler on the same @c GmDispatcher — those operations also acquire the
 * recursive mutex and are therefore safe to re-enter, but doing so while
 * iterating the routing table may corrupt it.  If dynamic subscription
 * management from within a callback is required, use @ref AsyncDispatcher.
 */
class SyncDispatcher : public IDispatcher {
public:
	/**
	 * @brief Constructs a SyncDispatcher with the given router.
	 *
	 * @param router Unique-ownership pointer to the routing implementation.
	 */
	explicit SyncDispatcher(std::unique_ptr<IRouter> router);

	~SyncDispatcher() = default;

	/**
	 * @brief Routes the envelope to all subscribed channels under a mutex lock.
	 *
	 * @param envelope The event to dispatch.
	 */
	void dispatch(const Envelope& envelope) override;

	/**
	 * @brief Registers a channel under the mutex lock.
	 *
	 * @param typeId  Subscription key.  Use @c "*" for broadcast.
	 * @param channel The channel to register.
	 */
	void subscribe(const std::string&        typeId,
				   std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Removes a subscription under the mutex lock.
	 *
	 * @param typeId  The key used at subscription time.
	 * @param channel The channel to remove.
	 */
	void unsubscribe(const std::string&        typeId,
					 std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Flushes all registered channels under a mutex lock.
	 */
	void flush() override;

private:
	std::unique_ptr<IRouter>    _router;
	mutable std::recursive_mutex _mutex;
};

} // namespace gmDispatch

#endif // GMDISPATCH_SYNCDISPATCHER_HPP
