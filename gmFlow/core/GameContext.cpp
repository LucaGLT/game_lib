/**
 * @file core/GameContext.cpp
 * @brief Implementation of gmFlow::GameContext.
 */

#include "gmFlow/core/GameContext.hpp"

namespace gmFlow {

GameContext::GameContext(SessionId      session_id,
                         GameState&     state,
                         ActorRegistry& registry,
                         EventBus&      event_bus)
    : session_id_(std::move(session_id))
    , state_(state)
    , registry_(registry)
    , event_bus_(event_bus)
{}

const SessionId& GameContext::session_id() const  { return session_id_; }
GameState&       GameContext::state()             { return state_; }
const GameState& GameContext::state() const       { return state_; }
ActorRegistry&   GameContext::actor_registry()    { return registry_; }
const ActorRegistry& GameContext::actor_registry() const { return registry_; }
EventBus&        GameContext::event_bus()         { return event_bus_; }

const PhaseId& GameContext::current_phase_id() const { return current_phase_id_; }
const RoundId& GameContext::current_round_id() const { return current_round_id_; }
const TurnId&  GameContext::current_turn_id()  const { return current_turn_id_; }

void GameContext::set_current_phase_id(PhaseId id) { current_phase_id_ = std::move(id); }
void GameContext::set_current_round_id(RoundId id) { current_round_id_ = std::move(id); }
void GameContext::set_current_turn_id(TurnId  id)  { current_turn_id_  = std::move(id); }

} // namespace gmFlow
