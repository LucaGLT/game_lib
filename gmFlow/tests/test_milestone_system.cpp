/**
 * @file tests/test_milestone_system.cpp
 * @brief Unit tests for gmFlow::TimelineMilestoneSystem.
 *
 * Covers:
 *   - milestone fires in interval (old_time, new_time]
 *   - lower bound is exclusive (threshold == old_time does NOT fire)
 *   - upper bound is inclusive (threshold == new_time fires)
 *   - one-shot milestone fires once then is removed
 *   - persistent milestone fires on every crossing
 *   - multiple milestones in one advance() fire in ascending threshold order
 *   - multiple callbacks at the same threshold fire in registration order
 *   - remove_milestone() removes all entries at a threshold
 *   - clear() empties all entries
 *   - advance() with new_time <= old_time fires nothing
 *   - next_threshold() returns the nearest upcoming threshold
 *   - milestone_count() reflects registrations and removals
 *   - callback-registered milestone fires in the same advance() if in range
 *
 * Build (from game_lib root):
 *   cl /std:c++17 /EHsc /I. ^
 *       gmFlow/core/Result.cpp ^
 *       gmFlow/core/GameContext.cpp ^
 *       gmFlow/actors/ActorRegistry.cpp ^
 *       gmFlow/events/EventBus.cpp ^
 *       gmFlow/flow/TimelineMilestoneSystem.cpp ^
 *       gmDispatch/Dispatcher.cpp ^
 *       gmDispatch/DispatcherFactory.cpp ^
 *       gmDispatch/channels/EventBusChannel.cpp ^
 *       gmDispatch/channels/StdoutChannel.cpp ^
 *       gmDispatch/serializers/JsonSerializer.cpp ^
 *       gmDispatch/routers/SyncRouter.cpp ^
 *       gmDispatch/dispatchers/SyncDispatcher.cpp ^
 *       gmFlow/tests/test_milestone_system.cpp ^
 *       /Fe:test_milestone_system.exe && test_milestone_system.exe
 */

#include "gmFlow/flow/TimelineMilestoneSystem.hpp"
#include "gmFlow/flow/TimelineTypes.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmDispatch/DispatcherFactory.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
	std::cout << "[FAIL] " << name << " — " << reason << "\n";
	++g_fail;
}

// ── Minimal stub game state ───────────────────────────────────────────────────

namespace {

struct StubState : public gmFlow::GameState
{
	const gmFlow::SessionId& session_id() const override { return _id; }
	void on_session_started(const gmFlow::SessionId& id) override { _id = id; }
	void on_session_completed() override {}

private:
	gmFlow::SessionId _id;
};

// Minimal test fixture: creates a GameContext usable as callback argument.
struct MilestoneFixture
{
	StubState                                    state;
	gmFlow::ActorRegistry                        registry;
	std::shared_ptr<gmDispatch::GmDispatcher>    dispatcher;
	std::unique_ptr<gmFlow::EventBus>            bus;
	std::unique_ptr<gmFlow::GameContext>         ctx;

	MilestoneFixture()
	{
		dispatcher = std::make_shared<gmDispatch::GmDispatcher>(
		    gmDispatch::DispatcherFactory::create_sync_dispatcher("MilestoneTestBus"));
		bus = std::make_unique<gmFlow::EventBus>(dispatcher);
		ctx = std::make_unique<gmFlow::GameContext>(
		    "test_session", state, registry, *bus);
	}
};

} // anonymous namespace

// ── Tests ─────────────────────────────────────────────────────────────────────

