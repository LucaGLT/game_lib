/**
 * @file tests/test_effect_resolver.cpp
 * @brief Unit tests for EffectResolver.
 */

#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/tests/MockRuleContext.hpp"

#include <iostream>
#include <string>
#include <algorithm>

using namespace gmRules;
using namespace gmRules_test;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name) { std::cout << "[PASS] " << name << "\n"; ++g_pass; }
static void fail(const std::string& name, const std::string& r)
{ std::cout << "[FAIL] " << name << " -- " << r << "\n"; ++g_fail; }

static EffectSpec make_effect(EffectType type, TargetSelector sel,
                               int amount = 0, const std::string& value = "")
{
	EffectSpec e;
	e.type         = type;
	e.amount       = amount;
	e.value        = value;
	e.target.kind  = TargetKind::ACTOR;
	e.target.selector = sel;
	return e;
}

static TargetRef actor_ref(const std::string& id)
{
	TargetRef r;
	r.kind = TargetKind::ACTOR;
	r.id   = id;
	return r;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_deal_damage()
{
	const std::string T = "deal_damage";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::DEAL_DAMAGE, TargetSelector::MANUAL, 4);
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {actor_ref("goblin")}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	if (ctx.hp("goblin") != 6) { fail(T, "expected 6 HP"); return; }
	pass(T);
}

static void test_heal_increases_hp()
{
	const std::string T = "heal_increases_hp";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 4, 10, "r1");

	EffectSpec e = make_effect(EffectType::HEAL, TargetSelector::MANUAL, 3);
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "healer", {actor_ref("hero1")}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	if (ctx.hp("hero1") != 7) { fail(T, "expected 7 HP"); return; }
	pass(T);
}

static void test_move_actor()
{
	const std::string T = "move_actor";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_location("r2");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::MOVE_ACTOR, TargetSelector::MANUAL, 0, "r2");
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {actor_ref("hero1")}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	if (ctx.location("hero1") != "r2") { fail(T, "expected location r2"); return; }
	pass(T);
}

static void test_draw_cards()
{
	const std::string T = "draw_cards";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");
	ctx.add_deck("deck1", {"c1", "c2", "c3"});

	EffectSpec e = make_effect(EffectType::DRAW_CARDS, TargetSelector::SELF, 2, "deck1");
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	pass(T);
}

static void test_apply_status_creates_instance()
{
	const std::string T = "apply_status_creates_instance";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::APPLY_STATUS, TargetSelector::MANUAL, 0, "burn");
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "trap", {actor_ref("goblin")}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	if (ctx.added_statuses.empty()) { fail(T, "no status was added"); return; }
	if (ctx.added_statuses[0].status_id != "burn") { fail(T, "wrong status_id"); return; }
	pass(T);
}

static void test_remove_status_emits_event()
{
	const std::string T = "remove_status_emits_event";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::REMOVE_STATUS, TargetSelector::MANUAL, 0, "burn");
	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "healer", {actor_ref("goblin")}, ctx);

	if (!result.succeeded()) { fail(T, result.message()); return; }
	bool found = false;
	for (const RuleEvent& ev : result.events())
		if (ev.type == "gmRules.status.removed") found = true;
	if (!found) { fail(T, "remove event not emitted"); return; }
	pass(T);
}

static void test_manual_effect_does_not_mutate()
{
	const std::string T = "manual_effect_no_mutation";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e;
	e.type  = EffectType::MANUAL_EFFECT;
	e.value = "custom.magic_explosion";
	e.target.kind     = TargetKind::NONE;
	e.target.selector = TargetSelector::MANUAL;
	e.target.required = false;

	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {}, ctx);

	if (!result.succeeded())    { fail(T, result.message()); return; }
	if (ctx.hp("hero1") != 10)  { fail(T, "hp should not change"); return; }
	if (result.events().empty()) { fail(T, "manual effect should emit event"); return; }
	pass(T);
}

static void test_stop_on_failure()
{
	const std::string T = "stop_on_failure";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	// Two effects: first requires unknown actor (fails), second heals goblin
	EffectSpec e1 = make_effect(EffectType::DEAL_DAMAGE, TargetSelector::MANUAL, 5);
	e1.stop_on_failure = true;
	ConditionSpec cond;
	cond.type       = ConditionType::ACTOR_EXISTS;
	cond.subject_id = "nobody";
	e1.conditions.push_back(cond);

	EffectSpec e2 = make_effect(EffectType::HEAL, TargetSelector::MANUAL, 3);
	e2.stop_on_failure = true;

	EffectResolver resolver;
	EffectResult result = resolver.resolve_many({e1, e2}, "hero1", {actor_ref("goblin")}, ctx);

	if (result.succeeded())     { fail(T, "should fail"); return; }
	if (ctx.hp("goblin") != 10) { fail(T, "second effect should not have run"); return; }
	pass(T);
}

