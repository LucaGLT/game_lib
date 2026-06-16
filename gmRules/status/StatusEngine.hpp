#ifndef GMRULES_STATUS_STATUSENGINE_HPP
#define GMRULES_STATUS_STATUSENGINE_HPP

/**
 * @file status/StatusEngine.hpp
 * @brief Manages status application, removal, and lifecycle hooks.
 *
 * `StatusEngine` uses `EffectResolver` internally for lifecycle effects
 * and delegates state mutations to `RuleContext`.
 *
 * ## Status lifecycle hooks
 * | Hook                    | When fired                                |
 * |-------------------------|-------------------------------------------|
 * | `on_apply`              | When the status is first applied          |
 * | `on_remove`             | When the status is removed or expires     |
 * | `on_activation_start`   | At the start of the owning actor's turn   |
 * | `on_activation_end`     | At the end of the owning actor's turn     |
 */

#include "gmRules/status/StatusDefinition.hpp"
#include "gmRules/status/StatusInstance.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleContext.hpp"
#include "gmRules/core/Ids.hpp"

namespace gmRules {

/**
 * @brief Manages status lifecycle through `RuleContext` and `EffectResolver`.
 *
 * Stateless — all state is stored in `RuleContext`.
 */
class StatusEngine
{
public:
    /**
     * @brief Applies a status definition to an actor.
     *
     * Creates a new `StatusInstance` (or updates an existing one per
     * the stacking policy), stores it via `RuleContext::add_status_instance()`,
     * then resolves `on_apply` effects.
     *
     * @param def           Status to apply.
     * @param owner_actor_id Actor receiving the status.
     * @param source_id     Who is applying the status.
     * @param ctx           Mutable game state adapter.
     */
    RuleResult apply_status(const StatusDefinition& def,
                            const ActorId& owner_actor_id,
                            const std::string& source_id,
                            RuleContext& ctx);

    /**
     * @brief Removes a status instance by its unique instance ID.
     *
     * Resolves `on_remove` effects, then calls
     * `RuleContext::remove_status_instance()`.
     *
     * @param status_instance_id Instance to remove.
     * @param owner_actor_id     Actor that owns the status (for on_remove targeting).
     * @param def                Status definition (for on_remove effects).
     * @param ctx                Mutable game state adapter.
     */
    RuleResult remove_status(const StatusInstanceId& status_instance_id,
                             const ActorId& owner_actor_id,
                             const StatusDefinition& def,
                             RuleContext& ctx);

    /**
     * @brief Fires `on_activation_start` effects for all statuses on the actor.
     *
     * @param actor_id Actor whose activation is starting.
     * @param statuses All status instances currently on the actor.
     * @param defs     Definitions registry (indexed by status_id).
     * @param ctx      Mutable game state adapter.
     */
    RuleResult on_activation_start(const ActorId& actor_id,
                                   const std::vector<StatusInstance>& statuses,
                                   const std::vector<StatusDefinition>& defs,
                                   RuleContext& ctx);

    /**
     * @brief Fires `on_activation_end` effects for all statuses on the actor.
     *
     * Also decrements `FOR_N_ACTIVATIONS` durations and marks expired instances.
     *
     * @param actor_id Actor whose activation is ending.
     * @param statuses All status instances currently on the actor.
     * @param defs     Definitions registry.
     * @param ctx      Mutable game state adapter.
     */
    RuleResult on_activation_end(const ActorId& actor_id,
                                 const std::vector<StatusInstance>& statuses,
                                 const std::vector<StatusDefinition>& defs,
                                 RuleContext& ctx);
};

} // namespace gmRules

#endif // GMRULES_STATUS_STATUSENGINE_HPP
