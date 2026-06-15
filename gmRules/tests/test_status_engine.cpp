/**
 * @file tests/test_status_engine.cpp
 * @brief Unit tests for StatusEngine.
 */

#include "gmRules/status/StatusEngine.hpp"
#include "gmRules/status/StatusDefinition.hpp"
#include "gmRules/status/StatusInstance.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/tests/MockRuleContext.hpp"

#include <iostream>
#include <string>

using namespace gmRules;
using namespace gmRules_test;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name) { std::cout << "[PASS] " << name << "\n"; ++g_pass; }
static void fail(const std::string& name, const std::string& r)
{ std::cout << "[FAIL] " << name << " -- " << r << "\n"; ++g_fail; }

// ── Helpers ───────────────────────────────────────────────────────────────────

static StatusDefinition make_status(const std::string& id,
                                    StackingMode mode = StackingMode::REFRESH_DURATION)
{
	StatusDefinition d;
	d.id = id;
	d.name = id;
	d.stacking_policy.mode = mode;
	d.default_duration.type = DurationType::UNTIL_REMOVED;
	return d;
}

static StatusDefinition make_status_with_on_apply_damage(const std::string& id, int damage)
{
	StatusDefinition d = make_status(id);
	EffectSpec e;
	e.type   = EffectType::DEAL_DAMAGE;
	e.amount = damage;
	e.target.kind     = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELF;
	d.on_apply.push_back(e);
	return d;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_apply_status_adds_instance()
{
	const std::string T = "apply_status_adds_instance";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("burn");
	StatusEngine engine;
	RuleResult r = engine.apply_status(def, "goblin", "trap_fire", ctx);

	if (!r.valid()) { fail(T, r.message()); return; }
	if (ctx.added_statuses.empty()) { fail(T, "no status added"); return; }
	if (ctx.added_statuses[0].status_id != "burn") { fail(T, "wrong status"); return; }
	pass(T);
}

static void test_apply_status_refresh_duration()
{
	const std::string T = "apply_status_refresh_duration";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("burn", StackingMode::REFRESH_DURATION);
	StatusEngine engine;
	engine.apply_status(def, "goblin", "trap1", ctx);
	engine.apply_status(def, "goblin", "trap2", ctx); // re-apply

	// Should have removed the old and added a new one
	bool removed_old = !ctx.removed_statuses.empty();
	if (!removed_old) { fail(T, "old instance should be removed on refresh"); return; }
	pass(T);
}

static void test_apply_status_add_stack()
{
	const std::string T = "apply_status_add_stack";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("poison", StackingMode::ADD_STACK);
	StatusEngine engine;
	engine.apply_status(def, "goblin", "src1", ctx);
	engine.apply_status(def, "goblin", "src2", ctx);

	// ADD_STACK in V1 treated like refresh: old removed, new added
	if (ctx.added_statuses.size() < 1) { fail(T, "expected at least 1 add"); return; }
	pass(T);
}

static void test_apply_status_ignore_new()
{
	const std::string T = "apply_status_ignore_new";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("shield", StackingMode::IGNORE_NEW);
	StatusEngine engine;
	engine.apply_status(def, "goblin", "src1", ctx);
	size_t after_first = ctx.added_statuses.size();
	engine.apply_status(def, "goblin", "src2", ctx); // should be ignored

	if (ctx.added_statuses.size() != after_first) { fail(T, "second apply should be ignored"); return; }
	pass(T);
}

static void test_apply_status_replace()
{
	const std::string T = "apply_status_replace";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("stun", StackingMode::REPLACE);
	StatusEngine engine;
	engine.apply_status(def, "goblin", "src1", ctx);
	engine.apply_status(def, "goblin", "src2", ctx); // replace

	if (ctx.removed_statuses.empty()) { fail(T, "old should be removed"); return; }
	pass(T);
}

static void test_on_apply_effects_resolved()
{
	const std::string T = "on_apply_effects_resolved";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status_with_on_apply_damage("burn", 2);
	StatusEngine engine;
	RuleResult r = engine.apply_status(def, "goblin", "trap", ctx);

	if (!r.valid()) { fail(T, r.message()); return; }
	if (ctx.hp("goblin") != 8) { fail(T, "on_apply should have dealt 2 damage"); return; }
	pass(T);
}

static void test_remove_status_calls_on_remove()
{
	const std::string T = "remove_status_on_remove";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 5, 10, "r1");

	StatusDefinition def = make_status("heal_on_remove");
	EffectSpec e;
	e.type   = EffectType::HEAL;
	e.amount = 3;
	e.target.kind     = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELF;
	def.on_remove.push_back(e);

	StatusEngine engine;
	engine.apply_status(def, "goblin", "src", ctx);
	std::string iid = ctx.added_statuses[0].instance_id;
	RuleResult r = engine.remove_status(iid, "goblin", def, ctx);

	if (!r.valid())            { fail(T, r.message()); return; }
	if (ctx.hp("goblin") != 8) { fail(T, "on_remove heal should give +3"); return; }
	pass(T);
}

static void test_on_activation_start_effects()
{
	const std::string T = "on_activation_start_effects";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("poison");
	EffectSpec e;
	e.type   = EffectType::DEAL_DAMAGE;
	e.amount = 1;
	e.target.kind     = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELF;
	def.on_activation_start.push_back(e);

	StatusEngine engine;
	engine.apply_status(def, "goblin", "src", ctx);
	std::vector<StatusInstance> insts = ctx.added_statuses;
	RuleResult r = engine.on_activation_start("goblin", insts, {def}, ctx);

	if (!r.valid())             { fail(T, r.message()); return; }
	if (ctx.hp("goblin") != 9)  { fail(T, "activation start should deal 1 damage"); return; }
	pass(T);
}

static void test_on_activation_end_effects()
{
	const std::string T = "on_activation_end_effects";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("regen");
	EffectSpec e;
	e.type   = EffectType::HEAL;
	e.amount = 2;
	e.target.kind     = TargetKind::ACTOR;
	e.target.selector = TargetSelector::SELF;
	def.on_activation_end.push_back(e);

	// Damage first to have something to heal
	ctx.modify_actor_hp("goblin", -4); // HP = 6

	StatusEngine engine;
	engine.apply_status(def, "goblin", "src", ctx);
	std::vector<StatusInstance> insts = ctx.added_statuses;
	RuleResult r = engine.on_activation_end("goblin", insts, {def}, ctx);

	if (!r.valid())             { fail(T, r.message()); return; }
	if (ctx.hp("goblin") != 8)  { fail(T, "activation end should heal +2 (6->8)"); return; }
	pass(T);
}

static void test_until_next_activation_expires()
{
	const std::string T = "until_next_activation_expires";
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("goblin", "monsters", 10, 10, "r1");

	StatusDefinition def = make_status("haste");
	def.default_duration.type = DurationType::UNTIL_NEXT_ACTIVATION;

	StatusEngine engine;
	engine.apply_status(def, "goblin", "src", ctx);
	std::vector<StatusInstance> insts = ctx.added_statuses;
	engine.on_activation_start("goblin", insts, {def}, ctx);

	if (ctx.removed_statuses.empty()) { fail(T, "status should expire at activation start"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmRules: StatusEngine tests ===\n\n";

	test_apply_status_adds_instance();
	test_apply_status_refresh_duration();
	test_apply_status_add_stack();
	test_apply_status_ignore_new();
	test_apply_status_replace();
	test_on_apply_effects_resolved();
	test_remove_status_calls_on_remove();
	test_on_activation_start_effects();
	test_on_activation_end_effects();
	test_until_next_activation_expires();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
