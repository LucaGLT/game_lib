/**
 * @file flow/Turn.cpp
 * @brief Implementation of gmFlow::Turn.
 */

#include "gmFlow/flow/Turn.hpp"

#include <algorithm>

namespace gmFlow {

Turn::Turn(TurnId id)
    : id_(std::move(id))
{}

const TurnId& Turn::id() const
{
    return id_;
}

void Turn::add_active_actor(const ActorId& actor_id)
{
    active_actors_.push_back(actor_id);
}

const std::vector<ActorId>& Turn::active_actors() const
{
    return active_actors_;
}

bool Turn::is_actor_active(const ActorId& actor_id) const
{
    return std::find(active_actors_.begin(), active_actors_.end(), actor_id)
        != active_actors_.end();
}

} // namespace gmFlow
