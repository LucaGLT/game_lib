#ifndef GMFLOW_EVENTBUS_HPP
#define GMFLOW_EVENTBUS_HPP

/**
 * @file events/EventBus.hpp
 * @brief Publish/subscribe bus for gmFlow lifecycle events.
 *
 * EventBus is a thin façade over `GmDispatch::Dispatcher` and
 * `GmDispatch::EventBusChannel`.  It translates `gmFlow::IEvent` objects
 * into `GmDispatch::Envelope` messages and routes them to registered
 * subscribers.
 *
 * ### Subscribing to an event
 * @code
 *   session.event_bus().subscribe(gmFlow::EVT_TURN_STARTED,
 *       [](const gmFlow::IEvent& e) {
 *           const auto& ev = static_cast<const gmFlow::TurnStartedEvent&>(e);
 *           // ev.active_actors …
 *       });
 * @endcode
 *
 * ### Publishing an event
 * @code
 *   gmFlow::TurnStartedEvent ev;
 *   ev.turn_id       = "round_1_turn_1";
 *   ev.active_actors = {"player_1"};
 *   ctx.event_bus().publish(ev);
 * @endcode
 *
 * ### Thread safety
 * Inherits the thread safety guarantees of the underlying
 * `GmDispatch::SyncDispatcher`: handlers are invoked synchronously on the
 * calling thread.  Handlers must not call `publish()` or `subscribe()` on
 * the same EventBus instance (potential deadlock).
 *
 * @note This class wraps `GmDispatch::EventBusChannel` — it never
 *       implements its own pub/sub mechanism.
 */

#include "gmFlow/events/IEvent.hpp"
#include "gmFlow/core/Ids.hpp"

#include <functional>
#include <memory>
#include <string>

// Forward declarations — avoid pulling in full gmDispatch headers here.
namespace GmDispatch {
    class Dispatcher;
}

namespace gmFlow {

/**
 * @class EventBus
 * @brief Thin pub/sub façade over `GmDispatch::Dispatcher`.
 *
 * One EventBus instance is owned by each @ref GameSession.  Access it via
 * `GameSession::event_bus()` or `GameContext::event_bus()`.
 */
class EventBus {
public:
    /// @brief Handler type for event subscribers.
    using Handler = std::function<void(const IEvent&)>;

    /**
     * @brief Constructs an EventBus backed by the provided GmDispatch Dispatcher.
     *
     * @param dispatcher Shared dispatcher (e.g. created via
     *                   `GmDispatch::DispatcherFactory::createSyncDispatcher()`).
     *                   Must outlive this EventBus.
     */
    explicit EventBus(std::shared_ptr<GmDispatch::Dispatcher> dispatcher);

    /// @brief Destructor — unregisters all internally created channels.
    ~EventBus();

    // Non-copyable, movable.
    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&)                 = default;
    EventBus& operator=(EventBus&&)      = default;

    /**
     * @brief Registers a handler to be invoked whenever an event of the given
     *        type is published.
     *
     * Multiple handlers may be registered for the same event type; they are
     * invoked in registration order.
     *
     * @param event_type One of the `EVT_*` constants from @ref EventType.hpp,
     *                   or a custom game-specific string.
     * @param handler    Callable invoked with a const reference to the event.
     */
    void subscribe(const EventType& event_type, Handler handler);

    /**
     * @brief Publishes an event to all subscribers registered for its type.
     *
     * Creates a `GmDispatch::Envelope` with `typeId = event.type()` and
     * `payload = std::cref(event)`, then dispatches it synchronously through
     * the underlying `GmDispatch::Dispatcher`.
     *
     * @param event The event to publish. Must remain valid for the duration
     *              of the call (handlers receive a const reference).
     */
    void publish(const IEvent& event);

private:
    std::shared_ptr<GmDispatch::Dispatcher> dispatcher_;
};

} // namespace gmFlow

#endif // GMFLOW_EVENTBUS_HPP
