#ifndef GMDISPATCH_DISPATCHERFACTORY_HPP
#define GMDISPATCH_DISPATCHERFACTORY_HPP

/**
 * @file DispatcherFactory.hpp
 * @brief Convenience factory for creating pre-configured GmDispatcher instances.
 */

#include "Dispatcher.hpp"

#include <string>

namespace gmDispatch {

/**
 * @brief Static factory that creates ready-to-use @ref GmDispatcher instances.
 *
 * | Method                    | Router         | GmDispatcher      | Default channels |
 * |---------------------------|----------------|-----------------|------------------|
 * | @ref create_sync_dispatcher | SyncRouter     | SyncDispatcher  | none             |
 * | @ref create_debug_dispatcher| SyncRouter     | SyncDispatcher  | StdoutChannel(*) |
 * | @ref create_async_dispatcher| SyncRouter     | AsyncDispatcher | none             |
 * | @ref create_pattern_dispatcher| PatternRouter | SyncDispatcher  | none             |
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
	 * @param name          GmDispatcher name.
	 * @param auto_timestamp Auto-stamp @c Envelope::timestamp when unset.
	 */
	static GmDispatcher create_sync_dispatcher(
		const std::string& name,
		bool               auto_timestamp = true);

	/**
	 * @brief Synchronous dispatcher pre-wired with a StdoutChannel on @c "*".
	 *
	 * Every dispatched envelope is printed as a JSON line to @c std::cout.
	 *
	 * @param name GmDispatcher name.
	 */
	static GmDispatcher create_debug_dispatcher(
		const std::string& name);

	/**
	 * @brief Asynchronous dispatcher + SyncRouter.
	 *
	 * @c dispatch() returns immediately; a worker thread routes envelopes.
	 * Call @c flush() to wait for all pending envelopes to be delivered.
	 *
	 * @param name          GmDispatcher name.
	 * @param auto_timestamp Auto-stamp @c Envelope::timestamp when unset.
	 */
	static GmDispatcher create_async_dispatcher(
		const std::string& name,
		bool               auto_timestamp = true);

	/**
	 * @brief Synchronous dispatcher + @ref PatternRouter.
	 *
	 * Use this variant when you need wildcard subscription patterns
	 * (@c "engine.*") or targeted delivery via @c Envelope::targets.
	 *
	 * @param name          GmDispatcher name.
	 * @param auto_timestamp Auto-stamp @c Envelope::timestamp when unset.
	 */
	static GmDispatcher create_pattern_dispatcher(
		const std::string& name,
		bool               auto_timestamp = true);
};

} // namespace gmDispatch

#endif // GMDISPATCH_DISPATCHERFACTORY_HPP
