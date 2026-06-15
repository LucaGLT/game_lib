#ifndef GMDISPATCH_DISPATCHER_HPP
#define GMDISPATCH_DISPATCHER_HPP

/**
 * @file Dispatcher.hpp
 * @brief User-facing facade over IDispatcher — the primary application entry point.
 */

#include "DispatcherConfig.hpp"
#include "Envelope.hpp"
#include "IChannel.hpp"
#include "IDispatcher.hpp"

#include <memory>
#include <string>

namespace gmDispatch {

/**
 * @brief Core dispatcher facade — the primary application-facing class.
 *
 * GmDispatcher owns a @ref DispatcherConfig and delegates all routing and
 * subscription management to an @ref IDispatcher.  It is intentionally
 * ignorant of routing tables, serialisation formats, and thread locking.
 *
 * GmDispatcher is **non-copyable** and **move-constructible**.
 *
 * ### Recommended construction (via factory)
 * @code
 *   gmDispatch::GmDispatcher bus =
 *       gmDispatch::DispatcherFactory::create_sync_dispatcher("GameBus");
 * @endcode
 *
 * ### Direct construction
 * @code
 *   gmDispatch::DispatcherConfig cfg;
 *   cfg.name          = "GameBus";
 *   cfg.auto_timestamp = true;
 *
 *   gmDispatch::GmDispatcher bus(
 *       cfg,
 *       std::make_unique<gmDispatch::SyncDispatcher>(
 *           std::make_unique<gmDispatch::SyncRouter>()
 *       )
 *   );
 * @endcode
 *
 * ### Dispatching an event
 * @code
 *   gmDispatch::Envelope env;
 *   env.typeId  = "engine.tick";
 *   env.source  = "CoreEngine";
 *   env.payload = TickData{frame, dt};
 *   bus.dispatch(env);
 * @endcode
 *
 * ### Subscription management
 * @code
 *   std::shared_ptr<gmDispatch::EventBusChannel> ch =
 *       std::make_shared<gmDispatch::EventBusChannel>();
 *   ch->add_handler([](const gmDispatch::Envelope& e){ ... });
 *
 *   bus.subscribe("engine.tick", ch);
 *   // ...
 *   bus.unsubscribe("engine.tick", ch);
 * @endcode
 */
class GmDispatcher {
public:
	/**
	 * @brief Constructs a GmDispatcher with a given configuration and dispatcher impl.
	 *
	 * @param config     Identity and behaviour parameters.
	 * @param dispatcher Ownership-transferred dispatcher (sync or future async).
	 */
	GmDispatcher(DispatcherConfig                config,
			   std::unique_ptr<IDispatcher>    dispatcher);

	GmDispatcher(const GmDispatcher&)            = delete;
	GmDispatcher& operator=(const GmDispatcher&) = delete;

	GmDispatcher(GmDispatcher&&)                 = default;
	GmDispatcher& operator=(GmDispatcher&&)      = default;

	/**
	 * @brief Destructor — calls @ref flush() before releasing resources.
	 */
	~GmDispatcher();

	// ── Identity ──────────────────────────────────────────────────────────────

	/// @brief Returns the dispatcher name from @ref DispatcherConfig::name.
	const std::string& name() const;

	// ── Core dispatch ─────────────────────────────────────────────────────────

	/**
	 * @brief Dispatches an envelope to all subscribed channels.
	 *
	 * If @ref DispatcherConfig::auto_timestamp is @c true and
	 * @c envelope.timestamp is at the epoch (@c time_point{}), it is set to
	 * @c std::chrono::system_clock::now() before dispatching.
	 *
	 * @param envelope The event to dispatch.  The object is not modified by
	 *                 the dispatcher (auto-timestamp acts on a local copy).
	 */
	void dispatch(const Envelope& envelope);

	// ── Subscription management ───────────────────────────────────────────────

	/**
	 * @brief Registers a channel to receive envelopes with the given @p typeId.
	 *
	 * @param typeId  Subscription key.  Use @c "*" for a broadcast subscription.
	 * @param channel The channel to register.
	 */
	void subscribe(const std::string&        typeId,
				   std::shared_ptr<IChannel> channel);

	/**
	 * @brief Removes a subscription.
	 *
	 * @param typeId  The key used at subscription time.
	 * @param channel The channel to remove.
	 */
	void unsubscribe(const std::string&        typeId,
					 std::shared_ptr<IChannel> channel);

	// ── Flush ─────────────────────────────────────────────────────────────────

	/**
	 * @brief Flushes all registered channels via the underlying dispatcher.
	 *
	 * Called automatically by the destructor.
	 */
	void flush();

private:
	DispatcherConfig             _config;
	std::unique_ptr<IDispatcher> _dispatcher;
};

} // namespace gmDispatch

#endif // GMDISPATCH_DISPATCHER_HPP
