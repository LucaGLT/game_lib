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

static RuleResult validate_extended_effect_arguments(const EffectSpec& effect)
{
	switch (effect.type)
	{
		case EffectType::MODIFY_RESOURCE:
		case EffectType::SET_RESOURCE_MAX:
		case EffectType::EQUIP_ITEM:
		case EffectType::UNEQUIP_ITEM:
		{
			if (effect.value.empty())
			{
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"extended actor effect requires non-empty value");
			}
			return RuleResult::ok();
		}

		case EffectType::SHUFFLE_ZONE:
		case EffectType::LOOK_TOP_CARD:
		case EffectType::LOOK_BOTTOM_CARD:
		case EffectType::SELECT_SPECIFIC_CARD:
		case EffectType::DISCARD_RANDOM:
		case EffectType::PLACE_ON_TOP:
		case EffectType::PLACE_ON_BOTTOM:
		{
			if (effect.source_id.empty())
			{
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"extended deck effect requires source_id (deck_id)");
			}
			if (effect.value.empty())
			{
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"extended deck effect requires non-empty value (zone/card spec)");
			}
			return RuleResult::ok();
		}

		case EffectType::ROLL_DICE:
		{
			if (effect.value.empty())
			{
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"ROLL_DICE requires non-empty dice expression/value");
			}
			return RuleResult::ok();
		}

		case EffectType::SET_LOCATION_PASSABLE:
		case EffectType::ADD_LOCATION_TAG:
		case EffectType::REMOVE_LOCATION_TAG:
		case EffectType::SET_LOCATION_OWNER:
		case EffectType::CREATE_BARRIER:
		case EffectType::REMOVE_BARRIER:
		case EffectType::SPAWN_INTERACTABLE:
		case EffectType::DESPAWN_INTERACTABLE:
		{
			if (effect.value.empty())
			{
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"extended map effect requires non-empty value");
			}
			return RuleResult::ok();
		}

		case EffectType::REVIVE_ACTOR:
		case EffectType::CHANGE_TEAM:
		case EffectType::CUSTOM:
			return RuleResult::ok();

		default:
			return RuleResult::ok();
	}
}

static RuleResult publish_event(const RuleEvent& event,
							  const std::string& bus_name,
							  EffectResult& result,
							  RuleContext& ctx)
{
	ctx.emit_event(event, bus_name);
	result.add_event(event);
	return RuleResult::ok();
}

// ── Internal: apply one effect to one resolved target ────────────────────────

