/**
 * @file tests/test_rules_integration.cpp
 * @brief Integration tests for gmRulesEngine facade.
 */

#include "gmRules/facade/gmRulesEngine.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/condition/ConditionType.hpp"
#include "gmRules/status/StatusDefinition.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/tests/MockRuleContext.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace gmRules;
using namespace gmRules_test;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name) { std::cout << "[PASS] " << name << "\n"; ++g_pass; }
static void fail(const std::string& name, const std::string& r)
{ std::cout << "[FAIL] " << name << " -- " << r << "\n"; ++g_fail; }

static TargetRef actor_ref(const std::string& id)
{
	TargetRef r;
	r.kind = TargetKind::ACTOR;
	r.id   = id;
	return r;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_card_like_effect_target_condition_damage()
{
	const std::string T = "card_like_target_condition_damage";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1", "heroes", 10, 10, "room1");
	ctx.add_actor("goblin", "monsters", 6, 6, "room1");

	EffectSpec e;
	e.type = EffectType::DEAL_DAMAGE;
	e.amount = 2;
	e.target.kind = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELECTED_ENEMY;
	e.target.range_type = RangeType::SAME_LOCATION;

	ConditionSpec c;
	c.type       = ConditionType::ACTOR_EXISTS;
	c.subject_id = "hero1";
	e.conditions.push_back(c);

	gmRulesEngine engine;
	EffectResult r = engine.resolve_effect(e, "hero1", {actor_ref("goblin")}, ctx);

	if (!r.succeeded()) { fail(T, r.message()); return; }
	if (ctx.hp("goblin") != 4) { fail(T, "expected goblin HP 4"); return; }
	pass(T);
}

static void test_status_damage_on_activation_start()
{
	const std::string T = "status_damage_on_activation_start";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("goblin", "monsters", 10, 10, "room1");

	StatusDefinition poison;
	poison.id = "poison";
	poison.name = "Poison";
	EffectSpec tick;
	tick.type = EffectType::DEAL_DAMAGE;
	tick.amount = 1;
	tick.target.kind = TargetKind::ACTOR;
	tick.target.selector = TargetSelector::SELF;
	poison.on_activation_start.push_back(tick);

	gmRulesEngine engine;
	RuleResult ar = engine.apply_status(poison, "goblin", "trap", ctx);
	if (!ar.valid()) { fail(T, ar.message()); return; }

	StatusEngine se;
	RuleResult sr = se.on_activation_start("goblin", ctx.added_statuses, {poison}, ctx);
	if (!sr.valid()) { fail(T, sr.message()); return; }
	if (ctx.hp("goblin") != 9) { fail(T, "poison tick should deal 1"); return; }
	pass(T);
}

static void test_move_effect_validates_location()
{
	const std::string T = "move_effect_validates_location";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_location("r2");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e;
	e.type = EffectType::MOVE_ACTOR;
	e.value = "r2";
	e.target.kind = TargetKind::ACTOR;
	e.target.selector = TargetSelector::MANUAL;

	gmRulesEngine engine;
	EffectResult r = engine.resolve_effect(e, "hero1", {actor_ref("hero1")}, ctx);

	if (!r.succeeded()) { fail(T, r.message()); return; }
	if (ctx.location("hero1") != "r2") { fail(T, "hero should be moved to r2"); return; }
	pass(T);
}

static void test_draw_effect_validates_deck_adapter()
{
	const std::string T = "draw_effect_validates_deck_adapter";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");
	ctx.add_deck("hero_deck", {"c1", "c2", "c3"});

	EffectSpec e;
	e.type = EffectType::DRAW_CARDS;
	e.amount = 2;
	e.value = "hero_deck";
	e.target.kind = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELF;

	gmRulesEngine engine;
	EffectResult r = engine.resolve_effect(e, "hero1", {}, ctx);

	if (!r.succeeded()) { fail(T, r.message()); return; }
	pass(T);
}

static void test_manual_effect_produces_event()
{
	const std::string T = "manual_effect_produces_event";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	EffectSpec e;
	e.type = EffectType::MANUAL_EFFECT;
	e.value = "custom.manual";
	e.target.kind = TargetKind::NONE;
	e.target.selector = TargetSelector::MANUAL;
	e.target.required = false;

	gmRulesEngine engine;
	EffectResult r = engine.resolve_effect(e, "hero1", {}, ctx);

	if (!r.succeeded()) { fail(T, r.message()); return; }
	if (r.events().empty()) { fail(T, "manual effect should emit event"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmRules: Integration tests ===\n\n";

	test_card_like_effect_target_condition_damage();
	test_status_damage_on_activation_start();
	test_move_effect_validates_location();
	test_draw_effect_validates_deck_adapter();
	test_manual_effect_produces_event();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
