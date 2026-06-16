#ifndef GMRULES_EFFECT_EFFECTRESOLVER_HPP
#define GMRULES_EFFECT_EFFECTRESOLVER_HPP

/**
 * @file effect/EffectResolver.hpp
 * @brief Resolves and applies `EffectSpec` instances against game state.
 *
 * `EffectResolver` is the **primary mutation point** in `gmRules`.
 * All state changes happen through `RuleContext` methods.
 *
 * ## Resolution flow for one effect
 * 1. Evaluate `effect.conditions` via `ConditionEvaluator`.
 * 2. Resolve `effect.target` via `TargetResolver`.
 * 3. For each resolved target, apply the effect through `RuleContext`.
 * 4. Emit `RuleEvent`s.
 * 5. Return `EffectResult`.
 */

#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectResult.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/core/RuleContext.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Applies `EffectSpec` instances through a `RuleContext`.
 *
 * Stateless — all methods are `const`.
 */
class EffectResolver
{
public:
    /**
     * @brief Resolves and applies one effect.
     *
     * @param effect            Effect specification.
     * @param source_actor_id   Actor originating the effect.
     * @param selected_targets  Pre-selected targets (forwarded to TargetResolver).
     * @param ctx               Mutable game state adapter.
     * @return                  Resolution outcome including emitted events.
     */
    EffectResult resolve(const EffectSpec& effect,
                         const ActorId& source_actor_id,
                         const std::vector<TargetRef>& selected_targets,
                         RuleContext& ctx) const;

    /**
     * @brief Resolves and applies multiple effects in sequence.
     *
     * Stops and returns failure at the first non-optional effect that fails
     * when `stop_on_failure == true`.  Optional failures become warnings.
     *
     * @param effects           Ordered list of effects.
     * @param source_actor_id   Actor originating the effects.
     * @param selected_targets  Pre-selected targets.
     * @param ctx               Mutable game state adapter.
     * @return                  Combined resolution outcome.
     */
    EffectResult resolve_many(const std::vector<EffectSpec>& effects,
                              const ActorId& source_actor_id,
                              const std::vector<TargetRef>& selected_targets,
                              RuleContext& ctx) const;
};

} // namespace gmRules

#endif // GMRULES_EFFECT_EFFECTRESOLVER_HPP
