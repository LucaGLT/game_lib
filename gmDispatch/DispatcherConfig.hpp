#ifndef GMDISPATCH_DISPATCHERCONFIG_HPP
#define GMDISPATCH_DISPATCHERCONFIG_HPP

/**
 * @file DispatcherConfig.hpp
 * @brief Configuration parameters for a single Dispatcher instance.
 */

#include <string>

namespace GmDispatch {

/**
 * @brief Holds the static configuration of a @ref Dispatcher.
 *
 * Pass a populated DispatcherConfig to the @ref Dispatcher constructor (or to
 * a @ref DispatcherFactory helper) to define the dispatcher's identity and
 * behaviour without coupling configuration concerns to the implementation.
 *
 * Future extensions (async-queue capacity, flush-on-destroy flag, etc.) should
 * be added here as optional fields rather than as additional Dispatcher
 * constructor parameters.
 *
 * @par Example
 * @code
 *   GmDispatch::DispatcherConfig cfg;
 *   cfg.name          = "GameBus";
 *   cfg.autoTimestamp = true;
 * @endcode
 */
struct DispatcherConfig {
    /**
     * @brief Human-readable dispatcher name.
     *
     * Appears in debug output (e.g. @ref StdoutChannel) and in future logging
     * bridges.  Does not affect routing behaviour.
     */
    std::string name;

    /**
     * @brief Automatically stamp the envelope timestamp at dispatch time.
     *
     * When @c true, @ref Dispatcher::dispatch() sets @c Envelope::timestamp
     * to @c std::chrono::system_clock::now() if the caller leaves it at the
     * default epoch value (@c time_point{}).
     *
     * Set to @c false when the caller wants to control the timestamp explicitly
     * (e.g. replaying recorded events).
     */
    bool autoTimestamp = true;
};

} // namespace GmDispatch

#endif // GMDISPATCH_DISPATCHERCONFIG_HPP
