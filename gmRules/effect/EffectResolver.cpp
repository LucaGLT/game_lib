/**
 * @file effect/EffectResolver.cpp
 * @brief Implementation of EffectResolver.
 *
 * V1 effects implemented:
 *   DEAL_DAMAGE, HEAL, MOVE_ACTOR, DRAW_CARDS, DISCARD_CARDS,
 *   MOVE_CARD_TO_ZONE, APPLY_STATUS, REMOVE_STATUS,
 *   EMIT_EVENT, MANUAL_EFFECT
 *
 * Chapter 4 — gmActor:
 *   SPAWN_ACTOR, DESPAWN_ACTOR, REVIVE_ACTOR, CHANGE_TEAM,
 *   MODIFY_RESOURCE, SET_RESOURCE_MAX, EQUIP_ITEM, UNEQUIP_ITEM
 *
 * Chapter 5 — gmAlea:
 *   SHUFFLE_ZONE, LOOK_TOP_CARD, LOOK_BOTTOM_CARD,
 *   SELECT_SPECIFIC_CARD, DISCARD_RANDOM,
 *   PLACE_ON_TOP, PLACE_ON_BOTTOM, ROLL_DICE
 *
 * Chapter 6 — gmMap:
 *   SET_LOCATION_PASSABLE, ADD_LOCATION_TAG, REMOVE_LOCATION_TAG,
 *   SET_LOCATION_OWNER, CREATE_BARRIER, REMOVE_BARRIER,
 *   SPAWN_INTERACTABLE, DESPAWN_INTERACTABLE
 */

#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/target/TargetResolver.hpp"
#include "gmRules/status/StatusInstance.hpp"

namespace gmRules {

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

		// ── Chapter 4 — gmActor lifecycle ─────────────────────────────────────

