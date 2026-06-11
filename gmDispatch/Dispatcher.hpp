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

namespace GmDispatch {

/**
 * @brief Core dispatcher facade — the primary application-facing class.
 *
 * Dispatcher owns a @ref DispatcherConfig and delegates all routing and
 * subscription management to an @ref IDispatcher.  It is intentionally
 * ignorant of routing tables, serialisation formats, and thread locking.
 *
 * Dispatcher is **non-copyable** and **move-constructible**.
 *
 * ### Recommended construction (via factory)
 * @code
 *   GmDispatch::Dispatcher bus =
 *       GmDispatch::DispatcherFactory::createSyncDispatcher("GameBus");
 * @endcode
 *
 * ### Direct construction
 * @code
 *   GmDispatch::DispatcherConfig cfg;
 *   cfg.name          = "GameBus";
 *   cfg.autoTimestamp = true;
 *
 *   GmDispatch::Dispatcher bus(
 *       cfg,
 *       std::make_unique<GmDispatch::SyncDispatcher>(
 *           std::make_unique<GmDispatch::SyncRouter>()
 *       )
 *   );
 * @endcode
 *
 * ### Dispatching an event
 * @code
 *   GmDispatch::Envelope env;
 *   env.typeId  = "engine.tick";
 *   env.source  = "CoreEngine";
 *   env.payload = TickData{frame, dt};
 *   bus.dispatch(env);
 * @endcode
 *
 * ### Subscription management
 * @code
 *   std::shared_ptr<GmDispatch::EventBusChannel> ch =
 *       std::make_shared<GmDispatch::EventBusChannel>();
 *   ch->addHandler([](const GmDispatch::Envelope& e){ ... });
 *
 *   bus.subscribe("engine.tick", ch);
 *   // ...
 *   bus.unsubscribe("engine.tick", ch);
 * @endcode
 */
class Dispatcher {
public:
    /**
     * @brief Constructs a Dispatcher with a given configuration and dispatcher impl.
     *
     * @param config     Identity and behaviour parameters.
     * @param dispatcher Ownership-transferred dispatcher (sync or future async).
     */
    Dispatcher(DispatcherConfig                config,
               std::unique_ptr<IDispatcher>    dispatcher);

    Dispatcher(const Dispatcher&)            = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Dispatcher(Dispatcher&&)                 = default;
    Dispatcher& operator=(Dispatcher&&)      = default;

    /**
     * @brief Destructor — calls @ref flush() before releasing resources.
     */
    ~Dispatcher();

    // ── Identity ──────────────────────────────────────────────────────────────

    /// @brief Returns the dispatcher name from @ref DispatcherConfig::name.
    const std::string& name() const;

    // ── Core dispatch ─────────────────────────────────────────────────────────

    /**
     * @brief Dispatches an envelope to all subscribed channels.
     *
     * If @ref DispatcherConfig::autoTimestamp is @c true and
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
    DispatcherConfig             config_;
    std::unique_ptr<IDispatcher> dispatcher_;
};

} // namespace GmDispatch

#endif // GMDISPATCH_DISPATCHER_HPP
