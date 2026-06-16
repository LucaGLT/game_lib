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
    : _session_id(std::move(session_id))
    , _state(state)
    , _registry(registry)
    , _event_bus(event_bus)
{}

const SessionId& GameContext::session_id() const  { return _session_id; }
GameState&       GameContext::state()             { return _state; }
const GameState& GameContext::state() const       { return _state; }
ActorRegistry&   GameContext::actor_registry()    { return _registry; }
const ActorRegistry& GameContext::actor_registry() const { return _registry; }
EventBus&        GameContext::event_bus()         { return _event_bus; }

const PhaseId& GameContext::current_phase_id() const { return _current_phase_id; }
const RoundId& GameContext::current_round_id() const { return _current_round_id; }
const TurnId&  GameContext::current_turn_id()  const { return _current_turn_id; }

void GameContext::set_current_phase_id(PhaseId id) { _current_phase_id = std::move(id); }
void GameContext::set_current_round_id(RoundId id) { _current_round_id = std::move(id); }
void GameContext::set_current_turn_id(TurnId  id)  { _current_turn_id  = std::move(id); }

} // namespace gmFlow
