/**
 * @file tests/test_rule_book.cpp
 * @brief Unit tests for RuleBook, RuleBookLoader, and gmRulesEngine::resolve_rule.
 *
 * Tests are self-contained: JSON is provided as inline strings so no
 * file system access is required.
 *
 * Test groups:
 *   1. RuleBook registration / query
 *   2. RuleBookLoader::load_json_string — valid JSON
 *   3. RuleBookLoader::load_json_string — error handling
 *   4. RuleBook::resolve_rule with MockRuleContext
 *   5. RuleBook::resolve_rules (sequence)
 *   6. gmRulesEngine::load_rules_json_string + resolve_rule
 */

#include "gmRules/core/RuleBook.hpp"
#include "gmRules/core/RuleDefinition.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/loader/RuleBookLoader.hpp"
#include "gmRules/facade/gmRulesEngine.hpp"
#include "MockRuleContext.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static void PASS(const char* name)
{
	std::cout << "  [PASS] " << name << "\n";
}

static void FAIL(const char* name, const std::string& reason)
{
	std::cerr << "  [FAIL] " << name << ": " << reason << "\n";
	std::exit(1);
}

#define ASSERT(cond, name) \
	do { if (!(cond)) FAIL(name, #cond " was false"); else PASS(name); } while (false)

#define ASSERT_THROWS(expr, exc_type, name) \
	do { \
		bool caught = false; \
		try { expr; } catch (const exc_type&) { caught = true; } \
		if (!caught) FAIL(name, "expected " #exc_type " not thrown"); \
		else         PASS(name); \
	} while (false)

// Builds a simple MODIFY_RESOURCE effect targeting SELF.
static gmRules::EffectSpec make_modify_resource_effect(
	const std::string& resource, int amount)
{
	gmRules::EffectSpec e;
	e.type           = gmRules::EffectType::MODIFY_RESOURCE;
	e.target.kind     = gmRules::TargetKind::ACTOR;
	e.target.selector = gmRules::TargetSelector::SELF;
	e.value          = resource;
	e.amount         = amount;
	return e;
}

// Builds a minimal RuleDefinition.
static gmRules::RuleDefinition make_rule(
	const std::string& id,
	const std::string& resource,
	int                amount)
{
	gmRules::RuleDefinition def;
	def.rule_id     = id;
	def.description = "test rule";
	def.effects.push_back(make_modify_resource_effect(resource, amount));
	return def;
}

// Minimal MockRuleContext with one actor.
static gmRules_test::MockRuleContext make_ctx()
{
	gmRules_test::MockRuleContext ctx;
	ctx.allow_extended_effects = true;
	ctx.add_actor("player_1", "heroes", 5, 5, "loc_a");
	ctx.add_location("loc_a");
	return ctx;
}

const std::string JSON_DOMINION = R"({
  "rules": [
    {
      "rule_id": "r_add_action_1",
      "description": "+1 Azione",
      "effects": [
        { "type": "MODIFY_RESOURCE", "target": "SELF", "value": "actions", "amount": 1 }
      ]
    },
    {
      "rule_id": "r_add_actions_2",
      "description": "+2 Azioni",
      "effects": [
        { "type": "MODIFY_RESOURCE", "target": "SELF", "value": "actions", "amount": 2 }
      ]
    },
    {
      "rule_id": "r_draw_3",
      "description": "Pesca 3 carte",
      "effects": [
        { "type": "DRAW_CARDS", "target": "SELF", "value": "main_deck", "amount": 3 }
      ]
    },
    {
      "rule_id": "r_add_coin_1",
      "description": "+1 Moneta",
      "effects": [
        { "type": "MODIFY_RESOURCE", "target": "SELF", "value": "coins", "amount": 1 }
      ]
    }
  ]
})";

// ─────────────────────────────────────────────────────────────────────────────
// Group 1 — RuleBook registration / query
// ─────────────────────────────────────────────────────────────────────────────

static void test_rulebook_register_and_query()
{
	std::cout << "\n[Group 1] RuleBook registration / query\n";

	gmRules::RuleBook book;
	ASSERT(book.rule_count() == 0, "empty book has 0 rules");
	ASSERT(!book.has_rule("r_test"), "unknown rule not found");

	book.register_rule(make_rule("r_test", "actions", 1));
	ASSERT(book.rule_count() == 1, "after register: count == 1");
	ASSERT(book.has_rule("r_test"), "registered rule found");

	const gmRules::RuleDefinition& def = book.get_rule("r_test");
	ASSERT(def.rule_id == "r_test", "get_rule returns correct id");
	ASSERT(def.has_effects(), "definition has effects");

	ASSERT_THROWS(book.register_rule(make_rule("r_test", "coins", 2)),
	              gmRules::ERuleBookError,
	              "duplicate rule_id throws ERuleBookError");

	ASSERT_THROWS(
		[&]{ gmRules::RuleDefinition bad; bad.rule_id = ""; book.register_rule(bad); }(),
		gmRules::ERuleBookError,
		"empty rule_id throws ERuleBookError");

	ASSERT_THROWS(book.get_rule("unknown"), gmRules::ERuleBookError,
	              "get_rule unknown throws ERuleBookError");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 2 — RuleBookLoader valid JSON
// ─────────────────────────────────────────────────────────────────────────────

static void test_loader_valid_json()
{
	std::cout << "\n[Group 2] RuleBookLoader — valid JSON\n";

	gmRules::RuleBook book;
	gmRules::RuleBookLoader::load_json_string(JSON_DOMINION, book);

	ASSERT(book.rule_count() == 4, "loader registers 4 rules");
	ASSERT(book.has_rule("r_add_action_1"), "r_add_action_1 registered");
	ASSERT(book.has_rule("r_add_actions_2"), "r_add_actions_2 registered");
	ASSERT(book.has_rule("r_draw_3"), "r_draw_3 registered");
	ASSERT(book.has_rule("r_add_coin_1"), "r_add_coin_1 registered");

	const gmRules::RuleDefinition& d = book.get_rule("r_add_action_1");
	ASSERT(d.description == "+1 Azione", "description loaded correctly");
	ASSERT(d.effects.size() == 1, "one effect loaded");
	ASSERT(d.effects[0].type == gmRules::EffectType::MODIFY_RESOURCE, "effect type MODIFY_RESOURCE");
	ASSERT(d.effects[0].value == "actions", "effect value == 'actions'");
	ASSERT(d.effects[0].amount == 1, "effect amount == 1");
	ASSERT(d.effects[0].target.selector == gmRules::TargetSelector::SELF, "target SELF");

	const gmRules::RuleDefinition& d3 = book.get_rule("r_draw_3");
	ASSERT(d3.effects[0].type == gmRules::EffectType::DRAW_CARDS, "r_draw_3 type DRAW_CARDS");
	ASSERT(d3.effects[0].amount == 3, "r_draw_3 amount == 3");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 3 — RuleBookLoader error handling
// ─────────────────────────────────────────────────────────────────────────────

static void test_loader_errors()
{
	std::cout << "\n[Group 3] RuleBookLoader — error handling\n";

	// Missing 'rules' array
	ASSERT_THROWS(
		[]{
			gmRules::RuleBook b;
			gmRules::RuleBookLoader::load_json_string(R"({"other": []})", b);
		}(),
		gmRules::ERuleBookError,
		"missing 'rules' array throws");

	// Missing rule_id
	ASSERT_THROWS(
		[]{
			gmRules::RuleBook b;
			gmRules::RuleBookLoader::load_json_string(
				R"({"rules": [{"effects": []}]})", b);
		}(),
		gmRules::ERuleBookError,
		"missing rule_id throws");

	// Unknown effect type
	ASSERT_THROWS(
		[]{
			gmRules::RuleBook b;
			gmRules::RuleBookLoader::load_json_string(
				R"({"rules":[{"rule_id":"r_x","effects":[{"type":"EXPLODE"}]}]})", b);
		}(),
		gmRules::ERuleBookError,
		"unknown effect type throws");

	// Malformed JSON
	ASSERT_THROWS(
		[]{
			gmRules::RuleBook b;
			gmRules::RuleBookLoader::load_json_string("{rules: []}", b);
		}(),
		gmRules::ERuleBookError,
		"malformed JSON throws");

	// File not found
	ASSERT_THROWS(
		[]{
			gmRules::RuleBook b;
			gmRules::RuleBookLoader::load_json("/tmp/__nonexistent__.json", b);
		}(),
		gmRules::ERuleBookError,
		"missing file throws");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 4 — RuleBook::resolve_rule with MockRuleContext
// ─────────────────────────────────────────────────────────────────────────────

static void test_resolve_rule()
{
	std::cout << "\n[Group 4] RuleBook::resolve_rule\n";

	gmRules::RuleBook book;
	gmRules::RuleBookLoader::load_json_string(JSON_DOMINION, book);

	gmRules_test::MockRuleContext ctx = make_ctx();

	// r_add_action_1 — MODIFY_RESOURCE actions +1
	gmRules::RuleResult r = book.resolve_rule(
		"r_add_action_1", "player_1", {}, ctx);
	ASSERT(r.succeeded(), "r_add_action_1 resolves successfully");

	// Verify resource was modified
	ASSERT(ctx.actor_resource("player_1", "actions") == 1, "actions == 1 after resolve");

	// r_add_actions_2 — +2 actions
	gmRules::RuleResult r2 = book.resolve_rule(
		"r_add_actions_2", "player_1", {}, ctx);
	ASSERT(r2.succeeded(), "r_add_actions_2 resolves successfully");
	ASSERT(ctx.actor_resource("player_1", "actions") == 3, "actions == 3 after +2");

	// r_add_coin_1 — +1 coins
	gmRules::RuleResult r3 = book.resolve_rule(
		"r_add_coin_1", "player_1", {}, ctx);
	ASSERT(r3.succeeded(), "r_add_coin_1 resolves successfully");
	ASSERT(ctx.actor_resource("player_1", "coins") == 1, "coins == 1 after resolve");

	// Unregistered rule throws
	ASSERT_THROWS(
		book.resolve_rule("r_nonexistent", "player_1", {}, ctx),
		gmRules::ERuleBookError,
		"resolve unregistered rule throws ERuleBookError");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 5 — RuleBook::resolve_rules (sequence)
// ─────────────────────────────────────────────────────────────────────────────

static void test_resolve_rules_sequence()
{
	std::cout << "\n[Group 5] RuleBook::resolve_rules sequence\n";

	gmRules::RuleBook book;
	gmRules::RuleBookLoader::load_json_string(JSON_DOMINION, book);

	gmRules_test::MockRuleContext ctx = make_ctx();

	// Village group: r_add_action_1 + r_add_actions_2 → total +3 actions
	gmRules::RuleResult r = book.resolve_rules(
		{"r_add_action_1", "r_add_actions_2"}, "player_1", {}, ctx);
	ASSERT(r.succeeded(), "village rule group resolves");
	ASSERT(ctx.actor_resource("player_1", "actions") == 3,
	       "actions == 3 after village group");

	// Market group: +1 card(skip), +1 action, +1 buy, +1 coin
	// r_draw_3 will likely fail (no deck in ctx) but stop_on_failure
	// can be tested separately. Test only the resource rules here.
	gmRules_test::MockRuleContext ctx2 = make_ctx();
	gmRules::RuleResult r2 = book.resolve_rules(
		{"r_add_action_1", "r_add_coin_1"}, "player_1", {}, ctx2);
	ASSERT(r2.succeeded(), "action+coin rules resolve");
	ASSERT(ctx2.actor_resource("player_1", "actions") == 1, "actions == 1");
	ASSERT(ctx2.actor_resource("player_1", "coins") == 1, "coins == 1");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 6 — gmRulesEngine facade integration
// ─────────────────────────────────────────────────────────────────────────────

static void test_engine_facade()
{
	std::cout << "\n[Group 6] gmRulesEngine::load_rules_json_string + resolve_rule\n";

	gmRules::gmRulesEngine engine;
	engine.load_rules_json_string(JSON_DOMINION);

	ASSERT(engine.rule_book().rule_count() == 4, "engine book has 4 rules");
	ASSERT(engine.rule_book().has_rule("r_add_action_1"), "engine has r_add_action_1");

	gmRules_test::MockRuleContext ctx = make_ctx();

	gmRules::RuleResult r = engine.resolve_rule(
		"r_add_action_1", "player_1", {}, ctx);
	ASSERT(r.succeeded(), "engine.resolve_rule succeeds");
	ASSERT(ctx.actor_resource("player_1", "actions") == 1,
	       "engine resolve_rule mutates context");

	// Calling load again accumulates (no clear)
	const std::string EXTRA = R"({"rules": [
		{"rule_id":"r_extra","description":"extra","effects":[
			{"type":"MODIFY_RESOURCE","target":"SELF","value":"extra_res","amount":5}
		]}
	]})";
	engine.load_rules_json_string(EXTRA);
	ASSERT(engine.rule_book().rule_count() == 5, "second load accumulates rules");

	gmRules::RuleResult r2 = engine.resolve_rule("r_extra", "player_1", {}, ctx);
	ASSERT(r2.succeeded(), "extra rule resolves");
	ASSERT(ctx.actor_resource("player_1", "extra_res") == 5, "extra_res == 5");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== test_rule_book ===\n";
	test_rulebook_register_and_query();
	test_loader_valid_json();
	test_loader_errors();
	test_resolve_rule();
	test_resolve_rules_sequence();
	test_engine_facade();
	std::cout << "\n[OK] All test_rule_book tests passed.\n";
	return 0;
}
