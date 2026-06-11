#ifndef GMDISPATCH_ICHANNEL_HPP
#define GMDISPATCH_ICHANNEL_HPP

/**
 * @file IChannel.hpp
 * @brief Abstract output-channel interface for the GmDispatch library.
 */

#include "Envelope.hpp"

namespace GmDispatch {

/**
 * @brief Abstract base class for all dispatch output channels.
 *
 * A channel receives a fully-constructed @ref Envelope from the @ref IRouter
 * and delivers it to the underlying medium (in-process callbacks, stdout,
 * file, network socket, …).
 *
 * Each concrete channel decides independently whether it needs to serialise
 * the envelope.  In-process channels (e.g. @ref EventBusChannel) pass the
 * raw struct to registered callbacks; network/file channels compose with an
 * @ref ISerializer internally.
 *
 * ### Thread safety
 * The V1 @ref SyncDispatcher serialises all calls to @c send() and @c flush()
 * via its internal mutex, so concrete channel implementations do not need
 * their own locking.  Future async dispatchers must provide the same
 * guarantee or document otherwise.
 *
 * ### Implementing a custom channel
 * @code
 *   class MyChannel : public GmDispatch::IChannel {
 *   public:
 *       void send(const GmDispatch::Envelope& envelope) override { ... }
 *       void flush()                                     override { ... }
 *   };
 * @endcode
 */
class IChannel {
public:
    virtual ~IChannel() = default;

    /**
     * @brief Delivers a single envelope to the underlying output medium.
     *
     * Called by @ref IRouter once per matching subscription.  The envelope
     * is the same object passed to @ref Dispatcher::dispatch(); channels
     * must not modify it.
     *
     * @param envelope The dispatch event to deliver.
     */
    virtual void send(const Envelope& envelope) = 0;

    /**
     * @brief Flushes any buffered data to the underlying medium.
     *
     * For in-process channels (e.g. @ref EventBusChannel) this is a no-op.
     * For I/O channels (file, socket) it must ensure all buffered bytes
     * are written before returning.
     */
    virtual void flush() = 0;
};

} // namespace GmDispatch

#endif // GMDISPATCH_ICHANNEL_HPP
