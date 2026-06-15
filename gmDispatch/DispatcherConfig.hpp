#ifndef GMDISPATCH_DISPATCHERCONFIG_HPP
#define GMDISPATCH_DISPATCHERCONFIG_HPP

/**
 * @file DispatcherConfig.hpp
 * @brief Configuration parameters for a single GmDispatcher instance.
 */

#include <string>

namespace gmDispatch {

/**
 * @brief Holds the static configuration of a @ref GmDispatcher.
 *
 * Pass a populated DispatcherConfig to the @ref GmDispatcher constructor (or to
 * a @ref DispatcherFactory helper) to define the dispatcher's identity and
 * behaviour without coupling configuration concerns to the implementation.
 *
 * Future extensions (async-queue capacity, flush-on-destroy flag, etc.) should
 * be added here as optional fields rather than as additional GmDispatcher
 * constructor parameters.
 *
 * @par Example
 * @code
 *   gmDispatch::DispatcherConfig cfg;
 *   cfg.name          = "GameBus";
 *   cfg.auto_timestamp = true;
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
	 * When @c true, @ref GmDispatcher::dispatch() sets @c Envelope::timestamp
	 * to @c std::chrono::system_clock::now() if the caller leaves it at the
	 * default epoch value (@c time_point{}).
	 *
	 * Set to @c false when the caller wants to control the timestamp explicitly
	 * (e.g. replaying recorded events).
	 */
	bool auto_timestamp = true;
};

} // namespace gmDispatch

#endif // GMDISPATCH_DISPATCHERCONFIG_HPP
