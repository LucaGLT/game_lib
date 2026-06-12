#ifndef GMDISPATCH_ASYNCDISPATCHER_HPP
#define GMDISPATCH_ASYNCDISPATCHER_HPP

/**
 * @file dispatchers/AsyncDispatcher.hpp
 * @brief Non-blocking asynchronous dispatcher — Phase 4.
 */

#include "IDispatcher.hpp"
#include "IRouter.hpp"
#include "IChannel.hpp"
#include "Envelope.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace gmDispatch {

/**
 * @brief Non-blocking dispatcher that routes envelopes on a dedicated worker thread.
 *
 * AsyncDispatcher is a drop-in replacement for @ref SyncDispatcher.  Because
 * both implement @ref IDispatcher, swapping them requires only changing the
 * object passed at construction time — no @ref GmDispatcher API changes.
 *
 * ### Behaviour
 * - `dispatch()` enqueues the envelope and returns **immediately** (non-blocking).
 * - A dedicated worker thread dequeues envelopes and calls `IRouter::route()`.
 * - `subscribe()` / `unsubscribe()` may be called from any thread; they are
 *   serialised via a separate mutex from the queue.
 * - `flush()` **blocks** until the internal queue is drained and the worker
 *   has finished processing the last envelope, then flushes all channels.
 *
 * ### Mutexes
 * Two independent mutexes avoid the deadlock risk of a single coarse lock:
 * | Mutex         | Protects                                         |
 * |---------------|--------------------------------------------------|
 * | `_queue_mutex` | `_queue`, `_worker_busy`, `_stop`, `_queue_cv`, `_drain_cv` |
 * | `_route_mutex` | `_router` (subscribe / unsubscribe / route / flush) |
 *
 * ### Deadlock warning
 * Do not call `subscribe()`, `unsubscribe()`, `dispatch()`, or `flush()` on
 * the **same** `GmDispatcher` from within a channel's `send()` callback.
 * Doing so will deadlock on `_route_mutex`.
 *
 * @par Example
 * @code
 *   gmDispatch::GmDispatcher bus(
 *       cfg,
 *       std::make_unique<gmDispatch::AsyncDispatcher>(
 *           std::make_unique<gmDispatch::SyncRouter>()));
 *
 *   bus.subscribe("engine.tick", ch);
 *
 *   env.typeId = "engine.tick";
 *   bus.dispatch(env);   // returns immediately
 *   bus.flush();         // waits for delivery
 * @endcode
 */
class AsyncDispatcher : public IDispatcher {
public:
	/**
	 * @brief Constructs an AsyncDispatcher and starts the worker thread.
	 *
	 * @param router Ownership-transferred router.
	 */
	explicit AsyncDispatcher(std::unique_ptr<IRouter> router);

	/**
	 * @brief Stops the worker thread and flushes all pending envelopes.
	 */
	~AsyncDispatcher();

	AsyncDispatcher(const AsyncDispatcher&)            = delete;
	AsyncDispatcher& operator=(const AsyncDispatcher&) = delete;

	/**
	 * @brief Enqueues @p envelope for asynchronous delivery.
	 *
	 * Returns immediately.  The envelope will be routed by the worker thread.
	 *
	 * @param envelope The event to enqueue.
	 */
	void dispatch(const Envelope& envelope) override;

	/**
	 * @brief Subscribes @p channel under the route mutex (thread-safe).
	 *
	 * @param typeId  Subscription key.
	 * @param channel The channel to register.
	 */
	void subscribe(const std::string&        typeId,
				   std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Unsubscribes @p channel under the route mutex (thread-safe).
	 *
	 * @param typeId  The key used at subscription time.
	 * @param channel The channel to remove.
	 */
	void unsubscribe(const std::string&        typeId,
					 std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Blocks until the queue is empty and the last envelope has been
	 *        routed, then flushes all channels.
	 */
	void flush() override;

private:
	/// Worker thread body — drains the queue and calls _router->route().
	void worker_loop();

	std::unique_ptr<IRouter> _router;

	// ── Queue state ────────────────────────────────────────────────────────
	std::queue<Envelope>    _queue;
	std::mutex              _queue_mutex;
	std::condition_variable _queue_cv;   ///< Worker wakes up when queue is non-empty.
	std::condition_variable _drain_cv;   ///< flush() wakes up when queue is empty.
	bool                    _worker_busy{false};
	bool                    _stop{false};

	// ── Router state ───────────────────────────────────────────────────────
	mutable std::mutex _route_mutex;

	// ── Worker thread ──────────────────────────────────────────────────────
	std::thread _worker;
};

} // namespace gmDispatch

#endif // GMDISPATCH_ASYNCDISPATCHER_HPP
