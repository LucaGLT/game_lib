/**
 * @file tests/test_timeline_flow_controller.cpp
 * @brief Unit tests for gmFlow::TimelineFlowController.
 *
 * Tests use a minimal fake adapter backed by FakeTimelineState.
 * All tests are deterministic and wall-clock independent.
 *
 * Build (from game_lib root):
 *   cl /std:c++17 /EHsc /I. ^
 *       gmFlow/core/Result.cpp ^
 *       gmFlow/core/GameContext.cpp ^
 *       gmFlow/actors/ActorRegistry.cpp ^
 *       gmFlow/actions/ActionQueue.cpp ^
 *       gmFlow/actions/ActionWindow.cpp ^
 *       gmFlow/actions/StepBasedAction.cpp ^
 *       gmFlow/flow/Turn.cpp ^
 *       gmFlow/flow/Round.cpp ^
 *       gmFlow/flow/TimelineFlowController.cpp ^
 *       gmFlow/events/EventBus.cpp ^
 *       gmFlow/session/GameSession.cpp ^
 *       gmDispatch/Dispatcher.cpp ^
 *       gmDispatch/DispatcherFactory.cpp ^
 *       gmDispatch/channels/EventBusChannel.cpp ^
 *       gmDispatch/channels/StdoutChannel.cpp ^
 *       gmDispatch/serializers/JsonSerializer.cpp ^
 *       gmDispatch/routers/SyncRouter.cpp ^
 *       gmDispatch/dispatchers/SyncDispatcher.cpp ^
 *       gmFlow/tests/test_timeline_flow_controller.cpp ^
 *       /Fe:test_timeline_flow_controller.exe && test_timeline_flow_controller.exe
 */

#include "gmFlow/flow/TimelineFlowController.hpp"
#include "gmFlow/flow/ITimelineAdapter.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"
#include "gmFlow/flow/TimelinePolicy.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/events/TimelineEvents.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmDispatch/DispatcherFactory.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ── Test harness ──────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name) {
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}
static void fail(const std::string& name, const std::string& reason) {
	std::cout << "[FAIL] " << name << " -- " << reason << "\n";
	++g_fail;
}

// ── Fake game state ───────────────────────────────────────────────────────────

namespace {

/// Minimal game state used by all timeline tests.
struct FakeTimelineState : public gmFlow::GameState
{
	std::unordered_map<gmFlow::ActorId, gmFlow::TimelineValue> positions;
	std::unordered_map<gmFlow::ActorId, bool>                  enabled;
	std::unordered_map<gmFlow::ActorId, int>                   ranks;
	bool         complete          = false;
	bool         keep_control      = false;
	gmFlow::ActorId selected_actor;
	int          time_advanced_calls = 0;
	gmFlow::TimelineValue last_old_time = 0;
	gmFlow::TimelineValue last_new_time = 0;

	// Ordered list of actor IDs as seen by the adapter.
	std::vector<gmFlow::ActorId> actor_order;

	const gmFlow::SessionId& session_id() const override { return id_; }
	void on_session_started(const gmFlow::SessionId& id) override { id_ = id; }
	void on_session_completed() override {}

private:
	gmFlow::SessionId id_;
};

// ── Fake adapter ──────────────────────────────────────────────────────────────

class FakeTimelineAdapter : public gmFlow::ITimelineAdapter
{
public:
	std::vector<gmFlow::ActorId>
	timeline_actors(const gmFlow::GameContext& ctx) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		return s.actor_order;
	}

	gmFlow::TimelineValue
	timeline_position(const gmFlow::GameContext& ctx,
	                  const gmFlow::ActorId& actor) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		const auto it = s.positions.find(actor);
		return it != s.positions.end() ? it->second : 0;
	}

	bool is_actor_enabled(const gmFlow::GameContext& ctx,
	                      const gmFlow::ActorId& actor) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		const auto it = s.enabled.find(actor);
		return it != s.enabled.end() ? it->second : true;
	}

	int tie_break_rank(const gmFlow::GameContext& ctx,
	                   const gmFlow::ActorId& actor) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		const auto it = s.ranks.find(actor);
		return it != s.ranks.end() ? it->second : 0;
	}

	bool actor_keeps_control(const gmFlow::GameContext& ctx,
	                         const gmFlow::ActorId& /*actor*/) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		return s.keep_control;
	}

	bool is_session_complete(const gmFlow::GameContext& ctx) const override
	{
		const FakeTimelineState& s =
		    static_cast<const FakeTimelineState&>(ctx.state());
		return s.complete;
	}

	void on_actor_selected(gmFlow::GameContext& ctx,
	                       const gmFlow::ActorId& actor) override
	{
		FakeTimelineState& s =
		    static_cast<FakeTimelineState&>(ctx.state());
		s.selected_actor = actor;
	}

	void on_time_advanced(gmFlow::GameContext& ctx,
	                      gmFlow::TimelineValue old_time,
	                      gmFlow::TimelineValue new_time) override
	{
		FakeTimelineState& s =
		    static_cast<FakeTimelineState&>(ctx.state());
		++s.time_advanced_calls;
		s.last_old_time = old_time;
		s.last_new_time = new_time;
	}
};