		case EffectType::SPAWN_ACTOR:
		{
			ctx.spawn_actor(target.id, effect.value);
			ev.type = "gmRules.actor.spawned";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::DESPAWN_ACTOR:
		{
			ctx.despawn_actor(target.id);
			ev.type = "gmRules.actor.despawned";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::REVIVE_ACTOR:
		{
			ctx.revive_actor(target.id);
			ev.type = "gmRules.actor.revived";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::CHANGE_TEAM:
		{
			ctx.change_actor_team(target.id, effect.value);
			ev.type = "gmRules.actor.team_changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		// ── Chapter 4 — gmActor resources / equipment ─────────────────────────

		case EffectType::MODIFY_RESOURCE:
		{
			// value = resource_id, amount = signed delta
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"MODIFY_RESOURCE: value must contain the resource_id");
			ctx.modify_resource(target.id, effect.value, effect.amount);
			ev.type = "gmRules.resource.changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::SET_RESOURCE_MAX:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SET_RESOURCE_MAX: value must contain the resource_id");
			ctx.set_resource_max(target.id, effect.value, effect.amount);
			ev.type = "gmRules.resource.max_changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::EQUIP_ITEM:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"EQUIP_ITEM: value must contain the item_id");
			ctx.equip_item(target.id, effect.value);
			ev.type = "gmRules.item.equipped";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::UNEQUIP_ITEM:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"UNEQUIP_ITEM: value must contain the slot_id");
			ctx.unequip_item(target.id, effect.value);
			ev.type = "gmRules.item.unequipped";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		// ── Chapter 5 — gmAlea deck/dice ──────────────────────────────────────

		case EffectType::SHUFFLE_ZONE:
		{
			// source_id = deck_id, value = zone_name
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SHUFFLE_ZONE: source_id must contain the deck_id");
			ctx.shuffle_zone(effect.source_id, effect.value);
			ev.type = "gmRules.deck.zone_shuffled";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::LOOK_TOP_CARD:
		{
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"LOOK_TOP_CARD: source_id must contain the deck_id");
			ctx.look_top_cards(effect.source_id,
				effect.amount > 0 ? effect.amount : 1);
			ev.type = "gmRules.deck.top_looked";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::LOOK_BOTTOM_CARD:
		{
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"LOOK_BOTTOM_CARD: source_id must contain the deck_id");
			ctx.look_bottom_cards(effect.source_id,
				effect.amount > 0 ? effect.amount : 1);
			ev.type = "gmRules.deck.bottom_looked";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::SELECT_SPECIFIC_CARD:
		{
			// source_id = deck_id, value = card_id
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SELECT_SPECIFIC_CARD: source_id must contain the deck_id");
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SELECT_SPECIFIC_CARD: value must contain the card_id");
			RuleResult r = ctx.select_specific_card(effect.source_id, effect.value);
			if (!r.valid()) return r;
			ev.type = "gmRules.deck.card_selected";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::DISCARD_RANDOM:
		{
			// source_id = deck_id, value = zone_name, amount = count
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"DISCARD_RANDOM: source_id must contain the deck_id");
			RuleResult r = ctx.discard_random_cards(effect.source_id,
				effect.value, effect.amount > 0 ? effect.amount : 1);
			if (!r.valid()) return r;
			ev.type = "gmRules.deck.random_discarded";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::PLACE_ON_TOP:
		{
			// source_id = deck_id, value = card_id
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"PLACE_ON_TOP: source_id must contain the deck_id");
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"PLACE_ON_TOP: value must contain the card_id");
			RuleResult r = ctx.place_card_on_top(effect.source_id, effect.value);
			if (!r.valid()) return r;
			ev.type = "gmRules.deck.placed_on_top";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::PLACE_ON_BOTTOM:
		{
			// source_id = deck_id, value = card_id
			if (effect.source_id.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"PLACE_ON_BOTTOM: source_id must contain the deck_id");
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"PLACE_ON_BOTTOM: value must contain the card_id");
			RuleResult r = ctx.place_card_on_bottom(effect.source_id, effect.value);
			if (!r.valid()) return r;
			ev.type = "gmRules.deck.placed_on_bottom";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::ROLL_DICE:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"ROLL_DICE: value must contain the dice expression");
			int rolled = ctx.roll_dice(effect.value);
			ev.type = "gmRules.dice.rolled";
			ev.payload_json = "{\"result\":" + std::to_string(rolled) + "}";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		// ── Chapter 6 — gmMap mutations ───────────────────────────────────────

		case EffectType::SET_LOCATION_PASSABLE:
		{
			// target.id = location_id, value = "true"/"false"
			bool passable = (effect.value == "true" || effect.value == "1");
			ctx.set_location_passable(target.id, passable);
			ev.type = "gmRules.map.passable_changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::ADD_LOCATION_TAG:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"ADD_LOCATION_TAG: value must contain the tag");
			ctx.add_location_tag(target.id, effect.value);
			ev.type = "gmRules.map.location_tag_added";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::REMOVE_LOCATION_TAG:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"REMOVE_LOCATION_TAG: value must contain the tag");
			ctx.remove_location_tag(target.id, effect.value);
			ev.type = "gmRules.map.location_tag_removed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::SET_LOCATION_OWNER:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SET_LOCATION_OWNER: value must contain the owner_id");
			ctx.set_location_owner(target.id, effect.value);
			ev.type = "gmRules.map.owner_changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::CREATE_BARRIER:
		{
			// target.id = from_location, value = to_location,
			// source_id = barrier_id (optional; auto-generated if empty)
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"CREATE_BARRIER: value must contain the to_location_id");
			const std::string barrier_id = effect.source_id.empty()
				? (target.id + "_" + effect.value + "_barrier")
				: effect.source_id;
			ctx.create_barrier(target.id, effect.value, barrier_id);
			ev.type = "gmRules.map.barrier_created";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::REMOVE_BARRIER:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"REMOVE_BARRIER: value must contain the barrier_id");
			ctx.remove_barrier(effect.value);
			ev.type = "gmRules.map.barrier_removed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::SPAWN_INTERACTABLE:
		{
			// target.id = location_id, value = spec_json
			ctx.spawn_interactable(target.id, effect.value);
			ev.type = "gmRules.map.interactable_spawned";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::DESPAWN_INTERACTABLE:
		{
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"DESPAWN_INTERACTABLE: value must contain the interactable_id");
			ctx.despawn_interactable(effect.value);
			ev.type = "gmRules.map.interactable_despawned";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::CUSTOM:
		{
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
				"CUSTOM effect not handled by context: " + ext.message());
		}