// Fires when threshold is inside (old, new]
static void test_fires_in_range()
{
	const std::string N = "fires_in_range/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int                             count = 0;
	sys.add_milestone(10,
		[&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.advance(5, 15, *f.ctx);
	count == 1 ? pass(N + "fired") : fail(N + "fired", "expected 1 fire");
}

// Lower bound is exclusive: threshold == old_time must NOT fire
static void test_lower_bound_exclusive()
{
	const std::string N = "lower_bound_exclusive/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int                             count = 0;
	sys.add_milestone(10,
		[&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.advance(10, 20, *f.ctx);  // old_time == threshold — must NOT fire
	count == 0
		? pass(N + "not_fired_at_old")
		: fail(N + "not_fired_at_old", "fired at old_time (expected exclusive)");
}

// Upper bound is inclusive: threshold == new_time MUST fire
static void test_upper_bound_inclusive()
{
	const std::string N = "upper_bound_inclusive/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int                             count = 0;
	sys.add_milestone(20,
		[&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.advance(10, 20, *f.ctx);  // new_time == threshold — MUST fire
	count == 1
		? pass(N + "fired_at_new_time")
		: fail(N + "fired_at_new_time", "did not fire at new_time");
}

// One-shot milestone fires once then is removed
static void test_one_shot_fires_once()
{
	const std::string N = "one_shot/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int                             count = 0;
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&)
		{ ++count; }, /*one_shot=*/true);

	sys.advance(0, 15, *f.ctx);
	sys.advance(0, 15, *f.ctx);  // second crossing: already removed

	count == 1
		? pass(N + "fires_once")
		: fail(N + "fires_once", "expected 1 fire, got " + std::to_string(count));

	sys.milestone_count() == 0
		? pass(N + "removed_after_fire")
		: fail(N + "removed_after_fire", "one-shot should be removed");
}

// Persistent milestone fires on every crossing
static void test_persistent_fires_every_time()
{
	const std::string N = "persistent/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int                             count = 0;
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&)
		{ ++count; }, /*one_shot=*/false);

	sys.advance(0,  15, *f.ctx);
	sys.advance(0,  15, *f.ctx);  // same crossing again
	sys.advance(5,  12, *f.ctx);  // another crossing

	count == 3
		? pass(N + "fires_3_times")
		: fail(N + "fires_3_times", "expected 3, got " + std::to_string(count));

	sys.milestone_count() == 1
		? pass(N + "still_registered")
		: fail(N + "still_registered", "persistent should remain");
}

// Multiple milestones in one advance() fire in ascending threshold order
static void test_multiple_milestones_ordered()
{
	const std::string N = "multiple_ordered/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	std::vector<gmFlow::TimelineValue> fired_order;

	sys.add_milestone(30,
		[&fired_order](gmFlow::TimelineValue t, gmFlow::GameContext&)
		{ fired_order.push_back(t); });
	sys.add_milestone(10,
		[&fired_order](gmFlow::TimelineValue t, gmFlow::GameContext&)
		{ fired_order.push_back(t); });
	sys.add_milestone(20,
		[&fired_order](gmFlow::TimelineValue t, gmFlow::GameContext&)
		{ fired_order.push_back(t); });

	sys.advance(0, 35, *f.ctx);

	bool ordered = (fired_order.size() == 3
	             && fired_order[0] == 10
	             && fired_order[1] == 20
	             && fired_order[2] == 30);
	ordered
		? pass(N + "ascending_order")
		: fail(N + "ascending_order", "milestones did not fire in ascending order");
}

// Multiple callbacks at the same threshold fire in registration order
static void test_same_threshold_registration_order()
{
	const std::string N = "same_threshold_order/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	std::vector<int> order;

	sys.add_milestone(10, [&order](gmFlow::TimelineValue, gmFlow::GameContext&)
		{ order.push_back(1); });
	sys.add_milestone(10, [&order](gmFlow::TimelineValue, gmFlow::GameContext&)
		{ order.push_back(2); });
	sys.add_milestone(10, [&order](gmFlow::TimelineValue, gmFlow::GameContext&)
		{ order.push_back(3); });

	sys.advance(0, 15, *f.ctx);

	bool ok = (order.size() == 3
	        && order[0] == 1 && order[1] == 2 && order[2] == 3);
	ok
		? pass(N + "registration_order")
		: fail(N + "registration_order", "callbacks did not fire in registration order");
}

// remove_milestone() removes all entries at a threshold
static void test_remove_milestone()
{
	const std::string N = "remove_milestone/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int count = 0;
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });
	sys.add_milestone(20, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.remove_milestone(10);

	sys.advance(0, 25, *f.ctx);

	count == 1
		? pass(N + "only_t20_fires")
		: fail(N + "only_t20_fires", "expected 1, got " + std::to_string(count));

	sys.milestone_count() == 0  // t20 was one_shot and auto-removed
		? pass(N + "count_correct")
		: fail(N + "count_correct", "expected 0 after firing one-shot");
}

// clear() removes all milestones
static void test_clear()
{
	const std::string N = "clear/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int count = 0;
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });
	sys.add_milestone(20, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.clear();

	sys.advance(0, 30, *f.ctx);

	count == 0
		? pass(N + "nothing_fires")
		: fail(N + "nothing_fires", "expected 0 after clear");

	sys.milestone_count() == 0
		? pass(N + "count_zero")
		: fail(N + "count_zero", "expected 0 count");
}

