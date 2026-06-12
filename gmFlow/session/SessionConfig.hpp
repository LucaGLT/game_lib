#ifndef GMFLOW_SESSIONCONFIG_HPP
#define GMFLOW_SESSIONCONFIG_HPP

/**
 * @file session/SessionConfig.hpp
 * @brief Configuration struct that describes a game session before it starts.
 *
 * SessionConfig is the single object passed to the @ref GameSession constructor.
 * It collects all static session parameters in one place so that `GameSession`
 * can be constructed with a single, readable block of configuration code.
 *
 * ### Minimal example
 * @code
 *   gmFlow::SessionConfig cfg;
 *   cfg.session_id   = "heroquest_001";
 *   cfg.session_name = "Scenario 1 — The Maze";
 *   cfg.actors       = {
 *       gmFlow::Actor("barbarian", gmFlow::ActorType::PLAYER),
 *       gmFlow::Actor("elf",       gmFlow::ActorType::PLAYER),
 *       gmFlow::Actor("zargon",    gmFlow::ActorType::GAME_MASTER)
 *   };
 *   cfg.turn_policy.allow_simultaneous_turns = false;
 *   cfg.round_policy.max_rounds              = -1;
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/actors/Actor.hpp"
#include "gmFlow/flow/TurnPolicy.hpp"
#include "gmFlow/flow/RoundPolicy.hpp"

#include <string>
#include <vector>

namespace gmFlow {

/**
 * @struct SessionConfig
 * @brief All static parameters required to construct a @ref GameSession.
 */
struct SessionConfig {
    /// @brief Unique identifier for this session (used in logs and save files).
    SessionId session_id;

    /// @brief Human-readable session name (e.g. scenario title).
    std::string session_name;

    /// @brief All actors participating in this session.
    std::vector<Actor> actors;

    /// @brief Turn management flags.
    TurnPolicy turn_policy;

    /// @brief Round management flags.
    RoundPolicy round_policy;
};

} // namespace gmFlow

#endif // GMFLOW_SESSIONCONFIG_HPP
