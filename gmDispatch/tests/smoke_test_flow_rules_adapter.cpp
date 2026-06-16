/**
 * @file tests/smoke_test_flow_rules_adapter.cpp
 * @brief Smoke tests for FlowRulesAdapter — gmFlow lifecycle events → gmRules trigger events.
 *
 * Tests verify that each gmFlow lifecycle event published on an EventBus is
 * translated into the correct RuleEvent and emitted through the RuleContext.
 *
 * These tests use:
 * - A real gmFlow::EventBus (backed by a SyncDispatcher from gmDispatch).
 * - A minimal MockRuleContext that records emitted RuleEvents.
 * - FlowRulesAdapter itself.
 */

#include "gmDispatch/bridges/FlowRulesAdapter.hpp"

#include "gmDispatch/DispatcherFactory.hpp"
#include "gmDispatch/Dispatcher.hpp"

#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/FlowEvents.hpp"
#include "gmFlow/events/EventType.hpp"

#include "gmRules/core/RuleContext.hpp"
#include "gmRules/core/RuleEvent.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/Ids.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/status/StatusInstance.hpp"

#include <iostream>
#include <string>
#include <vector>

// ── Minimal MockRuleContext ───────────────────────────────────────────────────

namespace
{

struct CaptureContext : public gmRules::RuleContext
{
	std::vector<gmRules::RuleEvent> events;
	std::vector<std::string>        buses;

	// ── RuleContext (minimal stubs — only emit_event is exercised) ────────────

	bool has_actor(const gmRules::ActorId&) const override { return false; }
	bool actor_has_tag(const gmRules::ActorId&, const std::string&) const override { return false; }
	int  actor_current_hp(const gmRules::ActorId&) const override { return 0; }
	int  actor_max_hp(const gmRules::ActorId&) const override { return 0; }
	int  actor_resource(const gmRules::ActorId&, const std::string&) const override { return 0; }
	bool actor_has_status(const gmRules::ActorId&, const gmRules::StatusId&) const override
		{ return false; }
	std::vector<gmRules::StatusInstanceId>
	statuses_on_actor(const gmRules::ActorId&) const override { return {}; }
	bool are_allies(const gmRules::ActorId&, const gmRules::ActorId&) const override
		{ return false; }
	bool are_enemies(const gmRules::ActorId&, const gmRules::ActorId&) const override
		{ return false; }

	void modify_actor_hp(const gmRules::ActorId&, int) override {}
	void add_actor_tag(const gmRules::ActorId&, const std::string&) override {}
	void remove_actor_tag(const gmRules::ActorId&, const std::string&) override {}
	void spawn_actor(const gmRules::ActorId&, const std::string&) override {}
	void despawn_actor(const gmRules::ActorId&) override {}
	void revive_actor(const gmRules::ActorId&) override {}
	void change_actor_team(const gmRules::ActorId&, const std::string&) override {}
	void modify_resource(const gmRules::ActorId&, const std::string&, int) override {}
	void set_resource_max(const gmRules::ActorId&, const std::string&, int) override {}
	void equip_item(const gmRules::ActorId&, const std::string&) override {}
	void unequip_item(const gmRules::ActorId&, const std::string&) override {}
	void add_status_instance(const gmRules::StatusInstance&) override {}
	void remove_status_instance(const gmRules::StatusInstanceId&) override {}

	bool has_location(const gmRules::LocationId&) const override { return false; }
	gmRules::LocationId actor_location(const gmRules::ActorId&) const override { return ""; }
	bool are_locations_adjacent(const gmRules::LocationId&,
								const gmRules::LocationId&) const override { return false; }
	int  distance_between_locations(const gmRules::LocationId&,
									const gmRules::LocationId&) const override { return -1; }
	bool location_has_tag(const gmRules::LocationId&,
						  const std::string&) const override { return false; }
	std::vector<gmRules::ActorId>
	actors_in_location(const gmRules::LocationId&) const override { return {}; }
	bool is_location_reachable(const gmRules::LocationId&,
							   const gmRules::LocationId&) const override { return false; }
	bool has_line_of_sight(const gmRules::LocationId&,
						   const gmRules::LocationId&) const override { return false; }
	int  move_cost_between(const gmRules::LocationId&,
						   const gmRules::LocationId&) const override { return -1; }

	void move_actor_to_location(const gmRules::ActorId&,
								const gmRules::LocationId&) override {}
	void set_location_passable(const gmRules::LocationId&, bool) override {}
	void add_location_tag(const gmRules::LocationId&, const std::string&) override {}
	void remove_location_tag(const gmRules::LocationId&, const std::string&) override {}
	void set_location_owner(const gmRules::LocationId&, const std::string&) override {}
	void create_barrier(const gmRules::LocationId&, const gmRules::LocationId&,
						const std::string&) override {}
	void remove_barrier(const std::string&) override {}
	void spawn_interactable(const gmRules::LocationId&, const std::string&) override {}
	void despawn_interactable(const std::string&) override {}

