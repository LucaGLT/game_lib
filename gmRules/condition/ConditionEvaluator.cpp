/**
 * @file condition/ConditionEvaluator.cpp
 * @brief Implementation of ConditionEvaluator.
 *
 * V1 conditions implemented:
 *   ALWAYS, NEVER, ACTOR_EXISTS, ACTOR_HAS_STATUS, ACTOR_HAS_TAG,
 *   ACTOR_HP_AT_OR_BELOW, ACTOR_HP_AT_OR_ABOVE, ACTOR_IN_LOCATION,
 *   LOCATION_HAS_TAG, LOCATION_IS_ADJACENT, TARGET_EXISTS,
 *   TARGET_HAS_STATUS, TARGET_HAS_TAG, RESOURCE_AT_LEAST,
 *   DECK_HAS_AT_LEAST, CARD_IN_ZONE,
 *   LOCATION_IS_REACHABLE, LOCATION_HAS_LOS, MOVE_COST_AT_MOST
 *
 * Composite operators: ALL_OF, ANY_OF, NONE_OF, NOT
 */

#include "gmRules/condition/ConditionEvaluator.hpp"

namespace gmRules {

// ── Atomic evaluation ─────────────────────────────────────────────────────────

static RuleResult evaluate_atomic(const ConditionSpec& c, const RuleContext& ctx)
{
	switch (c.type)
	{
		case ConditionType::ALWAYS:
			return RuleResult::ok();

		case ConditionType::NEVER:
			return RuleResult::fail(RuleError::CONDITION_FAILED, "NEVER condition");

		case ConditionType::ACTOR_EXISTS:
			if (ctx.has_actor(c.subject_id)) return RuleResult::ok();
			return RuleResult::fail(RuleError::UNKNOWN_ACTOR,
				"actor '" + c.subject_id + "' not found");

		case ConditionType::ACTOR_HAS_STATUS:
			if (ctx.actor_has_status(c.subject_id, c.value)) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"actor '" + c.subject_id + "' does not have status '" + c.value + "'");

		case ConditionType::ACTOR_HAS_TAG:
			if (ctx.actor_has_tag(c.subject_id, c.value)) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"actor '" + c.subject_id + "' does not have tag '" + c.value + "'");

