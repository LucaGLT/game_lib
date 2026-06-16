/**
 * @file facade/gmRulesEngine.cpp
 * @brief Implementation of the gmRulesEngine façade.
 */

#include "gmRules/facade/gmRulesEngine.hpp"

namespace gmRules {

TargetResult gmRulesEngine::resolve_target(
    const TargetSpec& spec,
    const ActorId& source_actor_id,
    const std::vector<TargetRef>& selected_targets,
    const RuleContext& ctx)
{
	return target_resolver_.resolve(spec, source_actor_id, selected_targets, ctx);
}

RuleResult gmRulesEngine::evaluate_condition(const ConditionSpec& spec,
                                             const RuleContext& ctx)
{
	return condition_evaluator_.evaluate(spec, ctx);
}

RuleResult gmRulesEngine::evaluate_conditions(const std::vector<ConditionSpec>& specs,
                                              const RuleContext& ctx)
{
	return condition_evaluator_.evaluate_all(specs, ctx);
}

EffectResult gmRulesEngine::resolve_effect(
    const EffectSpec& spec,
    const ActorId& source_actor_id,
    const std::vector<TargetRef>& selected_targets,
    RuleContext& ctx)
{
	return effect_resolver_.resolve(spec, source_actor_id, selected_targets, ctx);
}

EffectResult gmRulesEngine::resolve_effects(
    const std::vector<EffectSpec>& specs,
    const ActorId& source_actor_id,
    const std::vector<TargetRef>& selected_targets,
    RuleContext& ctx)
{
	return effect_resolver_.resolve_many(specs, source_actor_id, selected_targets, ctx);
}

RuleResult gmRulesEngine::apply_status(const StatusDefinition& status,
                                       const ActorId& owner_actor_id,
                                       const std::string& source_id,
                                       RuleContext& ctx)
{
	return status_engine_.apply_status(status, owner_actor_id, source_id, ctx);
}

} // namespace gmRules