// advance() with new_time <= old_time fires nothing
static void test_no_fire_on_no_advance()
{
	const std::string N = "no_advance/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	int count = 0;
	sys.add_milestone(10, [&count](gmFlow::TimelineValue, gmFlow::GameContext&) { ++count; });

	sys.advance(15, 15, *f.ctx);  // new == old
	sys.advance(15, 10, *f.ctx);  // new < old (time reversal)

	count == 0
		? pass(N + "no_fire")
		: fail(N + "no_fire", "advance with no forward movement must not fire");
}

// next_threshold() returns the nearest threshold > after
static void test_next_threshold()
{
	const std::string N = "next_threshold/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	sys.add_milestone(30, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});
	sys.add_milestone(10, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});
	sys.add_milestone(20, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});

	auto n0 = sys.next_threshold(0);
	auto n15 = sys.next_threshold(15);
	auto n25 = sys.next_threshold(25);
	auto n30 = sys.next_threshold(30);

	bool ok = n0.has_value()  && n0.value()  == 10
	       && n15.has_value() && n15.value() == 20
	       && n25.has_value() && n25.value() == 30
	       && !n30.has_value();

	ok
		? pass(N + "correct_values")
		: fail(N + "correct_values", "next_threshold returned unexpected values");
}

// milestone_count() tracks registrations and removals
static void test_milestone_count()
{
	const std::string N = "milestone_count/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;

	sys.milestone_count() == 0
		? pass(N + "starts_zero")
		: fail(N + "starts_zero", "expected 0 initially");

	sys.add_milestone(10, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});
	sys.add_milestone(10, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});
	sys.add_milestone(20, [](gmFlow::TimelineValue, gmFlow::GameContext&) {});

	sys.milestone_count() == 3
		? pass(N + "three_after_add")
		: fail(N + "three_after_add", "expected 3");

	sys.advance(0, 15, *f.ctx);  // fires and removes the two one-shot t10 entries

	sys.milestone_count() == 1
		? pass(N + "one_remaining")
		: fail(N + "one_remaining", "expected 1 after one-shot removal");
}

// Callback passes the correct threshold value to the lambda
static void test_callback_receives_correct_threshold()
{
	const std::string N = "callback_threshold/";
	MilestoneFixture  f;

	gmFlow::TimelineMilestoneSystem sys;
	gmFlow::TimelineValue received = -1;
	sys.add_milestone(42,
		[&received](gmFlow::TimelineValue t, gmFlow::GameContext&) { received = t; });

	sys.advance(0, 50, *f.ctx);

	received == 42
		? pass(N + "correct_value")
		: fail(N + "correct_value", "expected 42, got " + std::to_string(received));
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== TimelineMilestoneSystem unit tests ===\n";

	test_fires_in_range();
	test_lower_bound_exclusive();
	test_upper_bound_inclusive();
	test_one_shot_fires_once();
	test_persistent_fires_every_time();
	test_multiple_milestones_ordered();
	test_same_threshold_registration_order();
	test_remove_milestone();
	test_clear();
	test_no_fire_on_no_advance();
	test_next_threshold();
	test_milestone_count();
	test_callback_receives_correct_threshold();

	std::cout << "\n=== Results: " << g_pass << " passed, "
	          << g_fail << " failed ===\n";
	return g_fail == 0 ? 0 : 1;
}
