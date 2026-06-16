/**
 * @file actors/ActorRegistry.cpp
 * @brief Implementation of gmFlow::ActorRegistry and EUnknownActorError.
 */

#include "gmFlow/actors/ActorRegistry.hpp"

#include <algorithm>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// EUnknownActorError
// ─────────────────────────────────────────────────────────────────────────────

EUnknownActorError::EUnknownActorError(const ActorId& actor_id)
    : std::runtime_error("ActorRegistry: unknown actor '" + actor_id + "'")
{}

// ─────────────────────────────────────────────────────────────────────────────
// Actor stubs (Actor.hpp methods are simple enough to define here)
// ─────────────────────────────────────────────────────────────────────────────

Actor::Actor(ActorId id, ActorType type)
    : _id(std::move(id)), _type(type), _display_name(_id)
{}

const ActorId& Actor::id() const           { return _id; }
ActorType      Actor::type() const         { return _type; }
const std::string& Actor::display_name() const { return _display_name; }

void Actor::set_display_name(std::string name)
{
    _display_name = std::move(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// ActorRegistry
// ─────────────────────────────────────────────────────────────────────────────

void ActorRegistry::add(Actor actor)
{
    const ActorId id = actor.id();
    if (_actors.count(id) == 0) {
        _insertion_order.push_back(id);
    }
    _actors.emplace(id, std::move(actor));
}

bool ActorRegistry::has(const ActorId& actor_id) const
{
    return _actors.count(actor_id) > 0;
}

const Actor& ActorRegistry::get(const ActorId& actor_id) const
{
    const auto it = _actors.find(actor_id);
    if (it == _actors.end()) {
        throw EUnknownActorError(actor_id);
    }
    return it->second;
}

std::vector<ActorId> ActorRegistry::all_ids() const
{
    return _insertion_order;
}

std::size_t ActorRegistry::count() const
{
    return _actors.size();
}

} // namespace gmFlow
