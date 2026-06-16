/**
 * @file tests/test_target_resolver.cpp
 * @brief Unit tests for TargetResolver.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmRules/core/RuleResult.cpp ^
 *       gmRules/target/TargetResult.cpp ^
 *       gmRules/target/TargetResolver.cpp ^
 *       gmRules/effect/EffectResult.cpp ^
 *       gmRules/condition/ConditionEvaluator.cpp ^
 *       gmRules/effect/EffectResolver.cpp ^
 *       gmRules/status/StatusEngine.cpp ^
 *       gmRules/facade/gmRulesEngine.cpp ^
 *       gmRules/tests/test_target_resolver.cpp ^
 *       -o bin/exe/test_gmRules_target.exe
 */

#include "gmRules/target/TargetResolver.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/tests/MockRuleContext.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace gmRules;
using namespace gmRules_test;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " -- " << reason << "\n";
	++g_fail;
}

static bool has_id(const std::vector<TargetRef>& refs, const std::string& id)
{
	for (const TargetRef& r : refs) if (r.id == id) return true;
	return false;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_self_resolves_to_source()
{
	const std::string T = "SELF_resolves_to_source";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1", "heroes", 10, 10, "room1");

	TargetSpec spec;
	spec.kind     = TargetKind::ACTOR;
	spec.selector = TargetSelector::SELF;

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {}, ctx);

	if (!result.valid())         { fail(T, result.message()); return; }
	if (result.targets().size() != 1) { fail(T, "expected 1 target"); return; }
	if (result.targets()[0].id != "hero1") { fail(T, "expected hero1"); return; }
	pass(T);
}

static void test_selected_actor_valid()
{
	const std::string T = "SELECTED_ACTOR_valid";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters", 5,  5,  "room1");

	TargetSpec spec;
	spec.kind     = TargetKind::ACTOR;
	spec.selector = TargetSelector::SELECTED_ACTOR;
	spec.range_type = RangeType::SAME_LOCATION;

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())             { fail(T, result.message()); return; }
	if (!has_id(result.targets(), "goblin")) { fail(T, "goblin missing"); return; }
	pass(T);
}

static void test_selected_ally_rejects_enemy()
{
	const std::string T = "SELECTED_ALLY_rejects_enemy";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters",  5,  5, "room1");

	TargetSpec spec;
	spec.kind     = TargetKind::ACTOR;
	spec.selector = TargetSelector::SELECTED_ALLY;
	spec.required = false; // result empty = ok

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())                { fail(T, result.message()); return; }
	if (!result.targets().empty())      { fail(T, "enemy should be rejected"); return; }
	pass(T);
}

static void test_selected_enemy_rejects_ally()
{
	const std::string T = "SELECTED_ENEMY_rejects_ally";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1", "heroes", 10, 10, "room1");
	ctx.add_actor("hero2", "heroes",  8, 10, "room1");

	TargetSpec spec;
	spec.kind     = TargetKind::ACTOR;
	spec.selector = TargetSelector::SELECTED_ENEMY;
	spec.required = false;

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "hero2";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())           { fail(T, result.message()); return; }
	if (!result.targets().empty()) { fail(T, "ally should be rejected"); return; }
	pass(T);
}

static void test_all_allies_in_location()
{
	const std::string T = "ALL_ALLIES_IN_LOCATION";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1", "heroes",  10, 10, "room1");
	ctx.add_actor("hero2", "heroes",   8, 10, "room1");
	ctx.add_actor("goblin","monsters", 5,  5, "room1");

	TargetSpec spec;
	spec.kind       = TargetKind::ACTOR_GROUP;
	spec.selector   = TargetSelector::ALL_ALLIES_IN_LOCATION;
	spec.allow_self = false;

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {}, ctx);

	if (!result.valid())               { fail(T, result.message()); return; }
	if (!has_id(result.targets(), "hero2"))  { fail(T, "hero2 missing"); return; }
	if (has_id(result.targets(), "goblin"))  { fail(T, "goblin should not appear"); return; }
	if (has_id(result.targets(), "hero1"))   { fail(T, "self excluded (allow_self=false)"); return; }
	pass(T);
}