		case ConditionType::ACTOR_HP_AT_OR_BELOW:
			if (ctx.actor_current_hp(c.subject_id) <= c.amount) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"actor HP " + std::to_string(ctx.actor_current_hp(c.subject_id))
				+ " > " + std::to_string(c.amount));

		case ConditionType::ACTOR_HP_AT_OR_ABOVE:
			if (ctx.actor_current_hp(c.subject_id) >= c.amount) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"actor HP " + std::to_string(ctx.actor_current_hp(c.subject_id))
				+ " < " + std::to_string(c.amount));

		case ConditionType::ACTOR_IN_LOCATION:
			if (ctx.actor_location(c.subject_id) == c.value) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"actor '" + c.subject_id + "' not in location '" + c.value + "'");

		case ConditionType::LOCATION_EXISTS:
			if (ctx.has_location(c.subject_id)) return RuleResult::ok();
			return RuleResult::fail(RuleError::UNKNOWN_LOCATION,
				"location '" + c.subject_id + "' not found");

		case ConditionType::LOCATION_HAS_TAG:
			if (ctx.location_has_tag(c.subject_id, c.value)) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"location '" + c.subject_id + "' does not have tag '" + c.value + "'");

		case ConditionType::LOCATION_IS_ADJACENT:
			if (ctx.are_locations_adjacent(c.subject_id, c.target_id)) return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"locations '" + c.subject_id + "' and '" + c.target_id + "' are not adjacent");

		case ConditionType::TARGET_EXISTS:
			// Without a target list in the evaluator signature, we check via subject_id
			if (!c.subject_id.empty() && ctx.has_actor(c.subject_id)) return RuleResult::ok();
			return RuleResult::fail(RuleError::INVALID_TARGET, "no valid target");

		case ConditionType::TARGET_HAS_STATUS:
			if (!c.subject_id.empty() && ctx.actor_has_status(c.subject_id, c.value))
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED, "target lacks status");

		case ConditionType::TARGET_HAS_TAG:
			if (!c.subject_id.empty() && ctx.actor_has_tag(c.subject_id, c.value))
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED, "target lacks tag");

		// ── Chapter 4 — gmActor ───────────────────────────────────────────────

		case ConditionType::RESOURCE_AT_LEAST:
			if (ctx.actor_resource(c.subject_id, c.value) >= c.amount)
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"resource '" + c.value + "' on actor '" + c.subject_id
				+ "' is below " + std::to_string(c.amount));

		// ── Chapter 5 — gmAlea ────────────────────────────────────────────────

		case ConditionType::DECK_HAS_AT_LEAST:
			if (!ctx.has_deck(c.subject_id))
				return RuleResult::fail(RuleError::UNKNOWN_DECK,
					"deck '" + c.subject_id + "' not found");
			if (ctx.deck_zone_count(c.subject_id, c.value) >= c.amount)
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"deck '" + c.subject_id + "' zone '" + c.value
				+ "' has fewer than " + std::to_string(c.amount) + " cards");

		case ConditionType::CARD_IN_ZONE:
			if (ctx.card_in_zone(c.subject_id, c.target_id, c.value))
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"card '" + c.target_id + "' not in deck '" + c.subject_id
				+ "' zone '" + c.value + "'");

		// ── Chapter 6 — gmMap ─────────────────────────────────────────────────

		case ConditionType::LOCATION_IS_REACHABLE:
			if (ctx.is_location_reachable(c.subject_id, c.target_id))
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"location '" + c.target_id + "' not reachable from '"
				+ c.subject_id + "'");

		case ConditionType::LOCATION_HAS_LOS:
			if (ctx.has_line_of_sight(c.subject_id, c.target_id))
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"no line-of-sight from '" + c.subject_id + "' to '"
				+ c.target_id + "'");

		case ConditionType::MOVE_COST_AT_MOST:
		{
			int cost = ctx.move_cost_between(c.subject_id, c.target_id);
			if (cost >= 0 && cost <= c.amount)
				return RuleResult::ok();
			return RuleResult::fail(RuleError::CONDITION_FAILED,
				"move cost " + std::to_string(cost) + " exceeds "
				+ std::to_string(c.amount));
		}

		default:
			return RuleResult::fail(RuleError::UNSUPPORTED_CONDITION,
				"Condition type not implemented in V1");
	}
}

// ── ConditionEvaluator::evaluate ──────────────────────────────────────────────

RuleResult ConditionEvaluator::evaluate(const ConditionSpec& condition,
                                        const RuleContext& ctx) const
{
	// Composite
	if (!condition.children.empty())
	{
		switch (condition.op)
		{
			case CompositeOperator::ALL_OF:
			{
				for (const ConditionSpec& child : condition.children)
				{
					RuleResult r = evaluate(child, ctx);
					if (!r.valid()) return r;
				}
				return RuleResult::ok();
			}

			case CompositeOperator::ANY_OF:
			{
				for (const ConditionSpec& child : condition.children)
				{
					if (evaluate(child, ctx).valid()) return RuleResult::ok();
				}
				return RuleResult::fail(RuleError::CONDITION_FAILED,
					"ANY_OF: no child condition passed");
			}

			case CompositeOperator::NONE_OF:
			{
				for (const ConditionSpec& child : condition.children)
				{
					if (evaluate(child, ctx).valid())
						return RuleResult::fail(RuleError::CONDITION_FAILED,
							"NONE_OF: a child condition passed when it should not");
				}
				return RuleResult::ok();
			}

			case CompositeOperator::NOT:
			{
				if (condition.children.empty())
					return RuleResult::fail(RuleError::CONDITION_FAILED, "NOT: no child");
				RuleResult r = evaluate(condition.children[0], ctx);
				if (!r.valid()) return RuleResult::ok();
				return RuleResult::fail(RuleError::CONDITION_FAILED,
					"NOT: child condition was true");
			}
		}
	}

	// Atomic
	return evaluate_atomic(condition, ctx);
}

RuleResult ConditionEvaluator::evaluate_all(const std::vector<ConditionSpec>& conditions,
                                            const RuleContext& ctx) const
{
	for (const ConditionSpec& c : conditions)
	{
		RuleResult r = evaluate(c, ctx);
		if (!r.valid()) return r;
	}
	return RuleResult::ok();
}

} // namespace gmRules
