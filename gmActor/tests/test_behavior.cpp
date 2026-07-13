/**
 * @file tests/test_behavior.cpp
 * @brief Unit tests for BehaviorCardProcessor and BehaviorReactionSystem.
 *
 * Covers BehaviorCardProcessor:
 *   - process_group_turn: all steps fire in order, timeline advanced per step
 *   - process_group_turn: executor called once per member per step
 *   - process_group_turn: non-optional step fails (all members) → fallback triggered
 *   - process_group_turn: optional step fails → continues without fallback
 *   - process_group_turn: non-optional step fails but ≥1 member succeeds → no fallback
 *   - process_group_turn: empty members → no-op
 *   - process_group_turn: empty card id → no-op
 *   - process_fallback: fallback steps fire, timeline advanced
 *   - process_fallback: empty fallback → 1 tick minimum advance
 *   - multiple members: executor called for each member on each step
 *
 * Covers BehaviorReactionSystem:
 *   - has_reaction: returns true when trigger matches
 *   - has_reaction: returns false when trigger does not match
 *   - has_reaction: returns false when card has no reaction trigger
 *   - has_reaction: returns false when active_behavior_card_id is empty
 *   - fire_reaction: executes reaction steps for all members
 *   - fire_reaction: calls discard_and_draw and updates active_behavior_card_id
 *   - fire_reaction: returns reaction_interrupts = true
 *   - fire_reaction: returns reaction_interrupts = false when configured
 *
 * Build (from game_lib root):
 *   cl /std:c++17 /EHsc /I. ^
 *       gmActor/stats/Health.cpp ^
 *       gmActor/stats/StatBlock.cpp ^
 *       gmActor/modifiers/Modifier.cpp ^
 *       gmActor/statuses/StatusContainer.cpp ^
 *       gmActor/items/InventoryState.cpp ^
 *       gmActor/items/EquipmentState.cpp ^
 *       gmActor/actors/ActorStore.cpp ^
 *       gmActor/actors/ActorQueries.cpp ^
 *       gmActor/behavior/BehaviorCardProcessor.cpp ^
 *       gmActor/behavior/BehaviorReactionSystem.cpp ^
 *       gmActor/tests/test_behavior.cpp ^
 *       /Fe:test_gmActor_behavior.exe && test_gmActor_behavior.exe
 */

#include "gmActor/behavior/BehaviorStep.hpp"
#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/behavior/BehaviorCardProcessor.hpp"
#include "gmActor/behavior/BehaviorReactionSystem.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace gmActor;

// ── Test harness ──────────────────────────────────────────────────────────────

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

// ── Catalog builder helpers ───────────────────────────────────────────────────

static BehaviorStep make_step(const std::string& effect,
                              int amount = 1,
                              int cost   = 1,
                              bool opt   = false)
{
	BehaviorStep s;
	s.effect_type    = effect;
	s.amount         = amount;
	s.timeline_cost  = cost;
	s.optional       = opt;
	return s;
}

static MonsterGroupState make_group(const std::string& id,
                                    const std::string& card_id,
                                    const std::vector<ActorId>& members = {})
{
	MonsterGroupState g;
	g.actor_id                 = id;
	g.group_id                 = id;
	g.active_behavior_card_id  = card_id;
	g.timeline_position        = 0;
	g.members                  = members;
	return g;
}

// ── BehaviorCardProcessor tests ───────────────────────────────────────────────

