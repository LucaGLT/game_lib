/**
 * @file facade/gmRulesEngine.cpp
 * @brief Implementation of the gmRulesEngine façade.
 */

#include "gmRules/facade/gmRulesEngine.hpp"
#include "gmRules/loader/RuleBookLoader.hpp"

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
    RuleContext& ctx,
    int rule_priority)
{
    return effect_resolver_.resolve(spec,
                             source_actor_id,
                             selected_targets,
                             ctx,
                             rule_priority);
}

EffectResult gmRulesEngine::resolve_effects(
    const std::vector<EffectSpec>& specs,
    const ActorId& source_actor_id,
    const std::vector<TargetRef>& selected_targets,
    RuleContext& ctx,
    int rule_priority)
{
    return effect_resolver_.resolve_many(specs,
                              source_actor_id,
                              selected_targets,
                              ctx,
                              rule_priority);
}

RuleResult gmRulesEngine::apply_status(const StatusDefinition& status,
                                       const ActorId& owner_actor_id,
                                       const std::string& source_id,
                                       RuleContext& ctx)
{
	return status_engine_.apply_status(status, owner_actor_id, source_id, ctx);
}

} // namespace gmRules  — end of existing methods


// ─────────────────────────────────────────────────────────────────────────────
// RuleBook integration
// ─────────────────────────────────────────────────────────────────────────────

namespace gmRules {

void gmRulesEngine::load_rules_json(const std::string& path)
{
	RuleBookLoader::load_json(path, rule_book_);
}

void gmRulesEngine::load_rules_json_string(const std::string& json_text)
{
	RuleBookLoader::load_json_string(json_text, rule_book_);
}

RuleResult gmRulesEngine::resolve_rule(const RuleId&                  rule_id,
                                       const ActorId&                 source_actor_id,
                                       const std::vector<TargetRef>&  selected_targets,
                                       RuleContext&                   ctx)
{
	return rule_book_.resolve_rule(rule_id, source_actor_id, selected_targets, ctx);
}

RuleResult gmRulesEngine::resolve_rules(const std::vector<RuleId>&     rule_ids,
                                        const ActorId&                 source_actor_id,
                                        const std::vector<TargetRef>&  selected_targets,
                                        RuleContext&                   ctx)
{
	return rule_book_.resolve_rules(rule_ids, source_actor_id, selected_targets, ctx);
}

} // namespace gmRules
