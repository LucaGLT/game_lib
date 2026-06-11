/**
 * @file actors/ActorRegistry.cpp
 * @brief Implementation of gmFlow::ActorRegistry and UnknownActorError.
 */

#include "gmFlow/actors/ActorRegistry.hpp"

#include <algorithm>

namespace gmFlow {

// ─────────────────────────────────────────────────────────────────────────────
// UnknownActorError
// ─────────────────────────────────────────────────────────────────────────────

UnknownActorError::UnknownActorError(const ActorId& actor_id)
    : std::runtime_error("ActorRegistry: unknown actor '" + actor_id + "'")
{}

// ─────────────────────────────────────────────────────────────────────────────
// Actor stubs (Actor.hpp methods are simple enough to define here)
// ─────────────────────────────────────────────────────────────────────────────

Actor::Actor(ActorId id, ActorType type)
    : id_(std::move(id)), type_(type), display_name_(id_)
{}

const ActorId& Actor::id() const           { return id_; }
ActorType      Actor::type() const         { return type_; }
const std::string& Actor::display_name() const { return display_name_; }

void Actor::set_display_name(std::string name)
{
    display_name_ = std::move(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// ActorRegistry
// ─────────────────────────────────────────────────────────────────────────────

void ActorRegistry::add(Actor actor)
{
    // TODO: Phase 4.2 — consider emitting a log warning if ID already exists
    actors_.emplace(actor.id(), std::move(actor));
}

bool ActorRegistry::has(const ActorId& actor_id) const
{
    return actors_.count(actor_id) > 0;
}

const Actor& ActorRegistry::get(const ActorId& actor_id) const
{
    const auto it = actors_.find(actor_id);
    if (it == actors_.end()) {
        throw UnknownActorError(actor_id);
    }
    return it->second;
}

std::vector<ActorId> ActorRegistry::all_ids() const
{
    std::vector<ActorId> ids;
    ids.reserve(actors_.size());
    for (const auto& pair : actors_) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::size_t ActorRegistry::count() const
{
    return actors_.size();
}

} // namespace gmFlow
