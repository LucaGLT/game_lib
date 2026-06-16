/**
 * @file flow/Turn.cpp
 * @brief Implementation of gmFlow::Turn.
 */

#include "gmFlow/flow/Turn.hpp"

#include <algorithm>

namespace gmFlow {

Turn::Turn(TurnId id)
    : _id(std::move(id))
{}

const TurnId& Turn::id() const
{
    return _id;
}

void Turn::add_active_actor(const ActorId& actor_id)
{
    _active_actors.push_back(actor_id);
}

const std::vector<ActorId>& Turn::active_actors() const
{
    return _active_actors;
}

bool Turn::is_actor_active(const ActorId& actor_id) const
{
    return std::find(_active_actors.begin(), _active_actors.end(), actor_id)
        != _active_actors.end();
}

} // namespace gmFlow