	bool has_deck(const gmRules::DeckId&) const override { return false; }
	std::vector<gmRules::CardId> draw_cards(const gmRules::DeckId&, int) override { return {}; }
	gmRules::RuleResult move_card_to_zone(const gmRules::DeckId&, const gmRules::CardId&,
										  const std::string&) override
		{ return gmRules::RuleResult::ok(); }
	int  deck_zone_count(const gmRules::DeckId&, const std::string&) const override { return 0; }
	bool card_in_zone(const gmRules::DeckId&, const gmRules::CardId&,
					  const std::string&) const override { return false; }
	void shuffle_zone(const gmRules::DeckId&, const std::string&) override {}
	std::vector<gmRules::CardId> look_top_cards(const gmRules::DeckId&,
												int) const override { return {}; }
	std::vector<gmRules::CardId> look_bottom_cards(const gmRules::DeckId&,
												   int) const override { return {}; }
	gmRules::RuleResult select_specific_card(const gmRules::DeckId&,
											 const gmRules::CardId&) override
		{ return gmRules::RuleResult::ok(); }
	gmRules::RuleResult discard_random_cards(const gmRules::DeckId&, const std::string&,
											 int) override
		{ return gmRules::RuleResult::ok(); }
	gmRules::RuleResult place_card_on_top(const gmRules::DeckId&,
										  const gmRules::CardId&) override
		{ return gmRules::RuleResult::ok(); }
	gmRules::RuleResult place_card_on_bottom(const gmRules::DeckId&,
											 const gmRules::CardId&) override
		{ return gmRules::RuleResult::ok(); }
	int roll_dice(const std::string&) override { return 1; }

	void emit_event(const gmRules::RuleEvent& event,
					const std::string& bus_name = "RuleEvBus") override
	{
		events.push_back(event);
		buses.push_back(bus_name);
	}

	gmRules::RuleResult apply_extended_effect(const gmRules::EffectSpec&,
											  const gmRules::TargetRef&,
											  const gmRules::ActorId&,
											  gmRules::RuleEvent*) override
	{
		return gmRules::RuleResult::fail(gmRules::RuleError::UNSUPPORTED_EFFECT,
									"not implemented");
	}
};

} // anonymous namespace

// ── Test helpers ──────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	++g_pass;
	std::cout << "[PASS] " << name << "\n";
}

static void fail(const std::string& name, const std::string& reason)
{
	++g_fail;
	std::cout << "[FAIL] " << name << " -- " << reason << "\n";
}

// ── Fixture: creates EventBus + FlowRulesAdapter ──────────────────────────────

struct Fixture
{
	std::shared_ptr<gmDispatch::GmDispatcher> dispatcher;
	gmFlow::EventBus                          bus;
	CaptureContext                            ctx;
	gmDispatch::FlowRulesAdapter              adapter;

	Fixture()
		: dispatcher(std::make_shared<gmDispatch::GmDispatcher>(
			gmDispatch::DispatcherFactory::create_sync_dispatcher("FlowRulesTest")))
		, bus(dispatcher)
		, adapter(bus, ctx, "RuleEvBus")
	{
		adapter.attach();
	}
};

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_attach_detach()
{
	const std::string T = "attach_detach";
	Fixture f;
	if (!f.adapter.is_attached())
	{
		fail(T, "adapter should be attached after attach()");
		return;
	}
	f.adapter.detach();
	if (f.adapter.is_attached())
	{
		fail(T, "adapter should not be attached after detach()");
		return;
	}
	pass(T);
}

static void test_turn_started_emits_per_actor()
{
	const std::string T = "turn_started_emits_per_actor";
	Fixture f;

	gmFlow::TurnStartedEvent ev;
	ev.turn_id       = "turn_001";
	ev.active_actors = {"hero1", "hero2"};
	f.bus.publish(ev);

	if (f.ctx.events.size() != 2)
	{
		fail(T, "expected 2 events (one per actor), got "
			+ std::to_string(f.ctx.events.size()));
		return;
	}
	if (f.ctx.events[0].type != "gmFlow.turn.started")
	{
		fail(T, "unexpected event type: " + f.ctx.events[0].type);
		return;
	}
	if (f.ctx.events[0].source_id != "hero1" || f.ctx.events[1].source_id != "hero2")
	{
		fail(T, "source_id mismatch");
		return;
	}
	if (f.ctx.events[0].priority != gmDispatch::FLOW_TRIGGER_PRIORITY)
	{
		fail(T, "priority should be FLOW_TRIGGER_PRIORITY ("
			+ std::to_string(gmDispatch::FLOW_TRIGGER_PRIORITY) + ")");
		return;
	}
	pass(T);
}

