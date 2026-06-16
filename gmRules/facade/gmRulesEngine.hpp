#ifndef GMRULES_FACADE_GMRULESENGINE_HPP
#define GMRULES_FACADE_GMRULESENGINE_HPP

/**
 * @file facade/gmRulesEngine.hpp
 * @brief Top-level façade for the gmRules library.
 *
 * `gmRulesEngine` composes `TargetResolver`, `ConditionEvaluator`,
 * `EffectResolver`, and `StatusEngine`.  Game-specific code should
 * interact with `gmRules` through this class.
 *
 * ## Typical usage
 * @code
 *   MyRuleContext ctx(game_state, event_bus);
 *   gmRules::gmRulesEngine engine;
 *
 *   auto result = engine.resolve_effects(card.effects, actor_id, targets, ctx);
 *   if (!result.succeeded()) {
 *       // handle failure
 *   }
 * @endcode
 */

#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/target/TargetResult.hpp"
#include "gmRules/target/TargetResolver.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectResult.hpp"
#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/status/StatusDefinition.hpp"
#include "gmRules/status/StatusEngine.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleContext.hpp"
#include "gmRules/core/Ids.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Top-level façade for the gmRules rule toolkit.
 *
 * Composes TargetResolver, ConditionEvaluator, EffectResolver, StatusEngine.
 */
class gmRulesEngine
{
public:
    // ── Target ────────────────────────────────────────────────────────────────

    /**
     * @brief Resolves a target specification.
     *
     * Side-effect-free — delegates to `TargetResolver`.
     */
    TargetResult resolve_target(const TargetSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                                const RuleContext& ctx);

    // ── Conditions ────────────────────────────────────────────────────────────

    /**
     * @brief Evaluates a single condition.
     *
     * Side-effect-free — delegates to `ConditionEvaluator`.
     */
    RuleResult evaluate_condition(const ConditionSpec& spec,
                                  const RuleContext& ctx);

    /**
     * @brief Evaluates all conditions as an implicit ALL_OF.
     *
     * Side-effect-free — delegates to `ConditionEvaluator`.
     */
    RuleResult evaluate_conditions(const std::vector<ConditionSpec>& specs,
                                   const RuleContext& ctx);

    // ── Effects ───────────────────────────────────────────────────────────────

    /**
     * @brief Resolves and applies one effect.
     *
     * Delegates to `EffectResolver`.
     */
    EffectResult resolve_effect(const EffectSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                            RuleContext& ctx,
                            int rule_priority = 100);

    /**
     * @brief Resolves and applies multiple effects in sequence.
     *
     * Delegates to `EffectResolver`.
     */
    EffectResult resolve_effects(const std::vector<EffectSpec>& specs,
                                 const ActorId& source_actor_id,
                                 const std::vector<TargetRef>& selected_targets,
                             RuleContext& ctx,
                             int rule_priority = 100);

    // ── Status ────────────────────────────────────────────────────────────────

    /**
     * @brief Applies a status definition to an actor.
     *
     * Delegates to `StatusEngine`.
     */
    RuleResult apply_status(const StatusDefinition& status,
                            const ActorId& owner_actor_id,
                            const std::string& source_id,
                            RuleContext& ctx);

private:
    TargetResolver     target_resolver_;
    ConditionEvaluator condition_evaluator_;
    EffectResolver     effect_resolver_;
    StatusEngine       status_engine_;
};

} // namespace gmRules

#endif // GMRULES_FACADE_GMRULESENGINE_HPP
