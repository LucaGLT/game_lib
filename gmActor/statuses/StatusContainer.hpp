#ifndef GMACTOR_STATUSES_STATUSCONTAINER_HPP
#define GMACTOR_STATUSES_STATUSCONTAINER_HPP

/**
 * @file statuses/StatusContainer.hpp
 * @brief Container for the active statuses on a single actor.
 *
 * ## Stackability rules
 *
 * | `stackable` param | Behaviour on duplicate `StatusId`           |
 * |-------------------|---------------------------------------------|
 * | `false`           | Existing instance is **replaced** (stacks=1)|
 * | `true`            | `stacks` on the existing instance is incremented by the incoming `stacks` value |
 *
 * The `stackable` flag is passed at call time (from `StatusDefinition::stackable`).
 * `StatusContainer` itself does not hold a definition table.
 */

#include "gmActor/statuses/StatusInstance.hpp"
#include "gmActor/core/Ids.hpp"

#include <optional>
#include <vector>

namespace gmActor {

/**
 * @brief Manages the set of active `StatusInstance` objects on one actor.
 */
class StatusContainer {
public:
    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * @brief Adds a status to the container.
     *
     * - If `stackable == false` and a status with the same id already exists,
     *   the existing instance is replaced.
     * - If `stackable == true` and a status with the same id already exists,
     *   the existing instance's `stacks` is incremented by `status.stacks`.
     * - Otherwise the status is added as a new entry.
     *
     * @param status    Status instance to add.
     * @param stackable Whether stacking behaviour should apply.
     */
    void add(StatusInstance status, bool stackable);

    /**
     * @brief Removes the status with the given id.  No-op if not present.
     *
     * @param id Status id to remove.
     */
    void remove(const StatusId& id);

    /**
     * @brief Removes all active statuses.
     */
    void clear();

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Returns true if a status with the given id is currently active.
     *
     * @param id Status id to check.
     */
    bool has(const StatusId& id) const;

    /**
     * @brief Returns a copy of the status with the given id, if present.
     *
     * @param id Status id to retrieve.
     * @return   Optional containing the instance, or `std::nullopt`.
     */
    std::optional<StatusInstance> get(const StatusId& id) const;

    /**
     * @brief Returns a const reference to all active status instances.
     */
    const std::vector<StatusInstance>& all() const;

private:
    std::vector<StatusInstance> statuses_;
};

} // namespace gmActor

#endif // GMACTOR_STATUSES_STATUSCONTAINER_HPP
