#ifndef GMDISPATCH_ENVELOPE_HPP
#define GMDISPATCH_ENVELOPE_HPP

/**
 * @file Envelope.hpp
 * @brief Message container passed through the GmDispatch pipeline.
 */

#include <any>
#include <chrono>
#include <string>
#include <vector>

namespace GmDispatch {

/**
 * @brief Represents a single dispatch event.
 *
 * An Envelope is created by the caller and passed to @ref Dispatcher::dispatch().
 * It travels through @ref IDispatcher → @ref IRouter → @ref IChannel unchanged.
 *
 * The @c payload field uses @c std::any so that any C++17 copy-constructible
 * type can be carried without template instantiation in the dispatch pipeline.
 * Receivers extract the value with @c std::any_cast<T>(env.payload).
 *
 * ### Source location
 * Unlike @c GmLog::LogRecord, Envelope does NOT carry @c __FILE__ / @c __LINE__
 * fields.  Application-level diagnostics should be logged separately via gmLog.
 *
 * ### Routing
 * @li @c typeId is matched against subscription keys in @ref IRouter.
 * @li @c targets (non-empty) restricts delivery to named channels — Phase 4 feature.
 * @li @c messageId is optional; useful for request/response correlation.
 *
 * @par Example
 * @code
 *   GmDispatch::Envelope env;
 *   env.typeId  = "engine.tick";
 *   env.source  = "CoreEngine";
 *   env.payload = TickData{frameId, dt};
 *   bus.dispatch(env);
 * @endcode
 */
struct Envelope {
    /**
     * @brief Message type identifier; used by @ref IRouter for subscription matching.
     *
     * Convention: @c "subsystem.event_name", e.g. @c "engine.tick",
     * @c "input.key_pressed", @c "ui.button_clicked".
     * Use @c "*" as the subscription key to receive all types.
     */
    std::string typeId;

    /// Identity of the sender (e.g. @c "CoreEngine", @c "InputSystem").
    std::string source;

    /**
     * @brief Named recipients; empty = broadcast to all subscribers of @c typeId.
     *
     * When non-empty, only channels whose registered name appears in this list
     * will receive the envelope.  This is a Phase 4 feature; V1 ignores it.
     */
    std::vector<std::string> targets;

    /**
     * @brief Optional unique message identifier.
     *
     * Useful for request/response correlation or deduplication.
     * Leave empty when not needed.
     */
    std::string messageId;

    /**
     * @brief Variable payload — cast with @c std::any_cast<T>(env.payload).
     *
     * Leave default-constructed (@c std::any{}) for zero-payload notifications.
     */
    std::any payload;

    /**
     * @brief Wall-clock timestamp.
     *
     * Set automatically by @ref Dispatcher when @c DispatcherConfig::autoTimestamp
     * is @c true and the field is at the default epoch value.
     * The caller may pre-set it to override auto-stamping.
     */
    std::chrono::system_clock::time_point timestamp;
};

} // namespace GmDispatch

#endif // GMDISPATCH_ENVELOPE_HPP
