#ifndef GMFLOW_IEVENT_HPP
#define GMFLOW_IEVENT_HPP

/**
 * @file events/IEvent.hpp
 * @brief Base interface for all events published on the gmFlow EventBus.
 *
 * Every event that gmFlow or a game plug-in publishes must implement this
 * interface.  The @ref EventBus uses the `type()` return value to route
 * events to the correct subscribers via the underlying
 * `GmDispatch::EventBusChannel`.
 *
 * ### Implementing a custom event
 * @code
 *   class CardPlayedEvent : public gmFlow::IEvent {
 *   public:
 *       gmFlow::EventType type() const override { return "game.card_played"; }
 *       ActorId actor;
 *       uint32_t card_id;
 *   };
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"

namespace gmFlow {

/**
 * @class IEvent
 * @brief Pure-virtual base interface for all flow events.
 *
 * IEvent intentionally carries no payload fields; concrete event structs
 * declared in @ref FlowEvents.hpp store all event-specific data as public
 * members, making them trivially inspectable by subscribers without casting.
 */
class IEvent {
public:
    virtual ~IEvent() = default;

    /**
     * @brief Returns the event type string used for subscription routing.
     *
     * Convention: `"gmFlow.<subsystem>.<event_name>"` for built-in events.
     * Game-specific events should use a unique prefix, e.g. `"myGame.attack"`.
     *
     * @return EventType string key (one of the constants from @ref EventType.hpp
     *         for built-in events, or a custom string for plug-in events).
     */
    virtual EventType type() const = 0;
};

} // namespace gmFlow

#endif // GMFLOW_IEVENT_HPP
