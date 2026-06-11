/**
 * @file events/EventBus.cpp
 * @brief Implementation of gmFlow::EventBus.
 */

#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/IEvent.hpp"

#include "gmDispatch/Dispatcher.hpp"
#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/DispatcherFactory.hpp"
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
    // TODO: Phase 4.1 — initialise internal channel registry
}

EventBus::~EventBus()
{
    // TODO: Phase 4.1 — unregister all internally created channels
}

void EventBus::subscribe(const EventType& event_type, Handler handler)
{
    // TODO: Phase 4.1 — create a GmDispatch::EventBusChannel wrapping the
    //   handler, subscribe it to dispatcher_ for event_type, and store the
    //   channel so that it can be removed in the destructor.
    (void)event_type;
    (void)handler;
}

void EventBus::publish(const IEvent& event)
{
    // TODO: Phase 4.1 — build a GmDispatch::Envelope with:
    //   env.typeId  = event.type()
    //   env.payload = std::cref(event)
    // then call dispatcher_->dispatch(env).
    (void)event;
}

} // namespace gmFlow
