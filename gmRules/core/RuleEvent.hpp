#ifndef GMRULES_CORE_RULEEVENT_HPP
#define GMRULES_CORE_RULEEVENT_HPP

/**
 * @file core/RuleEvent.hpp
 * @brief Lightweight event emitted by rule operations.
 *
 * `RuleEvent` objects are collected in `EffectResult::events()` and may
 * be forwarded to `gmDispatch` or a game-specific event bus by the caller.
 * `gmRules` itself does not own or manage an event bus.
 */

#include "gmRules/core/Ids.hpp"

#include <string>

namespace gmRules {

/**
 * @brief A lightweight, serializable event produced by a rule effect.
 *
 * `payload_json` is an opaque string.  It may be empty in V1.
 */
struct RuleEvent
{
    EventType   type;         ///< Event type tag (e.g. "gmRules.actor.damaged")
    std::string source_id;    ///< Who caused the event
    std::string target_id;    ///< What was affected
    std::string payload_json; ///< Optional JSON payload (empty in V1)
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEEVENT_HPP
