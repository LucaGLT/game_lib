#ifndef GMACTOR_CORE_ERRORS_HPP
#define GMACTOR_CORE_ERRORS_HPP

/**
 * @file core/Errors.hpp
 * @brief Exception hierarchy for gmActor.
 *
 * Use exceptions for programming / data consistency errors (e.g. accessing an
 * actor that does not exist in the store).  For recoverable gameplay failures,
 * prefer result objects or return codes.
 */

#include <stdexcept>
#include <string>

namespace gmActor {

// ─────────────────────────────────────────────────────────────────────────────
// Base exception
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Base class for all gmActor runtime errors.
 */
class ActorError : public std::runtime_error {
public:
    explicit ActorError(const std::string& message)
        : std::runtime_error("gmActor: " + message) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Actor store errors
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thrown when an ActorId is not found in the ActorStore.
 */
class UnknownActorError : public ActorError {
public:
    explicit UnknownActorError(const std::string& actor_id)
        : ActorError("actor '" + actor_id + "' not found in store") {}
};

/**
 * @brief Thrown when attempting to add an actor whose ID is already registered.
 */
class DuplicateActorError : public ActorError {
public:
    explicit DuplicateActorError(const std::string& actor_id)
        : ActorError("actor '" + actor_id + "' is already registered") {}
};

/**
 * @brief Thrown when an operation is invalid for the actor's ActorKind.
 *
 * Example: calling `common(id)` on a MonsterGroup ID.
 */
class InvalidActorKindError : public ActorError {
public:
    explicit InvalidActorKindError(const std::string& actor_id,
                                   const std::string& context)
        : ActorError("invalid kind for actor '" + actor_id + "': " + context) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Item / equipment errors
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thrown when an ItemInstanceId is not found in an inventory.
 */
class UnknownItemError : public ActorError {
public:
    explicit UnknownItemError(const std::string& item_instance_id)
        : ActorError("item instance '" + item_instance_id + "' not found") {}
};

/**
 * @brief Thrown when an equipment slot operation is invalid.
 *
 * Example: equipping to a NONE slot, or double-equipping without unequipping.
 */
class InvalidEquipmentSlotError : public ActorError {
public:
    explicit InvalidEquipmentSlotError(const std::string& context)
        : ActorError("invalid equipment slot: " + context) {}
};

} // namespace gmActor

#endif // GMACTOR_CORE_ERRORS_HPP
