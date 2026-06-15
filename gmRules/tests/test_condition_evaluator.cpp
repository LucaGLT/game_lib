/**
 * @file tests/test_condition_evaluator.cpp
 * @brief Unit tests for ConditionEvaluator.
 */

#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/condition/ConditionType.hpp"
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

static ConditionSpec make(ConditionType t, const std::string& sub = "",
                          const std::string& val = "", int amount = 0)
{
	ConditionSpec c;
	c.type       = t;
	c.subject_id = sub;
	c.value      = val;
	c.amount     = amount;
	return c;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_always_passes()
{
	MockRuleContext ctx;
	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ALWAYS), ctx);
	if (!r.valid()) { fail("always_passes", r.message()); return; }
	pass("always_passes");
}

static void test_never_fails()
{
	MockRuleContext ctx;
	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::NEVER), ctx);
	if (r.valid()) { fail("never_fails", "should fail"); return; }
	pass("never_fails");
}

static void test_actor_exists_pass()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");
	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_EXISTS, "hero1"), ctx);
	if (!r.valid()) { fail("actor_exists_pass", r.message()); return; }
	pass("actor_exists_pass");
}

static void test_actor_exists_fail()
{
	MockRuleContext ctx;
	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_EXISTS, "nobody"), ctx);
	if (r.valid()) { fail("actor_exists_fail", "should fail"); return; }
	pass("actor_exists_fail");
}

static void test_actor_has_status()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");
	StatusInstance inst;
	inst.instance_id    = "hero1_burn_trap";
	inst.status_id      = "burn";
	inst.owner_actor_id = "hero1";
	ctx.add_status_instance(inst);

	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_HAS_STATUS, "hero1", "burn"), ctx);
	if (!r.valid()) { fail("actor_has_status", r.message()); return; }
	pass("actor_has_status");
}

static void test_actor_has_tag()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");
	ctx.add_actor_tag("hero1", "ranger");

	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_HAS_TAG, "hero1", "ranger"), ctx);
	if (!r.valid()) { fail("actor_has_tag", r.message()); return; }
	pass("actor_has_tag");
}

static void test_actor_hp_at_or_below_pass()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 3, 10, "r1");

	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_HP_AT_OR_BELOW, "hero1", "", 5), ctx);
	if (!r.valid()) { fail("hp_at_or_below_pass", r.message()); return; }
	pass("hp_at_or_below_pass");
}

static void test_actor_hp_at_or_below_fail()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 8, 10, "r1");

	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::ACTOR_HP_AT_OR_BELOW, "hero1", "", 5), ctx);
	if (r.valid()) { fail("hp_at_or_below_fail", "should fail"); return; }
	pass("hp_at_or_below_fail");
}

static void test_actor_in_location()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 10, 10, "r1");

	ConditionEvaluator ev;
	ConditionSpec c = make(ConditionType::ACTOR_IN_LOCATION, "hero1", "r1");
	auto r = ev.evaluate(c, ctx);
	if (!r.valid()) { fail("actor_in_location", r.message()); return; }
	pass("actor_in_location");
}

static void test_location_has_tag()
{
	MockRuleContext ctx;
	ctx.add_location("dungeon");
	ctx.add_location_tag("dungeon", "dark");

	ConditionEvaluator ev;
	auto r = ev.evaluate(make(ConditionType::LOCATION_HAS_TAG, "dungeon", "dark"), ctx);
	if (!r.valid()) { fail("location_has_tag", r.message()); return; }
	pass("location_has_tag");
}

static void test_all_of_composite_pass()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 3, 10, "r1");

	ConditionSpec composite;
	composite.op = CompositeOperator::ALL_OF;
	composite.children.push_back(make(ConditionType::ACTOR_EXISTS, "hero1"));
	composite.children.push_back(make(ConditionType::ACTOR_HP_AT_OR_BELOW, "hero1", "", 5));

	ConditionEvaluator ev;
	auto r = ev.evaluate(composite, ctx);
	if (!r.valid()) { fail("all_of_pass", r.message()); return; }
	pass("all_of_pass");
}

static void test_all_of_composite_fail()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 8, 10, "r1");

	ConditionSpec composite;
	composite.op = CompositeOperator::ALL_OF;
	composite.children.push_back(make(ConditionType::ACTOR_EXISTS, "hero1"));
	composite.children.push_back(make(ConditionType::ACTOR_HP_AT_OR_BELOW, "hero1", "", 5));

	ConditionEvaluator ev;
	auto r = ev.evaluate(composite, ctx);
	if (r.valid()) { fail("all_of_fail", "should fail"); return; }
	pass("all_of_fail");
}

static void test_any_of_composite()
{
	MockRuleContext ctx;
	ctx.add_location("r1");
	ctx.add_actor("hero1", "heroes", 8, 10, "r1");

	ConditionSpec composite;
	composite.op = CompositeOperator::ANY_OF;
	composite.children.push_back(make(ConditionType::ACTOR_HP_AT_OR_BELOW, "hero1", "", 3)); // fails
	composite.children.push_back(make(ConditionType::ACTOR_EXISTS, "hero1")); // passes

	ConditionEvaluator ev;
	auto r = ev.evaluate(composite, ctx);
	if (!r.valid()) { fail("any_of", r.message()); return; }
	pass("any_of");
}

static void test_not_composite()
{
	MockRuleContext ctx;
	ConditionSpec composite;
	composite.op = CompositeOperator::NOT;
	composite.children.push_back(make(ConditionType::ACTOR_EXISTS, "nobody")); // fails → NOT passes

	ConditionEvaluator ev;
	auto r = ev.evaluate(composite, ctx);
	if (!r.valid()) { fail("not_composite", r.message()); return; }
	pass("not_composite");
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmRules: ConditionEvaluator tests ===\n\n";

	test_always_passes();
	test_never_fails();
	test_actor_exists_pass();
	test_actor_exists_fail();
	test_actor_has_status();
	test_actor_has_tag();
	test_actor_hp_at_or_below_pass();
	test_actor_hp_at_or_below_fail();
	test_actor_in_location();
	test_location_has_tag();
	test_all_of_composite_pass();
	test_all_of_composite_fail();
	test_any_of_composite();
	test_not_composite();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