static void test_round_started_emits_with_index()
{
	const std::string T = "round_started_emits_with_index";
	Fixture f;

	gmFlow::RoundStartedEvent ev;
	ev.round_id = "round_1";
	ev.index    = 1;
	f.bus.publish(ev);

	if (f.ctx.events.empty())
	{
		fail(T, "no event emitted");
		return;
	}
	if (f.ctx.events[0].type != "gmFlow.round.started")
	{
		fail(T, "wrong type: " + f.ctx.events[0].type);
		return;
	}
	if (f.ctx.events[0].source_id != "round_1")
	{
		fail(T, "source_id should be round_id");
		return;
	}
	if (f.ctx.events[0].payload_json.find("\"index\":1") == std::string::npos)
	{
		fail(T, "payload_json missing index: " + f.ctx.events[0].payload_json);
		return;
	}
	pass(T);
}

static void test_action_submitted_carries_action_id()
{
	const std::string T = "action_submitted_carries_action_id";
	Fixture f;

	gmFlow::ActionSubmittedEvent ev;
	ev.action_id = "atk_001";
	ev.actor_id  = "hero1";
	f.bus.publish(ev);

	if (f.ctx.events.empty())
	{
		fail(T, "no event emitted");
		return;
	}
	if (f.ctx.events[0].type != "gmFlow.action.submitted")
	{
		fail(T, "wrong type: " + f.ctx.events[0].type);
		return;
	}
	if (f.ctx.events[0].source_id != "hero1")
	{
		fail(T, "source_id should be actor_id");
		return;
	}
	if (f.ctx.events[0].payload_json.find("atk_001") == std::string::npos)
	{
		fail(T, "payload_json missing action_id: " + f.ctx.events[0].payload_json);
		return;
	}
	pass(T);
}

static void test_phase_entered_carries_previous()
{
	const std::string T = "phase_entered_carries_previous";
	Fixture f;

	gmFlow::PhaseEnteredEvent ev;
	ev.phase_id    = "COMBAT";
	ev.previous_id = "SETUP";
	f.bus.publish(ev);

	if (f.ctx.events.empty())
	{
		fail(T, "no event emitted");
		return;
	}
	if (f.ctx.events[0].type != "gmFlow.phase.entered")
	{
		fail(T, "wrong type: " + f.ctx.events[0].type);
		return;
	}
	if (f.ctx.events[0].target_id != "SETUP")
	{
		fail(T, "target_id should be previous_id");
		return;
	}
	pass(T);
}

static void test_action_failed_carries_reason()
{
	const std::string T = "action_failed_carries_reason";
	Fixture f;

	gmFlow::ActionFailedEvent ev;
	ev.action_id = "move_001";
	ev.actor_id  = "hero2";
	ev.reason    = "path_blocked";
	f.bus.publish(ev);

	if (f.ctx.events.empty())
	{
		fail(T, "no event emitted");
		return;
	}
	if (f.ctx.events[0].payload_json.find("path_blocked") == std::string::npos)
	{
		fail(T, "payload_json missing reason: " + f.ctx.events[0].payload_json);
		return;
	}
	pass(T);
}

static void test_bus_name_forwarded()
{
	const std::string T = "bus_name_forwarded";

	// Build adapter with custom bus name
	std::shared_ptr<gmDispatch::GmDispatcher> disp =
		std::make_shared<gmDispatch::GmDispatcher>(
			gmDispatch::DispatcherFactory::create_sync_dispatcher("FlowBusTest"));
	gmFlow::EventBus  bus(disp);
	CaptureContext    ctx;
	gmDispatch::FlowRulesAdapter adapter(bus, ctx, "FlowEvBus");
	adapter.attach();

	gmFlow::TurnEndedEvent ev;
	ev.turn_id = "turn_002";
	bus.publish(ev);

	if (ctx.buses.empty() || ctx.buses[0] != "FlowEvBus")
	{
		fail(T, "expected bus_name 'FlowEvBus', got: "
			+ (ctx.buses.empty() ? "<none>" : ctx.buses[0]));
		return;
	}
	pass(T);
}

static void test_window_opened_emits_per_actor()
{
	const std::string T = "window_opened_emits_per_actor";
	Fixture f;

	gmFlow::WindowOpenedEvent ev;
	ev.eligible_actors = {"hero1", "hero2", "hero3"};
	f.bus.publish(ev);

	if (f.ctx.events.size() != 3)
	{
		fail(T, "expected 3 events, got " + std::to_string(f.ctx.events.size()));
		return;
	}
	for (const gmRules::RuleEvent& re : f.ctx.events)
	{
		if (re.type != "gmFlow.window.opened")
		{
			fail(T, "wrong event type: " + re.type);
			return;
		}
	}
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmDispatch: FlowRulesAdapter smoke tests ===\n\n";

	test_attach_detach();
	test_turn_started_emits_per_actor();
	test_round_started_emits_with_index();
	test_action_submitted_carries_action_id();
	test_phase_entered_carries_previous();
	test_action_failed_carries_reason();
	test_bus_name_forwarded();
	test_window_opened_emits_per_actor();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
