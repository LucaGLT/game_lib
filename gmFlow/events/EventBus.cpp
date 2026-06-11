/**
 * @file events/EventBus.cpp
 * @brief Implementation of gmFlow::EventBus.
 *
 * publish() wraps the IEvent in a GmDispatch::Envelope using
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

EventBus::EventBus(std::shared_ptr<GmDispatch::Dispatcher> dispatcher)
    : dispatcher_(std::move(dispatcher))
{
    if (!dispatcher_) {
        throw std::invalid_argument("EventBus: dispatcher must not be null");
    }
}

EventBus::~EventBus()
{
    for (auto& sub : subscriptions_) {
        dispatcher_->unsubscribe(sub.first, sub.second);
    }
}

void EventBus::subscribe(const EventType& event_type, Handler handler)
{
    // Wrap the gmFlow handler in a GmDispatch::EventBusChannel.
    // The channel extracts the IEvent from the envelope payload and forwards
    // it to the user's handler.
    auto channel = std::make_shared<GmDispatch::EventBusChannel>();
    channel->addHandler([h = std::move(handler)](const GmDispatch::Envelope& env) {
        // The payload was stored as std::cref(event) in publish().
        const IEvent& ev =
            std::any_cast<std::reference_wrapper<const IEvent>>(env.payload).get();
        h(ev);
    });
    dispatcher_->subscribe(event_type, channel);
    subscriptions_.emplace_back(event_type, std::move(channel));
}

void EventBus::publish(const IEvent& event)
{
    GmDispatch::Envelope env;
    env.typeId  = event.type();
    env.source  = "gmFlow";
    // std::cref is safe: SyncDispatcher calls all handlers before returning.
    env.payload = std::cref(event);
    dispatcher_->dispatch(env);
}

} // namespace gmFlow