static void test_optional_failure_becomes_warning()
{
	const std::string T = "optional_failure_becomes_warning";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	EffectSpec e1 = make_effect(EffectType::DEAL_DAMAGE, TargetSelector::MANUAL, 5);
	e1.optional = true;
	ConditionSpec cond;
	cond.type       = ConditionType::ACTOR_EXISTS;
	cond.subject_id = "nobody";
	e1.conditions.push_back(cond);

	EffectSpec e2 = make_effect(EffectType::DEAL_DAMAGE, TargetSelector::MANUAL, 2);

	EffectResolver resolver;
	EffectResult result = resolver.resolve_many({e1, e2}, "hero1", {actor_ref("goblin")}, ctx);

	if (!result.succeeded())    { fail(T, "should succeed (optional failure)"); return; }
	if (result.warnings().empty()) { fail(T, "expected a warning"); return; }
	if (ctx.hp("goblin") != 8)  { fail(T, "second effect should have run (hp 10-2=8)"); return; }
	pass(T);
}

static void test_extended_effect_requires_runtime_support()
{
	// CUSTOM still routes through apply_extended_effect.
	// Without allow_extended_effects the mock refuses and EffectResolver fails.
	const std::string T = "extended_effect_requires_runtime_support";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::CUSTOM, TargetSelector::SELF, 0, "game_specific");

	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {}, ctx);

	if (result.succeeded())
	{
		fail(T, "should fail without runtime extended-effect support");
		return;
	}
	if (result.message().find("CUSTOM") == std::string::npos)
	{
		fail(T, "failure message should mention CUSTOM effect");
		return;
	}
	pass(T);
}

static void test_optional_extended_effect_becomes_warning()
{
	// CUSTOM optional effect: mock refuses -> becomes warning; second effect runs.
	const std::string T = "optional_extended_effect_becomes_warning";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e1 = make_effect(EffectType::CUSTOM, TargetSelector::SELF, 0, "game_specific");
	e1.optional = true;

	EffectSpec e2 = make_effect(EffectType::DEAL_DAMAGE, TargetSelector::SELF, 2);

	EffectResolver resolver;
	EffectResult result = resolver.resolve_many({e1, e2}, "hero1", {}, ctx);

	if (!result.succeeded())
	{
		fail(T, "should succeed because first effect is optional");
		return;
	}
	if (result.warnings().empty())
	{
		fail(T, "expected warning for optional unsupported extended effect");
		return;
	}
	if (ctx.hp("hero1") != 8)
	{
		fail(T, "second effect should run and reduce hp to 8");
		return;
	}
	pass(T);
}

static void test_extended_effect_delegates_to_runtime()
{
	// ROLL_DICE is now a first-class effect; emits "gmRules.dice.rolled".
	const std::string T = "extended_effect_delegates_to_runtime";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::ROLL_DICE, TargetSelector::SELF, 0, "1d6");

	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {}, ctx);

	if (!result.succeeded())
	{
		fail(T, result.message());
		return;
	}
	if (result.events().empty())
	{
		fail(T, "expected emitted event from ROLL_DICE handler");
		return;
	}
	if (result.events()[0].type != "gmRules.dice.rolled")
	{
		fail(T, "unexpected event type: " + result.events()[0].type);
		return;
	}
	pass(T);
}

static void test_extended_effect_missing_required_arguments()
{
	// SHUFFLE_ZONE without source_id (deck_id) must fail with a validation error.
	const std::string T = "extended_effect_missing_required_arguments";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e = make_effect(EffectType::SHUFFLE_ZONE, TargetSelector::SELF, 0, "discard");
	// source_id (deck_id) intentionally missing

	EffectResolver resolver;
	EffectResult result = resolver.resolve(e, "hero1", {}, ctx);

	if (result.succeeded())
	{
		fail(T, "should fail with semantic validation error");
		return;
	}
	if (result.message().find("source_id") == std::string::npos)
	{
		fail(T, "missing source_id validation message not found, got: " + result.message());
		return;
	}
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmRules: EffectResolver tests ===\n\n";

	test_deal_damage();
	test_heal_increases_hp();
	test_move_actor();
	test_draw_cards();
	test_apply_status_creates_instance();
	test_remove_status_emits_event();
	test_manual_effect_does_not_mutate();
	test_stop_on_failure();
	test_optional_failure_becomes_warning();
	test_extended_effect_requires_runtime_support();
	test_optional_extended_effect_becomes_warning();
	test_extended_effect_delegates_to_runtime();
	test_extended_effect_missing_required_arguments();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