// ── Context factory ───────────────────────────────────────────────────────────

struct TestFixture
{
	FakeTimelineState         state;
	gmFlow::ActorRegistry     registry;
	std::shared_ptr<gmDispatch::GmDispatcher> dispatcher;
	std::unique_ptr<gmFlow::EventBus> bus;
	std::unique_ptr<gmFlow::GameContext> ctx;

	explicit TestFixture(const std::string& session_id = "test_session")
	{
		dispatcher = std::make_shared<gmDispatch::GmDispatcher>(
		    gmDispatch::DispatcherFactory::create_sync_dispatcher("TimelineTestBus"));
		bus = std::make_unique<gmFlow::EventBus>(dispatcher);
		ctx = std::make_unique<gmFlow::GameContext>(
		    session_id, state, registry, *bus);
	}

	gmFlow::TimelineFlowController make_controller(
	    gmFlow::TimelinePolicy policy = gmFlow::TimelinePolicy{})
	{
		return gmFlow::TimelineFlowController(
		    std::make_unique<FakeTimelineAdapter>(), policy);
	}
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: Selects lowest timeline actor
// ─────────────────────────────────────────────────────────────────────────────

static void test_selects_lowest_timeline_actor()
{
	const std::string name = "selects_lowest_timeline_actor";
	TestFixture f;
	f.state.actor_order = {"A", "B", "C"};
	f.state.positions   = {{"A", 5}, {"B", 2}, {"C", 8}};
	f.state.enabled     = {{"A", true}, {"B", true}, {"C", true}};

	gmFlow::TimelineFlowController ctrl = f.make_controller();
	ctrl.start(*f.ctx);

	if (!ctrl.active_actor().has_value()) {
		fail(name, "no active actor after start");
		return;
	}
	if (ctrl.active_actor().value() != "B") {
		fail(name, "expected B, got " + ctrl.active_actor().value());
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: Tie-break rank resolves same timeline value
// ─────────────────────────────────────────────────────────────────────────────

static void test_tiebreak_rank_resolves()
{
	const std::string name = "tiebreak_rank_resolves";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 3}, {"B", 3}};
	f.state.ranks       = {{"A", 2}, {"B", 1}};
	f.state.enabled     = {{"A", true}, {"B", true}};

	gmFlow::TimelineFlowController ctrl = f.make_controller();
	ctrl.start(*f.ctx);

	if (!ctrl.active_actor().has_value()) {
		fail(name, "no active actor");
		return;
	}
	if (ctrl.active_actor().value() != "B") {
		fail(name, "expected B (rank 1), got " + ctrl.active_actor().value());
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Stable tie-break preserves adapter order
// ─────────────────────────────────────────────────────────────────────────────

static void test_stable_tiebreak_preserves_order()
{
	const std::string name = "stable_tiebreak_preserves_order";
	TestFixture f;
	// A and B: equal position, equal rank; adapter order is [A, B].
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 3}, {"B", 3}};
	f.state.ranks       = {{"A", 0}, {"B", 0}};
	f.state.enabled     = {{"A", true}, {"B", true}};

	gmFlow::TimelinePolicy policy;
	policy.stable_tie_break = true;

	gmFlow::TimelineFlowController ctrl = f.make_controller(policy);
	ctrl.start(*f.ctx);

	if (!ctrl.active_actor().has_value()) {
		fail(name, "no active actor");
		return;
	}
	if (ctrl.active_actor().value() != "A") {
		fail(name, "expected A (first in adapter order), got " +
		    ctrl.active_actor().value());
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: Disabled actors are ignored
// ─────────────────────────────────────────────────────────────────────────────

static void test_disabled_actors_ignored()
{
	const std::string name = "disabled_actors_ignored";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 0}, {"B", 5}};
	f.state.enabled     = {{"A", false}, {"B", true}};

	gmFlow::TimelineFlowController ctrl = f.make_controller();
	ctrl.start(*f.ctx);

	if (!ctrl.active_actor().has_value()) {
		fail(name, "no active actor");
		return;
	}
	if (ctrl.active_actor().value() != "B") {
		fail(name, "expected B (A is disabled), got " +
		    ctrl.active_actor().value());
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: can_actor_act accepts only active actor
// ─────────────────────────────────────────────────────────────────────────────

static void test_can_actor_act_active_only()
{
	const std::string name = "can_actor_act_active_only";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 1}, {"B", 2}};
	f.state.enabled     = {{"A", true}, {"B", true}};

	// Disable auto-window so can_submit doesn't confuse the check.
	gmFlow::TimelinePolicy policy;
	policy.open_main_action_window = false;

	gmFlow::TimelineFlowController ctrl = f.make_controller(policy);
	ctrl.start(*f.ctx);  // active = A

	const bool a_can_act = ctrl.can_actor_act(*f.ctx, "A");
	const bool b_can_act = ctrl.can_actor_act(*f.ctx, "B");

	if (!a_can_act) {
		fail(name, "active actor A cannot act");
		return;
	}
	if (b_can_act) {
		fail(name, "non-active actor B should not be able to act");
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: Reaction window allows non-active actor
// ─────────────────────────────────────────────────────────────────────────────

static void test_reaction_window_allows_nonactive()
{
	const std::string name = "reaction_window_allows_nonactive";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 1}, {"B", 2}};
	f.state.enabled     = {{"A", true}, {"B", true}};

	gmFlow::TimelinePolicy policy;
	policy.open_main_action_window = false;  // keep window clean for test

	gmFlow::TimelineFlowController ctrl = f.make_controller(policy);
	ctrl.start(*f.ctx);  // active = A

	ctrl.open_reaction_window({"B"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);

	if (!ctrl.can_actor_act(*f.ctx, "B")) {
		fail(name, "B should be able to act inside reaction window");
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 7: Current time advances when minimum position advances
// ─────────────────────────────────────────────────────────────────────────────

static void test_current_time_advances()
{
	const std::string name = "current_time_advances";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	// A has the lowest position, so start time = 0.
	f.state.positions   = {{"A", 0}, {"B", 2}};
	f.state.enabled     = {{"A", true}, {"B", true}};

	gmFlow::TimelinePolicy policy;
	policy.open_main_action_window = false;
	policy.auto_select_next_actor  = false;

	gmFlow::TimelineFlowController ctrl = f.make_controller(policy);
	ctrl.start(*f.ctx);

	if (ctrl.current_time() != 0) {
		fail(name, "initial time should be 0");
		return;
	}

	// Simulate: A's action moves its position to 5; now min is B at 2.
	f.state.positions["A"] = 5;
	ctrl.on_action_completed(*f.ctx, gmFlow::ActionResult::success());

	if (ctrl.current_time() != 2) {
		fail(name, "expected current_time=2 after A advances to 5, got " +
		    std::to_string(ctrl.current_time()));
		return;
	}
	if (f.state.time_advanced_calls != 1) {
		fail(name, "adapter on_time_advanced not called");
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 8: actor_keeps_control prevents immediate actor switch
// ─────────────────────────────────────────────────────────────────────────────

static void test_actor_keeps_control()
{
	const std::string name = "actor_keeps_control";
	TestFixture f;
	f.state.actor_order  = {"A", "B"};
	f.state.positions    = {{"A", 1}, {"B", 2}};
	f.state.enabled      = {{"A", true}, {"B", true}};
	f.state.keep_control = true;  // adapter says: keep active actor

	gmFlow::TimelinePolicy policy;
	policy.open_main_action_window = false;
	policy.auto_select_next_actor  = true;

	gmFlow::TimelineFlowController ctrl = f.make_controller(policy);
	ctrl.start(*f.ctx);  // active = A

	ctrl.on_action_completed(*f.ctx, gmFlow::ActionResult::success());

	if (!ctrl.active_actor().has_value()) {
		fail(name, "active actor cleared despite keep_control=true");
		return;
	}
	if (ctrl.active_actor().value() != "A") {
		fail(name, "expected A to remain active, got " +
		    ctrl.active_actor().value());
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 9: No enabled actors produces no active actor (no crash)
// ─────────────────────────────────────────────────────────────────────────────

static void test_no_enabled_actors()
{
	const std::string name = "no_enabled_actors";
	TestFixture f;
	f.state.actor_order = {"A", "B"};
	f.state.positions   = {{"A", 0}, {"B", 1}};
	f.state.enabled     = {{"A", false}, {"B", false}};

	gmFlow::TimelineFlowController ctrl = f.make_controller();

	bool threw = false;
	try {
		ctrl.start(*f.ctx);
	}
	catch (...) {
		threw = true;
	}

	if (threw) {
		fail(name, "threw on all-disabled actors");
		return;
	}
	if (ctrl.active_actor().has_value()) {
		fail(name, "expected no active actor when all disabled");
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 10: Session complete delegates to adapter
// ─────────────────────────────────────────────────────────────────────────────

static void test_session_complete_delegates_to_adapter()
{
	const std::string name = "session_complete_delegates_to_adapter";
	TestFixture f;
	f.state.actor_order = {"A"};
	f.state.positions   = {{"A", 0}};
	f.state.enabled     = {{"A", true}};
	f.state.complete    = false;

	gmFlow::TimelineFlowController ctrl = f.make_controller();
	ctrl.start(*f.ctx);

	if (ctrl.is_session_complete(*f.ctx)) {
		fail(name, "session should not be complete before adapter says so");
		return;
	}

	f.state.complete = true;

	if (!ctrl.is_session_complete(*f.ctx)) {
		fail(name, "session should be complete after adapter says so");
		return;
	}
	pass(name);
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
	test_selects_lowest_timeline_actor();
	test_tiebreak_rank_resolves();
	test_stable_tiebreak_preserves_order();
	test_disabled_actors_ignored();
	test_can_actor_act_active_only();
	test_reaction_window_allows_nonactive();
	test_current_time_advances();
	test_actor_keeps_control();
	test_no_enabled_actors();
	test_session_complete_delegates_to_adapter();

	std::cout << "\n" << g_pass << " passed, " << g_fail << " failed.\n";
	return g_fail > 0 ? 1 : 0;
}