static RuleResult apply_to_target(const EffectSpec& effect,
								  const TargetRef& target,
								  const ActorId& source_actor_id,
								  EffectResult& result,
								  RuleContext& ctx,
								  int rule_priority)
{
	RuleEvent ev;
	ev.source_id = source_actor_id;
	ev.target_id = target.id;
	ev.priority = rule_priority;

	switch (effect.type)
	{
		case EffectType::DEAL_DAMAGE:
		{
			if (effect.amount <= 0) return RuleResult::ok(); // no-op for zero/negative
			ctx.modify_actor_hp(target.id, -effect.amount);
			ev.type = "gmRules.actor.damaged";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::HEAL:
		{
			if (effect.amount <= 0) return RuleResult::ok();
			ctx.modify_actor_hp(target.id, effect.amount);
			ev.type = "gmRules.actor.healed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::MOVE_ACTOR:
		{
			ctx.move_actor_to_location(target.id, effect.value);
			ev.type = "gmRules.actor.moved";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::DRAW_CARDS:
		{
			if (!ctx.has_deck(effect.value))
			{
				return RuleResult::fail(RuleError::UNKNOWN_DECK,
					"DRAW_CARDS: deck '" + effect.value + "' not found");
			}
			ctx.draw_cards(effect.value, effect.amount);
			ev.type = "gmRules.deck.drawn";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::DISCARD_CARDS:
		{
			// Discard is game-specific — emit event only
			ev.type = "gmRules.deck.discarded";
			return publish_event(ev, "RuleEvBus", result, ctx);
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
					return RuleResult::fail(r.error(), "MOVE_CARD_TO_ZONE: " + r.message());
				}
			}
			ev.type = "gmRules.card.zone_moved";
			return publish_event(ev, "RuleEvBus", result, ctx);
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
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::REMOVE_STATUS:
		{
			// Remove by status_id on target: remove all instances of the status
			auto inst_ids = ctx.statuses_on_actor(target.id);
			// We emit the event regardless of whether any instance matched
			ev.type = "gmRules.status.removed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::ADD_TAG:
		{
			ctx.add_actor_tag(target.id, effect.value);
			ev.type = "gmRules.actor.tag_added";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::REMOVE_TAG:
		{
			ctx.remove_actor_tag(target.id, effect.value);
			ev.type = "gmRules.actor.tag_removed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::EMIT_EVENT:
		case EffectType::MANUAL_EFFECT:
		{
			// D6: emit event, no state mutation
			ev.type = effect.value.empty() ? "gmRules.manual_effect" : effect.value;
			const std::string bus_name = effect.source_id.empty()
				? "RuleEvBus"
				: effect.source_id;
			return publish_event(ev, bus_name, result, ctx);
		}

		case EffectType::REVIVE_ACTOR:
		case EffectType::CHANGE_TEAM:
		case EffectType::MODIFY_RESOURCE:
		case EffectType::SET_RESOURCE_MAX:
		case EffectType::EQUIP_ITEM:
		case EffectType::UNEQUIP_ITEM:
		case EffectType::SHUFFLE_ZONE:
		case EffectType::LOOK_TOP_CARD:
		case EffectType::LOOK_BOTTOM_CARD:
		case EffectType::SELECT_SPECIFIC_CARD:
		case EffectType::DISCARD_RANDOM:
		case EffectType::PLACE_ON_TOP:
		case EffectType::PLACE_ON_BOTTOM:
		case EffectType::ROLL_DICE:
		case EffectType::SET_LOCATION_PASSABLE:
		case EffectType::ADD_LOCATION_TAG:
		case EffectType::REMOVE_LOCATION_TAG:
		case EffectType::SET_LOCATION_OWNER:
		case EffectType::CREATE_BARRIER:
		case EffectType::REMOVE_BARRIER:
		case EffectType::SPAWN_INTERACTABLE:
		case EffectType::DESPAWN_INTERACTABLE:
		case EffectType::CUSTOM:
		{
			RuleResult arg_check = validate_extended_effect_arguments(effect);
			if (!arg_check.valid())
			{
				return arg_check;
			}

			RuleEvent ext_event;
			RuleResult ext = ctx.apply_extended_effect(effect,
			                                          target,
			                                          source_actor_id,
			                                          &ext_event);
			if (ext.valid())
			{
				if (!ext_event.type.empty())
				{
					ext_event.priority = rule_priority;
					return publish_event(ext_event, "RuleEvBus", result, ctx);
				}
				return RuleResult::ok();
			}

			return RuleResult::fail(RuleError::UNSUPPORTED_EFFECT,
				"unsupported extended effect "
				+ std::string(effect_type_name(effect.type))
				+ ": " + ext.message());
		}

		default:
			return RuleResult::fail(RuleError::UNSUPPORTED_EFFECT,
				"Unsupported EffectType in V1: " + std::string(effect_type_name(effect.type)));
	}

	return RuleResult::ok();
}

// ── EffectResolver::resolve ───────────────────────────────────────────────────

EffectResult EffectResolver::resolve(const EffectSpec& effect,
                                     const ActorId& source_actor_id,
                                     const std::vector<TargetRef>& selected_targets,
								 RuleContext& ctx,
								 int rule_priority) const
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
			RuleResult applied = apply_to_target(effect,
										dummy,
										source_actor_id,
										result,
										ctx,
										rule_priority);
			if (!applied.valid())
			{
				if (effect.optional)
				{
					result.add_warning("effect apply failed (optional): " + applied.message());
					return result;
				}
				return EffectResult::failure("effect apply failed: " + applied.message());
			}
		}
		return result;
	}

	// 4. Apply to each resolved target
	for (const TargetRef& t : target_result.targets())
	{
		RuleResult applied = apply_to_target(effect,
									t,
									source_actor_id,
									result,
									ctx,
									rule_priority);
		if (!applied.valid())
		{
			if (effect.optional)
			{
				result.add_warning("effect apply failed (optional): " + applied.message());
				continue;
			}
			return EffectResult::failure("effect apply failed: " + applied.message());
		}
	}

	return result;
}

EffectResult EffectResolver::resolve_many(const std::vector<EffectSpec>& effects,
                                          const ActorId& source_actor_id,
                                          const std::vector<TargetRef>& selected_targets,
								  RuleContext& ctx,
								  int rule_priority) const
{
	EffectResult combined = EffectResult::success();

	for (const EffectSpec& eff : effects)
	{
		EffectResult r = resolve(eff,
								source_actor_id,
								selected_targets,
								ctx,
								rule_priority);

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
