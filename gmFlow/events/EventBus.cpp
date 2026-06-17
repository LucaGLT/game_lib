/**
 * @file events/EventBus.cpp
 * @brief Implementation of gmFlow::EventBus.
 *
 * publish() wraps the IEvent in a gmDispatch::Envelope using
 * std::reference_wrapper<const IEvent> as payload.  This is safe because
 * SyncDispatcher invokes all handlers synchronously within dispatch(), so the
 * event reference is always valid during the call chain.
 */

#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/IEvent.hpp"

#include "gmDispatch/Dispatcher.hpp"
#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/channels/EventBusChannel.hpp"

#include <any>
#include <memory>
#include <stdexcept>

namespace gmFlow {

EventBus::EventBus(std::shared_ptr<gmDispatch::GmDispatcher> dispatcher)
    : _dispatcher(std::move(dispatcher))
{
    if (!_dispatcher) {
        throw std::invalid_argument("EventBus: dispatcher must not be null");
    }
}

EventBus::~EventBus()
{
    for (auto& sub : _subscriptions) {
        _dispatcher->unsubscribe(sub.first, sub.second);
    }
}

void EventBus::subscribe(const EventType& event_type, Handler handler)
{
    // Wrap the gmFlow handler in a gmDispatch::EventBusChannel.
    // The channel extracts the IEvent from the envelope payload and forwards
    // it to the user's handler.
    std::shared_ptr<gmDispatch::EventBusChannel> channel =
        std::make_shared<gmDispatch::EventBusChannel>();
    channel->add_handler([h = std::move(handler)](const gmDispatch::Envelope& env) {
        // The payload was stored as std::cref(event) in publish().
        const IEvent& ev =
            std::any_cast<std::reference_wrapper<const IEvent>>(env.payload).get();
        h(ev);
    });
    _dispatcher->subscribe(event_type, channel);
    _subscriptions.emplace_back(event_type, std::move(channel));
}

void EventBus::publish(const IEvent& event)
{
	gmDispatch::Envelope env;
    env.typeId  = event.type();
    env.source  = "gmFlow";
    // std::cref is safe: SyncDispatcher calls all handlers before returning.
    env.payload = std::cref(event);
    _dispatcher->dispatch(env);
}

} // namespace gmFlow