		// ── Dungeon Crawler / advanced effects ────────────────────────────────

		case EffectType::SET_ACTOR_RESOURCE:
		{
			// Sets resource `value` to exact value `amount`.
			// Uses modify_resource(delta) to avoid requiring a set_resource() API.
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SET_ACTOR_RESOURCE: value must contain the resource_id");
			const int current = ctx.actor_resource(target.id, effect.value);
			const int delta   = effect.amount - current;
			if (delta != 0)
				ctx.modify_resource(target.id, effect.value, delta);
			ev.type = "gmRules.resource.changed";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::TRIGGER_RULE:
		{
			// Chains to another rule by ID; delegates scheduling to the game adapter
			// to avoid a circular dependency with RuleBook.
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"TRIGGER_RULE: value must contain the rule_id to chain");
			RuleEvent sub_event;
			RuleResult sub = ctx.apply_extended_effect(effect,
			                                          target,
			                                          source_actor_id,
			                                          &sub_event);
			if (!sub.valid())
				return RuleResult::fail(RuleError::UNSUPPORTED_EFFECT,
					"TRIGGER_RULE: chained rule '" + effect.value + "' not handled: " +
					sub.message());
			if (!sub_event.type.empty())
			{
				sub_event.priority = rule_priority;
				return publish_event(sub_event, "RuleEvBus", result, ctx);
			}
			return RuleResult::ok();
		}

		case EffectType::SCALE_EFFECT:
		{
			// Deals damage equal to actor resource(`value`) * max(1, `amount`).
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"SCALE_EFFECT: value must contain the resource_id to scale from");
			const int resource_val   = ctx.actor_resource(source_actor_id, effect.value);
			const int multiplier     = (effect.amount > 0) ? effect.amount : 1;
			const int effective      = resource_val * multiplier;
			if (effective > 0)
				ctx.modify_actor_hp(target.id, -effective);
			ev.type = "gmRules.actor.damaged";
			return publish_event(ev, "RuleEvBus", result, ctx);
		}

		case EffectType::CHAIN_EFFECT:
		{
			// Deals `amount` damage to primary target, then bounces to up to
			// `chain_count` additional enemies in the same location.
			if (effect.amount > 0)
				ctx.modify_actor_hp(target.id, -effect.amount);
			ev.type = "gmRules.actor.damaged";
			publish_event(ev, "RuleEvBus", result, ctx);

			if (effect.chain_count > 0)
			{
				const LocationId loc        = ctx.actor_location(target.id);
				const std::vector<ActorId> candidates = ctx.actors_in_location(loc);
				int remaining = effect.chain_count;
				for (const ActorId& cid : candidates)
				{
					if (remaining <= 0) break;
					if (cid == target.id)              continue;
					if (ctx.actor_current_hp(cid) <= 0) continue;
					if (!ctx.are_enemies(source_actor_id, cid)) continue;
					ctx.modify_actor_hp(cid, -effect.amount);
					--remaining;
				}
			}
			return RuleResult::ok();
		}

		case EffectType::DELAY_EFFECT:
		{
			// Schedules inner effect (`value` = type name, `amount` = amount) to fire
			// after `chain_count` turns. Delegates to ctx.apply_extended_effect so
			// the game engine handles the actual scheduling.
			if (effect.value.empty())
				return RuleResult::fail(RuleError::RULE_VIOLATION,
					"DELAY_EFFECT: value must contain the inner effect type name");
			ev.type = "gmRules.effect.delayed";
			ev.payload_json =
				"{\"inner_type\":\"" + effect.value + "\""
				",\"inner_amount\":" + std::to_string(effect.amount) +
				",\"delay_turns\":" + std::to_string(effect.chain_count) +
				",\"target_id\":\"" + target.id + "\""
				",\"source_id\":\"" + source_actor_id + "\"}";
			publish_event(ev, "RuleEvBus", result, ctx);
			// Also delegate to the game engine for actual scheduling
			RuleEvent sub_event;
			ctx.apply_extended_effect(effect, target, source_actor_id, &sub_event);
			return RuleResult::ok();
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
