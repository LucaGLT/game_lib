/**
 * @file core/RuleBook.cpp
 * @brief Implementation of RuleBook.
 */

#include "gmRules/core/RuleBook.hpp"
#include "gmRules/core/RuleError.hpp"

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// Registration
// ─────────────────────────────────────────────────────────────────────────────

void RuleBook::register_rule(const RuleDefinition& def)
{
	if (def.rule_id.empty())
	{
		throw ERuleBookError("register_rule: rule_id must not be empty");
	}
	if (_definitions.count(def.rule_id) > 0)
	{
		throw ERuleBookError(
			"register_rule: rule '" + def.rule_id + "' is already registered");
	}
	_definitions[def.rule_id] = def;
}

bool RuleBook::has_rule(const RuleId& rule_id) const
{
	return _definitions.count(rule_id) > 0;
}

const RuleDefinition& RuleBook::get_rule(const RuleId& rule_id) const
{
	auto it = _definitions.find(rule_id);
	if (it == _definitions.end())
	{
		throw ERuleBookError(
			"get_rule: rule '" + rule_id + "' is not registered");
	}
	return it->second;
}

int RuleBook::rule_count() const
{
	return static_cast<int>(_definitions.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Resolution
// ─────────────────────────────────────────────────────────────────────────────

RuleResult RuleBook::resolve_rule(const RuleId&                  rule_id,
                                  const ActorId&                 source_actor_id,
                                  const std::vector<TargetRef>&  selected_targets,
                                  RuleContext&                   ctx) const
{
	const RuleDefinition& def = get_rule(rule_id); // throws ERuleBookError if missing

	// ── 1. Evaluate preconditions ─────────────────────────────────────────────
	if (def.has_preconditions())
	{
		RuleResult cond_result = _condition_evaluator.evaluate_all(def.preconditions, ctx);
		if (!cond_result.valid())
		{
			return cond_result; // forward the failure — don't throw
		}
	}

	// ── 2. Apply effects ──────────────────────────────────────────────────────
	if (!def.has_effects())
	{
		return RuleResult::ok();
	}

	EffectResult eff_result = _effect_resolver.resolve_many(
		def.effects,
		source_actor_id,
		selected_targets,
		ctx);

	if (eff_result.succeeded())
	{
		return RuleResult::ok();
	}

	return RuleResult::fail(RuleError::EFFECT_FAILED,
	                        "resolve_rule: effect chain failed for rule '"
	                        + rule_id + "': " + eff_result.message());
}

RuleResult RuleBook::resolve_rules(const std::vector<RuleId>&     rule_ids,
                                   const ActorId&                 source_actor_id,
                                   const std::vector<TargetRef>&  selected_targets,
                                   RuleContext&                   ctx) const
{
	for (const RuleId& rid : rule_ids)
	{
		RuleResult r = resolve_rule(rid, source_actor_id, selected_targets, ctx);
		if (!r.valid())
		{
			return r; // stop on first failure
		}
	}
	return RuleResult::ok();
}

} // namespace gmRules
