#ifndef GMFLOW_ACTORREGISTRY_HPP
#define GMFLOW_ACTORREGISTRY_HPP

/**
 * @file actors/ActorRegistry.hpp
 * @brief Session-scoped registry that maps ActorId to Actor descriptors.
 *
 * The ActorRegistry is created by @ref GameSession at startup (populated from
 * @ref SessionConfig::actors) and is exposed via @ref GameContext.
 * It provides the canonical lookup service for all actor metadata during the
 * session lifetime.
 *
 * ### Usage
 * @code
 *   const gmFlow::Actor& a = ctx.actor_registry().get("player_1");
 *   bool exists = ctx.actor_registry().has("bot_2");
 * @endcode
 */

#include "gmFlow/actors/Actor.hpp"
#include "gmFlow/core/Ids.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gmFlow {

/**
 * @class UnknownActorError
 * @brief Thrown when an ActorId is not found in the registry.
 */
class UnknownActorError : public std::runtime_error {
public:
    /**
     * @brief Constructs the error with the missing actor ID embedded in the message.
     * @param actor_id The ActorId that was not found.
     */
    explicit UnknownActorError(const ActorId& actor_id);
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class ActorRegistry
 * @brief Maps ActorId strings to Actor descriptor objects for a session.
 *
 * The registry is non-copyable because it is owned by @ref GameSession and
 * referenced by @ref GameContext.  It is populated once at session start and
 * is read-only during normal session execution.
 */
class ActorRegistry {
public:
    ActorRegistry() = default;

    // Non-copyable.
    ActorRegistry(const ActorRegistry&)            = delete;
    ActorRegistry& operator=(const ActorRegistry&) = delete;
    ActorRegistry(ActorRegistry&&)                 = default;
    ActorRegistry& operator=(ActorRegistry&&)      = default;

    /**
     * @brief Registers an actor in the registry.
     *
     * Overwrites any existing actor with the same ID.
     * Called by @ref GameSession during initialisation.
     *
     * @param actor Actor descriptor to register.
     */
    void add(Actor actor);

    /**
     * @brief Returns true if an actor with the given ID exists.
     * @param actor_id ID to look up.
     * @return true if the actor is registered.
     */
    bool has(const ActorId& actor_id) const;

    /**
     * @brief Returns a reference to the actor with the given ID.
     *
     * @param actor_id ID to look up.
     * @return const reference to the Actor.
     * @throws UnknownActorError if the actor is not registered.
     */
    const Actor& get(const ActorId& actor_id) const;

    /// @brief Returns all registered actors.
    std::vector<ActorId> all_ids() const;

    /// @brief Returns the number of registered actors.
    std::size_t count() const;

private:
    std::unordered_map<ActorId, Actor> actors_;
    std::vector<ActorId>               insertion_order_; ///< Preserves add() order for turn sequencing.
};

} // namespace gmFlow

#endif // GMFLOW_ACTORREGISTRY_HPP
