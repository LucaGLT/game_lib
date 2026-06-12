#ifndef GMDISPATCH_EVENTBUSCHANNEL_HPP
#define GMDISPATCH_EVENTBUSCHANNEL_HPP

/**
 * @file channels/EventBusChannel.hpp
 * @brief In-process channel that delivers raw Envelope objects to callbacks.
 */

#include "IChannel.hpp"
#include "Envelope.hpp"

#include <functional>
#include <vector>

namespace gmDispatch {

/**
 * @brief In-process channel that invokes registered @c std::function callbacks.
 *
 * EventBusChannel is the V1 mechanism for connecting engine subsystems to UI
 * components, AI systems, or unit-test fixtures without any serialisation or
 * I/O overhead.  Subscribers register a @ref Handler via @ref add_handler();
 * every call to @ref send() invokes all handlers synchronously with the raw
 * @ref Envelope.
 *
 * ### Thread safety
 * Handlers are invoked within the @ref SyncDispatcher's mutex.  Handlers must
 * **not** call @c subscribe(), @c unsubscribe(), or @c dispatch() on the same
 * @ref GmDispatcher instance (deadlock).  If bidirectional communication is
 * needed, use a separate @ref GmDispatcher or post to a queue.
 *
 * ### Implementing a handler
 * @code
 *   std::shared_ptr<gmDispatch::EventBusChannel> ch =
 *       std::make_shared<gmDispatch::EventBusChannel>();
 *
 *   ch->add_handler([](const gmDispatch::Envelope& env) {
 *       // access env.typeId, env.source, env.payload, …
 *       if (env.typeId == "engine.tick") {
 *           TickData td = std::any_cast<TickData>(env.payload);
 *           // …
 *       }
 *   });
 *
 *   bus.subscribe("engine.tick", ch);
 * @endcode
 */
class EventBusChannel : public IChannel {
public:
	/**
	 * @brief Callable type for envelope handlers.
	 *
	 * The handler receives a const reference to the envelope.  It must not
	 * store the reference beyond the call scope; copy the envelope if
	 * deferred processing is needed.
	 */
	using Handler = std::function<void(const Envelope&)>;

	/**
	 * @brief Constructs an EventBusChannel with an optional name.
	 *
	 * @param channelName Identifies this channel for targeted delivery
	 *                    via @ref PatternRouter.  Empty = anonymous (always
	 *                    receives, regardless of @c Envelope::targets).
	 */
	explicit EventBusChannel(const std::string& channelName = "");

	/// @brief Returns the channel name provided at construction.
	std::string name() const override;

	/**
	 * @brief Registers a callback to be invoked on every @ref send() call.
	 *
	 * Handlers are called in registration order.  Registering the same
	 * callable object multiple times results in multiple invocations.
	 *
	 * @param handler Callable with signature @c void(const Envelope&).
	 */
	void add_handler(Handler handler);

	/**
	 * @brief Invokes all registered handlers with @p envelope.
	 *
	 * @param envelope The dispatch event to deliver.
	 */
	void send(const Envelope& envelope) override;

	/**
	 * @brief No-op — in-process delivery has no buffering.
	 */
	void flush() override;

private:
	std::string          _name;
	std::vector<Handler> _handlers;
};

} // namespace gmDispatch

#endif // GMDISPATCH_EVENTBUSCHANNEL_HPP
