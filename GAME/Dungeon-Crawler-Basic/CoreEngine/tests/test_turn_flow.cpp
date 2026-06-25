/**
 * @file tests/test_turn_flow.cpp
 * @brief Unit tests for TurnFlow.
 */

#include "flow/TurnFlow.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using gmDungeonBasic::TurnFlow;

static void test_initial_state()
{
	TurnFlow flow;
	assert(!flow.is_session_active());
	assert(!flow.is_turn_active());
	assert(flow.current_round() == 0);
	assert(flow.current_actor_id().empty());
	std::cout << "  [OK] test_initial_state\n";
}

static void test_start_end_session()
{
	TurnFlow flow;
	flow.start_session();
	assert(flow.is_session_active());
	assert(flow.current_round() == 1);
	flow.end_session();
	assert(!flow.is_session_active());
	assert(flow.current_round() == 0);
	std::cout << "  [OK] test_start_end_session\n";
}

static void test_start_turn_without_session_throws()
{
	TurnFlow flow;
	bool threw = false;
	try { flow.start_turn("actor_1"); } catch (const std::logic_error&) { threw = true; }
	assert(threw);
	std::cout << "  [OK] test_start_turn_without_session_throws\n";
}

static void test_turn_lifecycle()
{
	TurnFlow flow;
	flow.start_session();
	flow.start_turn("actor_1");
	assert(flow.is_turn_active());
	assert(flow.current_actor_id() == "actor_1");
	flow.end_turn();
	assert(!flow.is_turn_active());
	assert(flow.current_actor_id().empty());
	std::cout << "  [OK] test_turn_lifecycle\n";
}

static void test_end_turn_without_active_throws()
{
	TurnFlow flow;
	flow.start_session();
	bool threw = false;
	try { flow.end_turn(); } catch (const std::logic_error&) { threw = true; }
	assert(threw);
	std::cout << "  [OK] test_end_turn_without_active_throws\n";
}

static void test_actor_order_and_next()
{
	TurnFlow flow;
	flow.start_session();
	flow.set_actor_order({"hero", "monster_1", "monster_2"});
	assert(flow.next_actor_id() == "hero");
	flow.start_turn("hero");
	// next actor is monster_1 while hero turn is active
	assert(flow.next_actor_id() == "monster_1");
	flow.end_turn();
	assert(flow.next_actor_id() == "monster_1");
	std::cout << "  [OK] test_actor_order_and_next\n";
}

static void test_round_increment_after_full_cycle()
{
	TurnFlow flow;
	flow.start_session();
	flow.set_actor_order({"a", "b"});
	assert(flow.current_round() == 1);
	flow.start_turn("a");
	flow.end_turn();
	flow.start_turn("b");
	flow.end_turn();   // full round done → round 2
	assert(flow.current_round() == 2);
	std::cout << "  [OK] test_round_increment_after_full_cycle\n";
}

static void test_reset()
{
	TurnFlow flow;
	flow.start_session();
	flow.set_actor_order({"a"});
	flow.start_turn("a");
	flow.reset();
	assert(!flow.is_session_active());
	assert(!flow.is_turn_active());
	assert(flow.current_round() == 0);
	std::cout << "  [OK] test_reset\n";
}

int main()
{
	std::cout << "=== TurnFlow unit tests ===\n";
	test_initial_state();
	test_start_end_session();
	test_start_turn_without_session_throws();
	test_turn_lifecycle();
	test_end_turn_without_active_throws();
	test_actor_order_and_next();
	test_round_increment_after_full_cycle();
	test_reset();
	std::cout << "All TurnFlow tests PASSED.\n";
	return 0;
}
