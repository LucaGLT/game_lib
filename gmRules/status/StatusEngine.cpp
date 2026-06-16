/**
 * @file status/StatusEngine.cpp
 * @brief Implementation of StatusEngine.
 */

#include "gmRules/status/StatusEngine.hpp"
#include "gmRules/effect/EffectResolver.hpp"

#include <algorithm>

namespace gmRules {

// ── Internal helpers ──────────────────────────────────────────────────────────

static const StatusDefinition* find_def(const std::vector<StatusDefinition>& defs,
                                        const StatusId& id)
{
	for (const StatusDefinition& d : defs)
	{
		if (d.id == id) return &d;
	}
	return nullptr;
}

static std::string make_instance_id(const StatusId& status_id,
                                    const ActorId& owner_id,
                                    const std::string& source_id)
{
	return owner_id + "_" + status_id + "_" + source_id;
}

// ── StatusEngine::apply_status ────────────────────────────────────────────────

RuleResult StatusEngine::apply_status(const StatusDefinition& def,
                                      const ActorId& owner_actor_id,
                                      const std::string& source_id,
                                      RuleContext& ctx)
{
	// Check if actor already has this status
	bool already_has = ctx.actor_has_status(owner_actor_id, def.id);

	if (already_has)
	{
		switch (def.stacking_policy.mode)
		{
			case StackingMode::ONE_ONLY:
				return RuleResult::ok(); // discard silently

			case StackingMode::REPLACE:
			{
				// Remove all existing instances of this status then re-apply
				for (const StatusInstanceId& iid : ctx.statuses_on_actor(owner_actor_id))
				{
					// We remove all whose status_id matches; instance ID encodes this
					// (heuristic: instance_id contains status_id as substring)
					if (iid.find(def.id) != std::string::npos)
						ctx.remove_status_instance(iid);
				}
				break; // fall through to fresh apply below
			}

			case StackingMode::REFRESH_DURATION:
			{
				// Remove and re-add (effectively refreshes the duration stored in context)
				for (const StatusInstanceId& iid : ctx.statuses_on_actor(owner_actor_id))
				{
					if (iid.find(def.id) != std::string::npos)
						ctx.remove_status_instance(iid);
				}
				break;
			}

			case StackingMode::ADD_STACK:
			case StackingMode::UNIQUE_BY_SOURCE:
				// For V1: treat as REFRESH_DURATION
				break;
		}
	}

	// Build new instance
	StatusInstance inst;
	inst.instance_id    = make_instance_id(def.id, owner_actor_id, source_id);
	inst.status_id      = def.id;
	inst.owner_actor_id = owner_actor_id;
	inst.source_id      = source_id;
	inst.stacks         = 1;
	inst.duration.spec  = def.default_duration;
	inst.duration.remaining = def.default_duration.amount;
	inst.duration.expired   = false;

	ctx.add_status_instance(inst);

	// Resolve on_apply effects
	if (!def.on_apply.empty())
	{
		EffectResolver resolver;
		TargetRef self_target;
		self_target.kind = TargetKind::ACTOR;
		self_target.id   = owner_actor_id;

		EffectResult er = resolver.resolve_many(
			def.on_apply, owner_actor_id, {self_target}, ctx);

		if (!er.succeeded())
			return RuleResult::fail(RuleError::EFFECT_FAILED,
				"on_apply failed: " + er.message());
	}

	return RuleResult::ok();
}

// ── StatusEngine::remove_status ───────────────────────────────────────────────

RuleResult StatusEngine::remove_status(const StatusInstanceId& status_instance_id,
                                       const ActorId& owner_actor_id,
                                       const StatusDefinition& def,
                                       RuleContext& ctx)
{
	// Resolve on_remove effects before removing
	if (!def.on_remove.empty())
	{
		EffectResolver resolver;
		TargetRef self_target;
		self_target.kind = TargetKind::ACTOR;
		self_target.id   = owner_actor_id;

		EffectResult er = resolver.resolve_many(
			def.on_remove, owner_actor_id, {self_target}, ctx);

		if (!er.succeeded())
			return RuleResult::fail(RuleError::EFFECT_FAILED,
				"on_remove failed: " + er.message());
	}

	ctx.remove_status_instance(status_instance_id);
	return RuleResult::ok();
}

// ── StatusEngine::on_activation_start ─────────────────────────────────────────

RuleResult StatusEngine::on_activation_start(
    const ActorId& actor_id,
    const std::vector<StatusInstance>& statuses,
    const std::vector<StatusDefinition>& defs,
    RuleContext& ctx)
{
	EffectResolver resolver;
	TargetRef self_target;
	self_target.kind = TargetKind::ACTOR;
	self_target.id   = actor_id;

	for (const StatusInstance& inst : statuses)
	{
		if (inst.owner_actor_id != actor_id) continue;
		const StatusDefinition* def = find_def(defs, inst.status_id);
		if (!def || def->on_activation_start.empty()) continue;

		EffectResult er = resolver.resolve_many(
			def->on_activation_start, actor_id, {self_target}, ctx);

		if (!er.succeeded())
			return RuleResult::fail(RuleError::EFFECT_FAILED,
				"on_activation_start failed for status '" + inst.status_id
				+ "': " + er.message());
	}

	// Handle UNTIL_NEXT_ACTIVATION expiry
	for (const StatusInstance& inst : statuses)
	{
		if (inst.owner_actor_id != actor_id) continue;
		if (inst.duration.spec.type == DurationType::UNTIL_NEXT_ACTIVATION)
			ctx.remove_status_instance(inst.instance_id);
	}

	return RuleResult::ok();
}

// ── StatusEngine::on_activation_end ───────────────────────────────────────────

RuleResult StatusEngine::on_activation_end(
    const ActorId& actor_id,
    const std::vector<StatusInstance>& statuses,
    const std::vector<StatusDefinition>& defs,
    RuleContext& ctx)
{
	EffectResolver resolver;
	TargetRef self_target;
	self_target.kind = TargetKind::ACTOR;
	self_target.id   = actor_id;

	for (const StatusInstance& inst : statuses)
	{
		if (inst.owner_actor_id != actor_id) continue;
		const StatusDefinition* def = find_def(defs, inst.status_id);
		if (!def || def->on_activation_end.empty()) continue;

		EffectResult er = resolver.resolve_many(
			def->on_activation_end, actor_id, {self_target}, ctx);

		if (!er.succeeded())
			return RuleResult::fail(RuleError::EFFECT_FAILED,
				"on_activation_end failed for status '" + inst.status_id
				+ "': " + er.message());
	}

	// Decrement FOR_N_ACTIVATIONS; remove if exhausted
	for (const StatusInstance& inst : statuses)
	{
		if (inst.owner_actor_id != actor_id) continue;
		if (inst.duration.spec.type == DurationType::FOR_N_ACTIVATIONS)
		{
			// Remaining is tracked externally (context owns the live instances).
			// For V1: remove after each activation end (amount == 1 means one turn only).
			if (inst.duration.spec.amount <= 1)
				ctx.remove_status_instance(inst.instance_id);
		}
	}

	return RuleResult::ok();
}

} // namespace gmRules