static void test_processor_all_steps_fire()
{
	const std::string N = "processor_all_steps/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id = "bc_attack";
	card.steps   = { make_step("MOVE", 2, 1), make_step("ATTACK", 3, 2) };
	catalog["bc_attack"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_attack", { "g1" });
	ActorStore        store;

	std::vector<std::string> fired;
	StepExecutor exec = [&fired](const ActorId&, const ActorId&, const BehaviorStep& s) -> bool
	{
		fired.push_back(s.effect_type);
		return true;
	};

	proc.process_group_turn(group, store, exec);

	bool ok = fired.size() == 2 && fired[0] == "MOVE" && fired[1] == "ATTACK";
	ok ? pass(N + "steps_in_order") : fail(N + "steps_in_order", "expected MOVE then ATTACK");
}

static void test_processor_timeline_advanced_per_step()
{
	const std::string N = "processor_timeline/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id = "bc_two";
	card.steps   = { make_step("MOVE", 1, 3), make_step("ATTACK", 1, 5) };
	catalog["bc_two"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_two", { "g1" });
	ActorStore        store;

	proc.process_group_turn(group, store,
		[](const ActorId&, const ActorId&, const BehaviorStep&) { return true; });

	(group.timeline_position == 8)
		? pass(N + "total_8")
		: fail(N + "total_8", "expected 3+5=8, got " + std::to_string(group.timeline_position));
}

static void test_processor_executor_called_per_member_per_step()
{
	const std::string N = "processor_members/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id = "bc_multi";
	card.steps   = { make_step("ATTACK") };
	catalog["bc_multi"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblins", "bc_multi", { "g1", "g2", "g3" });
	ActorStore        store;

	int call_count = 0;
	proc.process_group_turn(group, store,
		[&call_count](const ActorId&, const ActorId&, const BehaviorStep&) -> bool
		{
			++call_count;
			return true;
		});

	(call_count == 3)
		? pass(N + "3_calls")
		: fail(N + "3_calls", "expected 3, got " + std::to_string(call_count));
}

static void test_processor_mandatory_fail_triggers_fallback()
{
	const std::string N = "processor_fallback/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id        = "bc_fail";
	card.steps          = { make_step("ATTACK", 1, 2, /*optional=*/false) };
	card.fallback_steps = { make_step("WAIT",   0, 1) };
	catalog["bc_fail"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_fail", { "g1" });
	ActorStore        store;

	std::vector<std::string> fired;
	proc.process_group_turn(group, store,
		[&fired](const ActorId&, const ActorId&, const BehaviorStep& s) -> bool
		{
			fired.push_back(s.effect_type);
			return false;  // always fail
		});

	bool attack_fired  = !fired.empty() && fired[0] == "ATTACK";
	bool wait_fired    = fired.size() == 2 && fired[1] == "WAIT";
	(attack_fired && wait_fired)
		? pass(N + "fallback_fired")
		: fail(N + "fallback_fired", "expected ATTACK then WAIT");

	// Timeline: mandatory step still pays its cost (2), fallback pays 1
	(group.timeline_position == 3)
		? pass(N + "timeline_3")
		: fail(N + "timeline_3", "expected 3, got " + std::to_string(group.timeline_position));
}

static void test_processor_optional_fail_no_fallback()
{
	const std::string N = "processor_optional/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id        = "bc_opt";
	card.steps          = { make_step("RANGE", 1, 1, /*optional=*/true),
	                         make_step("WAIT",  0, 1, /*optional=*/false) };
	card.fallback_steps = { make_step("PANIC", 0, 1) };
	catalog["bc_opt"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_opt", { "g1" });
	ActorStore        store;

	std::vector<std::string> fired;
	proc.process_group_turn(group, store,
		[&fired](const ActorId&, const ActorId&, const BehaviorStep& s) -> bool
		{
			fired.push_back(s.effect_type);
			// RANGE fails, WAIT succeeds
			return s.effect_type == "WAIT";
		});

	bool range_fired    = !fired.empty() && fired[0] == "RANGE";
	bool wait_fired     = fired.size() == 2 && fired[1] == "WAIT";
	bool panic_not_fired = std::find(fired.begin(), fired.end(), "PANIC") == fired.end();

	(range_fired && wait_fired && panic_not_fired)
		? pass(N + "no_fallback")
		: fail(N + "no_fallback", "PANIC should not fire when only optional step fails");
}

static void test_processor_partial_member_success_no_fallback()
{
	const std::string N = "processor_partial_success/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id        = "bc_partial";
	card.steps          = { make_step("ATTACK", 1, 1, /*optional=*/false) };
	card.fallback_steps = { make_step("WAIT", 0, 1) };
	catalog["bc_partial"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	// Two members: g1 succeeds, g2 fails
	MonsterGroupState group = make_group("goblins", "bc_partial", { "g1", "g2" });
	ActorStore        store;

	std::vector<std::string> fired;
	proc.process_group_turn(group, store,
		[&fired](const ActorId&, const ActorId& member_id, const BehaviorStep& s) -> bool
		{
			fired.push_back(s.effect_type);
			return member_id == "g1";  // only g1 succeeds
		});

	bool wait_not_fired = std::find(fired.begin(), fired.end(), "WAIT") == fired.end();
	wait_not_fired
		? pass(N + "no_fallback_when_any_succeeds")
		: fail(N + "no_fallback_when_any_succeeds", "fallback should not fire if any member succeeded");
}

static void test_processor_empty_members_noop()
{
	const std::string N = "processor_empty_members/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id = "bc_x";
	card.steps   = { make_step("ATTACK") };
	catalog["bc_x"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_x", {} /*no members*/);
	ActorStore        store;

	int call_count = 0;
	proc.process_group_turn(group, store,
		[&call_count](const ActorId&, const ActorId&, const BehaviorStep&) -> bool
		{
			++call_count;
			return true;
		});

	(call_count == 0)
		? pass(N + "no_executor_calls")
		: fail(N + "no_executor_calls", "expected 0 calls, got " + std::to_string(call_count));
}

static void test_processor_empty_card_id_noop()
{
	const std::string N = "processor_empty_card_id/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	// group has no active card
	MonsterGroupState group = make_group("goblin", "" /*empty card id*/, { "g1" });
	group.timeline_position = 5;
	ActorStore        store;

	proc.process_group_turn(group, store,
		[](const ActorId&, const ActorId&, const BehaviorStep&) { return true; });

	(group.timeline_position == 5)
		? pass(N + "no_advance")
		: fail(N + "no_advance", "timeline must not advance when no active card");
}

static void test_processor_empty_fallback_minimum_advance()
{
	const std::string N = "processor_empty_fallback/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id = "bc_nofb";
	card.steps   = { make_step("ATTACK", 1, 4, /*optional=*/false) };
	// fallback_steps is intentionally empty
	catalog["bc_nofb"] = card;

	BehaviorCardProcessor proc([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_nofb", { "g1" });
	group.timeline_position = 0;
	ActorStore        store;

	proc.process_group_turn(group, store,
		[](const ActorId&, const ActorId&, const BehaviorStep&) { return false; });

	// Mandatory step costs 4 + fallback pays minimum 1 = 5
	(group.timeline_position == 5)
		? pass(N + "step4_plus_min1")
		: fail(N + "step4_plus_min1", "expected 5, got " + std::to_string(group.timeline_position));
}

// ── BehaviorReactionSystem tests ──────────────────────────────────────────────

static void test_reaction_has_reaction_match()
{
	const std::string N = "reaction_has/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id         = "bc_react";
	card.reaction_trigger = "hero.attacked";
	catalog["bc_react"] = card;

	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });
	MonsterGroupState group = make_group("goblin", "bc_react");

	sys.has_reaction(group, "hero.attacked")
		? pass(N + "match_true")
		: fail(N + "match_true", "expected true");

	!sys.has_reaction(group, "hero.healed")
		? pass(N + "no_match_false")
		: fail(N + "no_match_false", "expected false");
}

static void test_reaction_no_trigger_on_card()
{
	const std::string N = "reaction_no_trigger/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id         = "bc_no_react";
	card.reaction_trigger = "";  // no reaction
	catalog["bc_no_react"] = card;

	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });
	MonsterGroupState group = make_group("goblin", "bc_no_react");

	!sys.has_reaction(group, "hero.attacked")
		? pass(N + "false_when_no_trigger")
		: fail(N + "false_when_no_trigger", "card has no reaction trigger");
}

static void test_reaction_empty_active_card()
{
	const std::string N = "reaction_empty_card/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "" /*no active card*/);

	!sys.has_reaction(group, "hero.attacked")
		? pass(N + "false")
		: fail(N + "false", "must be false when no active card");
}

static void test_reaction_fire_reaction_steps_and_swap()
{
	const std::string N = "reaction_fire/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id          = "bc_r";
	card.reaction_trigger = "hero.played_card";
	card.reaction_steps   = { make_step("COUNTER", 2, 0) };
	card.reaction_interrupts = true;
	catalog["bc_r"] = card;

	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_r", { "g1", "g2" });

	std::vector<std::string> fired;
	StepExecutor exec = [&fired](const ActorId&, const ActorId&, const BehaviorStep& s) -> bool
	{
		fired.push_back(s.effect_type);
		return true;
	};

	// Stub deck op: discards bc_r, draws bc_next
	BehaviorDeckOp deck_op = []() -> CardId { return "bc_next"; };

	bool interrupts = sys.fire_reaction(group, deck_op, exec);

	// COUNTER should have fired for both members (2 calls)
	(fired.size() == 2 && fired[0] == "COUNTER" && fired[1] == "COUNTER")
		? pass(N + "2_counter_calls")
		: fail(N + "2_counter_calls", "expected 2 COUNTER calls, got " + std::to_string(fired.size()));

	(group.active_behavior_card_id == "bc_next")
		? pass(N + "card_swapped")
		: fail(N + "card_swapped", "active card should be bc_next");

	interrupts
		? pass(N + "interrupts_true")
		: fail(N + "interrupts_true", "expected interrupts=true");
}

static void test_reaction_fire_non_interrupting()
{
	const std::string N = "reaction_non_interrupt/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id             = "bc_quiet";
	card.reaction_trigger    = "hero.moved";
	card.reaction_steps      = {};
	card.reaction_interrupts = false;
	catalog["bc_quiet"] = card;

	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_quiet", { "g1" });
	BehaviorDeckOp    deck_op = []() -> CardId { return "bc_next2"; };
	StepExecutor      exec    = [](const ActorId&, const ActorId&, const BehaviorStep&) { return true; };

	bool interrupts = sys.fire_reaction(group, deck_op, exec);

	!interrupts
		? pass(N + "interrupts_false")
		: fail(N + "interrupts_false", "expected interrupts=false");

	(group.active_behavior_card_id == "bc_next2")
		? pass(N + "card_swapped")
		: fail(N + "card_swapped", "card should be swapped even for non-interrupting reaction");
}

static void test_reaction_deck_exhausted()
{
	const std::string N = "reaction_deck_empty/";

	std::unordered_map<CardId, BehaviorCard> catalog;
	BehaviorCard card;
	card.card_id          = "bc_last";
	card.reaction_trigger = "hero.attacked";
	card.reaction_steps   = {};
	catalog["bc_last"] = card;

	BehaviorReactionSystem sys([&](const CardId& id) { return catalog.at(id); });

	MonsterGroupState group = make_group("goblin", "bc_last", { "g1" });
	BehaviorDeckOp    deck_op = []() -> CardId { return ""; /* deck empty */ };
	StepExecutor      exec    = [](const ActorId&, const ActorId&, const BehaviorStep&) { return true; };

	sys.fire_reaction(group, deck_op, exec);

	group.active_behavior_card_id.empty()
		? pass(N + "empty_card_when_deck_empty")
		: fail(N + "empty_card_when_deck_empty", "expected empty card ID");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== BehaviorCardProcessor + BehaviorReactionSystem unit tests ===\n";

	test_processor_all_steps_fire();
	test_processor_timeline_advanced_per_step();
	test_processor_executor_called_per_member_per_step();
	test_processor_mandatory_fail_triggers_fallback();
	test_processor_optional_fail_no_fallback();
	test_processor_partial_member_success_no_fallback();
	test_processor_empty_members_noop();
	test_processor_empty_card_id_noop();
	test_processor_empty_fallback_minimum_advance();

	test_reaction_has_reaction_match();
	test_reaction_no_trigger_on_card();
	test_reaction_empty_active_card();
	test_reaction_fire_reaction_steps_and_swap();
	test_reaction_fire_non_interrupting();
	test_reaction_deck_exhausted();

	std::cout << "\n=== Results: " << g_pass << " passed, "
	          << g_fail << " failed ===\n";
	return g_fail == 0 ? 0 : 1;
}
