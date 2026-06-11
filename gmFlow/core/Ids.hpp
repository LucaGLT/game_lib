#ifndef GMFLOW_IDS_HPP
#define GMFLOW_IDS_HPP

/**
 * @file core/Ids.hpp
 * @brief Canonical type aliases for all game-flow identifiers.
 *
 * All IDs are `std::string`.  String IDs are:
 * - directly serializable by gmSave without extra mapping;
 * - human-readable in log output via gmLog;
 * - cross-subsystem safe without a global numeric registry.
 *
 * Use the distinct aliases (PlayerId, ActionId, …) rather than raw
 * `std::string` to make function signatures self-documenting and to help
 * the compiler catch transposed arguments.
 */

#include <string>

namespace gmFlow {

/// @brief Identifies a human or remote player within a session.
using PlayerId = std::string;

/// @brief Identifies any entity that can act: player, bot, system, team, or GM.
using ActorId = std::string;

/// @brief Unique identifier for a single action instance.
using ActionId = std::string;

/// @brief Identifies one step inside a multi-step action.
using StepId = std::string;

/// @brief Identifies a game phase (e.g. "SETUP", "COMBAT", "CLEANUP").
using PhaseId = std::string;

/// @brief Identifies a turn within a round.
using TurnId = std::string;

/// @brief Identifies a round within a session.
using RoundId = std::string;

/// @brief Identifies a single play session (one game scenario or match).
using SessionId = std::string;

/// @brief String key used to identify event types on the EventBus.
using EventType = std::string;

} // namespace gmFlow

#endif // GMFLOW_IDS_HPP
