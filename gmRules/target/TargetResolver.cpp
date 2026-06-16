/**
 * @file target/TargetResolver.cpp
 * @brief Implementation of TargetResolver::resolve().
 *
 * V1 selectors implemented:
 *   SELF, SELECTED_ACTOR, SELECTED_ALLY, SELECTED_ENEMY,
 *   ALL_ALLIES_IN_LOCATION, ALL_ENEMIES_IN_LOCATION, LOCATION, MANUAL
 *
 * Range types:
 *   NONE, SAME_LOCATION, ADJACENT_LOCATION, WITHIN_N_LOCATIONS, GLOBAL
 *
 * Filters:
 *   required_tags, forbidden_tags, allow_self
 */

#include "gmRules/target/TargetResolver.hpp"

#include <algorithm>

namespace gmRules {

// ── Internal helpers ──────────────────────────────────────────────────────────

static bool actor_passes_tags(const ActorId& actor_id,
                               const TargetSpec& spec,
                               const RuleContext& ctx)
{
	for (const std::string& tag : spec.required_tags)
	{
		if (!ctx.actor_has_tag(actor_id, tag)) return false;
	}
	for (const std::string& tag : spec.forbidden_tags)
	{
		if (ctx.actor_has_tag(actor_id, tag)) return false;
	}
	return true;
}

static bool actor_in_range(const ActorId& actor_id,
                            const ActorId& source_id,
                            const TargetSpec& spec,
                            const RuleContext& ctx)
{
	if (spec.range_type == RangeType::NONE || spec.range_type == RangeType::GLOBAL)
		return true;

	LocationId src_loc = ctx.actor_location(source_id);
	LocationId tgt_loc = ctx.actor_location(actor_id);

	if (spec.range_type == RangeType::SAME_LOCATION)
		return src_loc == tgt_loc;

	if (spec.range_type == RangeType::ADJACENT_LOCATION)
		return ctx.are_locations_adjacent(src_loc, tgt_loc);

	if (spec.range_type == RangeType::WITHIN_N_LOCATIONS)
	{
		int dist = ctx.distance_between_locations(src_loc, tgt_loc);
		return dist >= 0 && dist <= spec.range_value;
	}

	return true; // ANY_VISIBLE_LOCATION — game-specific; allow by default
}

// ── TargetResolver::resolve ───────────────────────────────────────────────────

TargetResult TargetResolver::resolve(const TargetSpec& spec,
                                     const ActorId& source_actor_id,
                                     const std::vector<TargetRef>& selected_targets,
                                     const RuleContext& ctx) const
{
	if (spec.kind == TargetKind::NONE)
	{
		return TargetResult::success({});
	}

	std::vector<TargetRef> result;

	switch (spec.selector)
	{
		case TargetSelector::SELF:
		{
			if (!ctx.has_actor(source_actor_id))
				return TargetResult::failure("SELF: source actor not found: " + source_actor_id);
			TargetRef ref;
			ref.kind = TargetKind::ACTOR;
			ref.id   = source_actor_id;
			if (actor_passes_tags(source_actor_id, spec, ctx))
				result.push_back(ref);
			break;
		}

		case TargetSelector::SOURCE:
		{
			TargetRef ref;
			ref.kind = TargetKind::ACTOR;
			ref.id   = source_actor_id;
			result.push_back(ref);
			break;
		}

		case TargetSelector::MANUAL:
		case TargetSelector::SELECTED_ACTOR:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind != TargetKind::ACTOR) continue;
				if (!ctx.has_actor(t.id)) continue;
				if (!spec.allow_self && t.id == source_actor_id) continue;
				if (!actor_passes_tags(t.id, spec, ctx)) continue;
				if (!actor_in_range(t.id, source_actor_id, spec, ctx)) continue;
				result.push_back(t);
			}
			break;
		}

		case TargetSelector::SELECTED_ALLY:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind != TargetKind::ACTOR) continue;
				if (!ctx.has_actor(t.id)) continue;
				if (!ctx.are_allies(source_actor_id, t.id)) continue;
				if (!spec.allow_self && t.id == source_actor_id) continue;
				if (!actor_passes_tags(t.id, spec, ctx)) continue;
				if (!actor_in_range(t.id, source_actor_id, spec, ctx)) continue;
				result.push_back(t);
			}
			break;
		}

		case TargetSelector::SELECTED_ENEMY:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind != TargetKind::ACTOR) continue;
				if (!ctx.has_actor(t.id)) continue;
				if (!ctx.are_enemies(source_actor_id, t.id)) continue;
				if (!actor_passes_tags(t.id, spec, ctx)) continue;
				if (!actor_in_range(t.id, source_actor_id, spec, ctx)) continue;
				result.push_back(t);
			}
			break;
		}

		case TargetSelector::ALL_ALLIES_IN_LOCATION:
		{
			LocationId loc = ctx.actor_location(source_actor_id);
			for (const ActorId& aid : ctx.actors_in_location(loc))
			{
				if (!ctx.are_allies(source_actor_id, aid) && aid != source_actor_id) continue;
				if (!spec.allow_self && aid == source_actor_id) continue;
				if (!actor_passes_tags(aid, spec, ctx)) continue;
				if (!actor_in_range(aid, source_actor_id, spec, ctx)) continue;
				TargetRef ref;
				ref.kind = TargetKind::ACTOR;
				ref.id   = aid;
				result.push_back(ref);
			}
			break;
		}

		case TargetSelector::ALL_ENEMIES_IN_LOCATION:
		{
			LocationId loc = ctx.actor_location(source_actor_id);
			for (const ActorId& aid : ctx.actors_in_location(loc))
			{
				if (!ctx.are_enemies(source_actor_id, aid)) continue;
				if (!actor_passes_tags(aid, spec, ctx)) continue;
				if (!actor_in_range(aid, source_actor_id, spec, ctx)) continue;
				TargetRef ref;
				ref.kind = TargetKind::ACTOR;
				ref.id   = aid;
				result.push_back(ref);
			}
			break;
		}

		case TargetSelector::ALL_ACTORS_IN_LOCATION:
		{
			LocationId loc = ctx.actor_location(source_actor_id);
			for (const ActorId& aid : ctx.actors_in_location(loc))
			{
				if (!spec.allow_self && aid == source_actor_id) continue;
				if (!actor_passes_tags(aid, spec, ctx)) continue;
				TargetRef ref;
				ref.kind = TargetKind::ACTOR;
				ref.id   = aid;
				result.push_back(ref);
			}
			break;
		}

		case TargetSelector::LOCATION:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind == TargetKind::LOCATION)
					result.push_back(t);
			}
			break;
		}

		case TargetSelector::SELECTED_CARD:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind == TargetKind::CARD)
					result.push_back(t);
			}
			break;
		}

		case TargetSelector::SELECTED_ITEM:
		{
			for (const TargetRef& t : selected_targets)
			{
				if (t.kind == TargetKind::ITEM)
					result.push_back(t);
			}
			break;
		}

		default:
			return TargetResult::failure("Unsupported TargetSelector in V1");
	}

	if (spec.required && result.empty())
	{
		return TargetResult::failure("No valid targets found for required TargetSpec");
	}

	return TargetResult::success(std::move(result));
}

} // namespace gmRules
