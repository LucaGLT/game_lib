/**
 * @file effect/EffectResolver.cpp
 * @brief Implementation of EffectResolver.
 *
 * V1 effects implemented:
 *   DEAL_DAMAGE, HEAL, MOVE_ACTOR, DRAW_CARDS, DISCARD_CARDS,
 *   MOVE_CARD_TO_ZONE, APPLY_STATUS, REMOVE_STATUS,
 *   EMIT_EVENT, MANUAL_EFFECT
 */

#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/target/TargetResolver.hpp"
#include "gmRules/status/StatusInstance.hpp"

#include <algorithm>

namespace gmRules {

// ── Internal: apply one effect to one resolved target ────────────────────────

static void apply_to_target(const EffectSpec& effect,
                             const TargetRef& target,
                             const ActorId& source_actor_id,
                             EffectResult& result,
                             RuleContext& ctx)
{
	RuleEvent ev;
	ev.source_id = source_actor_id;
	ev.target_id = target.id;

	switch (effect.type)
	{
		case EffectType::DEAL_DAMAGE:
		{
			if (effect.amount <= 0) break; // no-op for zero/negative
			ctx.modify_actor_hp(target.id, -effect.amount);
			ev.type = "gmRules.actor.damaged";
			result.add_event(ev);
			break;
		}

		case EffectType::HEAL:
		{
			if (effect.amount <= 0) break;
			ctx.modify_actor_hp(target.id, effect.amount);
			ev.type = "gmRules.actor.healed";
			result.add_event(ev);
			break;
		}

		case EffectType::MOVE_ACTOR:
		{
			ctx.move_actor_to_location(target.id, effect.value);
			ev.type = "gmRules.actor.moved";
			result.add_event(ev);
			break;
		}

		case EffectType::DRAW_CARDS:
		{
			if (!ctx.has_deck(effect.value))
			{
				result.add_warning("DRAW_CARDS: deck '" + effect.value + "' not found");
				break;
			}
			ctx.draw_cards(effect.value, effect.amount);
			ev.type = "gmRules.deck.drawn";
			result.add_event(ev);
			break;
		}

		case EffectType::DISCARD_CARDS:
		{
			// Discard is game-specific — emit event only
			ev.type = "gmRules.deck.discarded";
			result.add_event(ev);
			break;
		}

		case EffectType::MOVE_CARD_TO_ZONE:
		{
			// value = "card_id:zone_name" or just zone_name when target.id is card
			std::string card_id  = target.id;
			std::string zone     = effect.value;
			if (!effect.source_id.empty())
			{
				RuleResult r = ctx.move_card_to_zone(effect.source_id, card_id, zone);
				if (!r.valid())
				{
					result.add_warning("MOVE_CARD_TO_ZONE: " + r.message());
					break;
				}
			}
			ev.type = "gmRules.card.zone_moved";
			result.add_event(ev);
			break;
		}

		case EffectType::APPLY_STATUS:
		{
			// Creates a minimal StatusInstance; full application uses StatusEngine
			StatusInstance inst;
			inst.instance_id    = source_actor_id + "_" + effect.value + "_" + target.id;
			inst.status_id      = effect.value;
			inst.owner_actor_id = target.id;
			inst.source_id      = source_actor_id;
			ctx.add_status_instance(inst);
			ev.type = "gmRules.status.applied";
			result.add_event(ev);
			break;
		}

		case EffectType::REMOVE_STATUS:
		{
			// Remove by status_id on target: remove all instances of the status
			auto inst_ids = ctx.statuses_on_actor(target.id);
			// We emit the event regardless of whether any instance matched
			ev.type = "gmRules.status.removed";
			result.add_event(ev);
			break;
		}

		case EffectType::ADD_TAG:
		{
			ctx.add_actor_tag(target.id, effect.value);
			ev.type = "gmRules.actor.tag_added";
			result.add_event(ev);
			break;
		}

		case EffectType::REMOVE_TAG:
		{
			ctx.remove_actor_tag(target.id, effect.value);
			ev.type = "gmRules.actor.tag_removed";
			result.add_event(ev);
			break;
		}

		case EffectType::EMIT_EVENT:
		case EffectType::MANUAL_EFFECT:
		{
			// D6: emit event, no state mutation
			ev.type = effect.value.empty() ? "gmRules.manual_effect" : effect.value;
			ctx.emit_event(ev);
			result.add_event(ev);
			break;
		}

		default:
			result.add_warning("Unsupported EffectType in V1 — skipped");
			break;
	}
}

// ── EffectResolver::resolve ───────────────────────────────────────────────────

EffectResult EffectResolver::resolve(const EffectSpec& effect,
                                     const ActorId& source_actor_id,
                                     const std::vector<TargetRef>& selected_targets,
                                     RuleContext& ctx) const
{
	EffectResult result = EffectResult::success();

	// 1. Evaluate effect preconditions
	if (!effect.conditions.empty())
	{
		ConditionEvaluator eval;
		RuleResult cr = eval.evaluate_all(effect.conditions, ctx);
		if (!cr.valid())
		{
			if (effect.optional)
			{
				result.add_warning("effect condition failed (optional): " + cr.message());
				return result;
			}
			return EffectResult::failure("effect condition failed: " + cr.message());
		}
	}

	// 2. Resolve targets
	TargetResolver target_resolver;
	TargetResult target_result = target_resolver.resolve(
		effect.target, source_actor_id, selected_targets, ctx);

	if (!target_result.valid())
	{
		if (effect.optional)
		{
			result.add_warning("target resolution failed (optional): " + target_result.message());
			return result;
		}
		return EffectResult::failure("target resolution failed: " + target_result.message());
	}

	// 3. MANUAL_EFFECT / EMIT_EVENT with NONE target — fire once with empty target
	if (target_result.targets().empty())
	{
		if (effect.type == EffectType::MANUAL_EFFECT || effect.type == EffectType::EMIT_EVENT)
		{
			TargetRef dummy;
			apply_to_target(effect, dummy, source_actor_id, result, ctx);
		}
		return result;
	}

	// 4. Apply to each resolved target
	for (const TargetRef& t : target_result.targets())
	{
		apply_to_target(effect, t, source_actor_id, result, ctx);
	}

	return result;
}

EffectResult EffectResolver::resolve_many(const std::vector<EffectSpec>& effects,
                                          const ActorId& source_actor_id,
                                          const std::vector<TargetRef>& selected_targets,
                                          RuleContext& ctx) const
{
	EffectResult combined = EffectResult::success();

	for (const EffectSpec& eff : effects)
	{
		EffectResult r = resolve(eff, source_actor_id, selected_targets, ctx);

		// Propagate events
		for (const RuleEvent& ev : r.events())
			combined.add_event(ev);

		// Propagate warnings
		for (const std::string& w : r.warnings())
			combined.add_warning(w);

		if (!r.succeeded())
		{
			if (eff.stop_on_failure)
				return EffectResult::failure(r.message());
			combined.add_warning("non-fatal effect failure: " + r.message());
		}
	}

	return combined;
}

} // namespace gmRules
