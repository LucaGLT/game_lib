#ifndef GMDISPATCH_DISPATCHERFACTORY_HPP
#define GMDISPATCH_DISPATCHERFACTORY_HPP

/**
 * @file DispatcherFactory.hpp
 * @brief Convenience factory for creating pre-configured Dispatcher instances.
 */

#include "Dispatcher.hpp"

#include <string>

namespace GmDispatch {

/**
 * @brief Static factory that creates ready-to-use @ref Dispatcher instances.
 *
 * DispatcherFactory reduces the construction boilerplate required to assemble
 * a Dispatcher with its underlying @ref IDispatcher, @ref IRouter, and
 * default channels.  All factory methods return a fully-constructed,
 * move-only @ref Dispatcher.
 *
 * @note This is a static-only utility class; it cannot be instantiated.
 *
 * ### V1 factory methods
 * | Method | Assembles |
 * |--------|-----------|
 * | @ref createSyncDispatcher | SyncDispatcher + SyncRouter (no channels) |
 * | @ref createDebugDispatcher | SyncDispatcher + SyncRouter + StdoutChannel(*) |
 *
 * ### Extending the factory
 * Add new static methods here when new channel or dispatcher types are
 * introduced.  The @ref Dispatcher class itself does not need to change.
 */
class DispatcherFactory {
public:
    DispatcherFactory()  = delete;
    ~DispatcherFactory() = delete;

    /**
     * @brief Creates a synchronous dispatcher with an empty @ref SyncRouter.
     *
     * Assembles: @ref SyncDispatcher → @ref SyncRouter (no channels subscribed).
     * Channels can be added via @ref Dispatcher::subscribe() at any time.
     *
     * @param name          Dispatcher name; appears in debug output.
     * @param autoTimestamp When @c true, @ref Dispatcher::dispatch() stamps
     *                      @c Envelope::timestamp automatically if unset.
     * @return A fully-configured, move-only @ref Dispatcher.
     */
    static Dispatcher createSyncDispatcher(
        const std::string& name,
        bool               autoTimestamp = true);

    /**
     * @brief Creates a synchronous dispatcher pre-wired with a
     *        @ref StdoutChannel subscribed to all messages (@c "*").
     *
     * Useful for development and debugging — every dispatched envelope is
     * printed as a JSON line to @c std::cout.
     *
     * @param name Dispatcher name.
     * @return A fully-configured, move-only @ref Dispatcher.
     */
    static Dispatcher createDebugDispatcher(
        const std::string& name);
};

} // namespace GmDispatch

#endif // GMDISPATCH_DISPATCHERFACTORY_HPP
