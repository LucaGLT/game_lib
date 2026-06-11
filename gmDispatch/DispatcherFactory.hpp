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
 * | Method                    | Router         | Dispatcher      | Default channels |
 * |---------------------------|----------------|-----------------|------------------|
 * | @ref createSyncDispatcher | SyncRouter     | SyncDispatcher  | none             |
 * | @ref createDebugDispatcher| SyncRouter     | SyncDispatcher  | StdoutChannel(*) |
 * | @ref createAsyncDispatcher| SyncRouter     | AsyncDispatcher | none             |
 * | @ref createPatternDispatcher| PatternRouter | SyncDispatcher  | none             |
 *
 * @note This is a static-only utility class; it cannot be instantiated.
 */
class DispatcherFactory {
public:
    DispatcherFactory()  = delete;
    ~DispatcherFactory() = delete;

    /**
     * @brief Synchronous dispatcher + SyncRouter (no channels).
     *
     * @param name          Dispatcher name.
     * @param autoTimestamp Auto-stamp @c Envelope::timestamp when unset.
     */
    static Dispatcher createSyncDispatcher(
        const std::string& name,
        bool               autoTimestamp = true);

    /**
     * @brief Synchronous dispatcher pre-wired with a StdoutChannel on @c "*".
     *
     * Every dispatched envelope is printed as a JSON line to @c std::cout.
     *
     * @param name Dispatcher name.
     */
    static Dispatcher createDebugDispatcher(
        const std::string& name);

    /**
     * @brief Asynchronous dispatcher + SyncRouter.
     *
     * @c dispatch() returns immediately; a worker thread routes envelopes.
     * Call @c flush() to wait for all pending envelopes to be delivered.
     *
     * @param name          Dispatcher name.
     * @param autoTimestamp Auto-stamp @c Envelope::timestamp when unset.
     */
    static Dispatcher createAsyncDispatcher(
        const std::string& name,
        bool               autoTimestamp = true);

    /**
     * @brief Synchronous dispatcher + @ref PatternRouter.
     *
     * Use this variant when you need wildcard subscription patterns
     * (@c "engine.*") or targeted delivery via @c Envelope::targets.
     *
     * @param name          Dispatcher name.
     * @param autoTimestamp Auto-stamp @c Envelope::timestamp when unset.
     */
    static Dispatcher createPatternDispatcher(
        const std::string& name,
        bool               autoTimestamp = true);
};

} // namespace GmDispatch

#endif // GMDISPATCH_DISPATCHERFACTORY_HPP