static void test_all_enemies_in_location()
{
	const std::string T = "ALL_ENEMIES_IN_LOCATION";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1",   "heroes",   10, 10, "room1");
	ctx.add_actor("goblin1", "monsters",  5,  5, "room1");
	ctx.add_actor("goblin2", "monsters",  3,  5, "room1");

	TargetSpec spec;
	spec.kind     = TargetKind::ACTOR_GROUP;
	spec.selector = TargetSelector::ALL_ENEMIES_IN_LOCATION;

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {}, ctx);

	if (!result.valid())                    { fail(T, result.message()); return; }
	if (!has_id(result.targets(), "goblin1")) { fail(T, "goblin1 missing"); return; }
	if (!has_id(result.targets(), "goblin2")) { fail(T, "goblin2 missing"); return; }
	if (has_id(result.targets(), "hero1"))    { fail(T, "hero1 should not appear"); return; }
	pass(T);
}

static void test_range_same_location_filters()
{
	const std::string T = "range_SAME_LOCATION_filters";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_location("room2");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters",  5,  5, "room2"); // different room

	TargetSpec spec;
	spec.kind       = TargetKind::ACTOR;
	spec.selector   = TargetSelector::SELECTED_ACTOR;
	spec.range_type = RangeType::SAME_LOCATION;
	spec.required   = false;

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())           { fail(T, result.message()); return; }
	if (!result.targets().empty()) { fail(T, "goblin in different room should be excluded"); return; }
	pass(T);
}

static void test_range_adjacent_location()
{
	const std::string T = "range_ADJACENT_LOCATION";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_location("room2");
	ctx.add_adjacency("room1", "room2");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters",  5,  5, "room2");

	TargetSpec spec;
	spec.kind       = TargetKind::ACTOR;
	spec.selector   = TargetSelector::SELECTED_ACTOR;
	spec.range_type = RangeType::ADJACENT_LOCATION;

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())              { fail(T, result.message()); return; }
	if (!has_id(result.targets(), "goblin")) { fail(T, "goblin should be reachable"); return; }
	pass(T);
}

static void test_required_tags_filter()
{
	const std::string T = "required_tags_filter";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters",  5,  5, "room1");
	ctx.add_actor_tag("goblin", "elite");

	TargetSpec spec;
	spec.kind           = TargetKind::ACTOR;
	spec.selector       = TargetSelector::SELECTED_ACTOR;
	spec.required_tags  = {"elite"};

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())              { fail(T, result.message()); return; }
	if (!has_id(result.targets(), "goblin")) { fail(T, "elite goblin should pass"); return; }
	pass(T);
}

static void test_forbidden_tags_filter()
{
	const std::string T = "forbidden_tags_filter";
	MockRuleContext ctx;
	ctx.add_location("room1");
	ctx.add_actor("hero1",  "heroes",   10, 10, "room1");
	ctx.add_actor("goblin", "monsters",  5,  5, "room1");
	ctx.add_actor_tag("goblin", "shielded");

	TargetSpec spec;
	spec.kind           = TargetKind::ACTOR;
	spec.selector       = TargetSelector::SELECTED_ACTOR;
	spec.forbidden_tags = {"shielded"};
	spec.required       = false;

	TargetRef sel;
	sel.kind = TargetKind::ACTOR;
	sel.id   = "goblin";

	TargetResolver resolver;
	TargetResult result = resolver.resolve(spec, "hero1", {sel}, ctx);

	if (!result.valid())           { fail(T, result.message()); return; }
	if (!result.targets().empty()) { fail(T, "shielded goblin should be excluded"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmRules: TargetResolver tests ===\n\n";

	test_self_resolves_to_source();
	test_selected_actor_valid();
	test_selected_ally_rejects_enemy();
	test_selected_enemy_rejects_ally();
	test_all_allies_in_location();
	test_all_enemies_in_location();
	test_range_same_location_filters();
	test_range_adjacent_location();
	test_required_tags_filter();
	test_forbidden_tags_filter();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
