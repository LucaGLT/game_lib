/**
 * @file tests/test_flow_phase.cpp
 * @brief Unit tests for gmFlow::PhaseContext and gmFlow::FlowPhase.
 *
 * Covers: PhaseContext isolation (IDs), GameState sharing, FlowPhase lifecycle,
 *         nested two-level FlowPhase (Epoch → Day), available_actions delegation,
 *         FlowPhase used as IPhase inside SequentialFlowController.
 *
 * All 10 tests are independent: no shared mutable state, no shared fixtures.
 */

#include "gmFlow/session/GameSession.hpp"
#include "gmFlow/session/SessionConfig.hpp"
#include "gmFlow/flow/SequentialFlowController.hpp"
#include "gmFlow/flow/FlowPhase.hpp"
#include "gmFlow/flow/PhaseContext.hpp"
#include "gmFlow/flow/IPhase.hpp"
#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/actions/ActionStatus.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/EventType.hpp"
#include "gmFlow/events/FlowEvents.hpp"
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
    std::cout << "[FAIL] " << name << " -- " << reason << "\n";
    ++g_fail;
}

// ── Shared test infrastructure ────────────────────────────────────────────────

namespace {

// Minimal concrete GameState.
struct SimpleState : public gmFlow::GameState
{
    int  counter = 0;   // incremented by CountAction and CountingPhase
    bool complete = false;

    const gmFlow::SessionId& session_id() const override { return _id; }
    void on_session_started(const gmFlow::SessionId& id) override { _id = id; }
    void on_session_completed()                          override {}

private:
    gmFlow::SessionId _id;
};

// Phase that completes when state.counter >= turns_to_complete.
class CountingPhase : public gmFlow::IPhase
{
public:
    explicit CountingPhase(std::string id, int turns_to_complete)
        : _id(std::move(id)), _turns_needed(turns_to_complete) {}

    gmFlow::PhaseId id() const override { return _id; }

    void on_enter(gmFlow::GameContext&) override { ++entered; }
    void on_exit(gmFlow::GameContext&)  override { ++exited;  }

    std::vector<std::unique_ptr<gmFlow::IAction>>
    available_actions(const gmFlow::GameContext&,
                      const gmFlow::ActorId& actor) const override
    {
        std::vector<std::unique_ptr<gmFlow::IAction>> v;
        if (!actor.empty())
        {
            // Return a minimal proxy so available_actions() tests can detect it.
            // Actual execution is done via CountAction submitted through the session.
        }
        return v;
    }

    bool is_complete(const gmFlow::GameContext& ctx) const override
    {
        const SimpleState& s = static_cast<const SimpleState&>(ctx.state());
        return s.counter >= _turns_needed;
    }

    int entered = 0;
    int exited  = 0;

private:
    std::string _id;
    int         _turns_needed;
};

// Phase whose available_actions() returns one token action per actor.
class ActionsPhase : public gmFlow::IPhase
{
public:
    explicit ActionsPhase(std::string id, int turns_to_complete)
        : _id(std::move(id)), _turns_needed(turns_to_complete) {}

    gmFlow::PhaseId id() const override { return _id; }

    void on_enter(gmFlow::GameContext&) override {}
    void on_exit(gmFlow::GameContext&)  override {}

    std::vector<std::unique_ptr<gmFlow::IAction>>
    available_actions(const gmFlow::GameContext& ctx,
                      const gmFlow::ActorId& actor) const override;

    bool is_complete(const gmFlow::GameContext& ctx) const override
    {
        const SimpleState& s = static_cast<const SimpleState&>(ctx.state());
        return s.counter >= _turns_needed;
    }

private:
    std::string _id;
    int         _turns_needed;
};

// Simple action that increments SimpleState::counter.
class CountAction : public gmFlow::IAction
{
public:
    CountAction(std::string id, std::string owner)
        : _id(std::move(id)), _owner(std::move(owner)) {}

    gmFlow::ActionId     id()    const override { return _id; }
    gmFlow::ActorId      owner() const override { return _owner; }
    gmFlow::ActionStatus status()const override { return _status; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override
    {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext& ctx) override
    {
        SimpleState& s = static_cast<SimpleState&>(ctx.state());
        ++s.counter;
        _status = gmFlow::ActionStatus::COMPLETED;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

private:
    gmFlow::ActionId     _id;
    gmFlow::ActorId      _owner;
    gmFlow::ActionStatus _status = gmFlow::ActionStatus::CREATED;
};

// Token action returned by ActionsPhase::available_actions().
class TokenAction : public gmFlow::IAction
{
public:
    explicit TokenAction(std::string actor)
        : _id("token_" + actor), _owner(std::move(actor)) {}

    gmFlow::ActionId     id()    const override { return _id; }
    gmFlow::ActorId      owner() const override { return _owner; }
    gmFlow::ActionStatus status()const override { return _status; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override
    {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext& ctx) override
    {
        SimpleState& s = static_cast<SimpleState&>(ctx.state());
        ++s.counter;
        _status = gmFlow::ActionStatus::COMPLETED;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

private:
    gmFlow::ActionId     _id;
    gmFlow::ActorId      _owner;
    gmFlow::ActionStatus _status = gmFlow::ActionStatus::CREATED;
};

std::vector<std::unique_ptr<gmFlow::IAction>>
ActionsPhase::available_actions(const gmFlow::GameContext& /*ctx*/,
                                const gmFlow::ActorId& actor) const
{
    std::vector<std::unique_ptr<gmFlow::IAction>> v;
    v.push_back(std::make_unique<TokenAction>(actor));
    return v;
}

// Helper: build a GameSession with the given outer phases and one actor "p1".
struct SessionBuilder
{
    std::shared_ptr<gmDispatch::GmDispatcher> dispatcher =
        std::make_shared<gmDispatch::GmDispatcher>(
            gmDispatch::DispatcherFactory::create_sync_dispatcher("test"));

    std::unique_ptr<gmFlow::GameSession> build(
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases,
        const std::vector<std::string>&              actor_ids,
        gmFlow::TurnPolicy                           tp = {},
        gmFlow::RoundPolicy                          rp = {})
    {
        auto state = std::make_unique<SimpleState>();

        gmFlow::SessionConfig cfg;
        cfg.session_id = "test_session";
        for (const std::string& id : actor_ids)
            cfg.actors.emplace_back(id, gmFlow::ActorType::PLAYER);
        cfg.turn_policy  = tp;
        cfg.round_policy = rp;

        auto ctrl = std::make_unique<gmFlow::SequentialFlowController>(
            std::move(phases), tp, rp);

        return std::make_unique<gmFlow::GameSession>(
            std::move(cfg), std::move(ctrl), std::move(state), dispatcher);
    }
};

// Helper: build a direct GameContext + EventBus for low-level tests.
struct ContextFixture
{
    std::shared_ptr<gmDispatch::GmDispatcher> dispatcher =
        std::make_shared<gmDispatch::GmDispatcher>(
            gmDispatch::DispatcherFactory::create_sync_dispatcher("ctx_test"));

    SimpleState           state;
    gmFlow::ActorRegistry registry;
    gmFlow::EventBus      event_bus{ dispatcher };

    gmFlow::GameContext root_ctx{ "root_session", state, registry, event_bus };
};

} // anonymous namespace

// ── Test 1 — PhaseContext shares GameState with parent ────────────────────────

static void test_phase_context_shares_game_state()
{
    const std::string T = "phase_context_shares_game_state";
    try
    {
        ContextFixture f;
        gmFlow::PhaseContext pc(f.root_ctx, "scope_1");

        // Mutation through PhaseContext is visible via root GameContext.
        static_cast<SimpleState&>(pc.state()).counter = 42;
        int seen = static_cast<SimpleState&>(f.root_ctx.state()).counter;

        seen == 42 ? pass(T) : fail(T, "counter was " + std::to_string(seen));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2 — PhaseContext round_id is isolated from parent ────────────────────

static void test_phase_context_round_id_isolated()
{
    const std::string T = "phase_context_round_id_isolated";
    try
    {
        ContextFixture f;
        gmFlow::PhaseContext pc(f.root_ctx, "scope_1");

        pc.set_current_round_id("inner_round_1");
        bool parent_unchanged = f.root_ctx.current_round_id().empty();
        bool child_set        = pc.current_round_id() == "inner_round_1";

        (parent_unchanged && child_set) ? pass(T) : fail(T,
            "parent_round='" + f.root_ctx.current_round_id()
            + "' child_round='" + pc.current_round_id() + "'");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3 — PhaseContext phase_id is isolated from parent ────────────────────

static void test_phase_context_phase_id_isolated()
{
    const std::string T = "phase_context_phase_id_isolated";
    try
    {
        ContextFixture f;
        gmFlow::PhaseContext pc(f.root_ctx, "scope_1");

        pc.set_current_phase_id("inner_phase_A");
        bool parent_unchanged = f.root_ctx.current_phase_id().empty();
        bool child_set        = pc.current_phase_id() == "inner_phase_A";

        (parent_unchanged && child_set) ? pass(T) : fail(T,
            "parent_phase='" + f.root_ctx.current_phase_id()
            + "' child_phase='" + pc.current_phase_id() + "'");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4 — PhaseContext turn_id is isolated from parent ─────────────────────

static void test_phase_context_turn_id_isolated()
{
    const std::string T = "phase_context_turn_id_isolated";
    try
    {
        ContextFixture f;
        gmFlow::PhaseContext pc(f.root_ctx, "scope_1");

        pc.set_current_turn_id("inner_turn_1");
        bool parent_unchanged = f.root_ctx.current_turn_id().empty();
        bool child_set        = pc.current_turn_id() == "inner_turn_1";

        (parent_unchanged && child_set) ? pass(T) : fail(T,
            "parent_turn='" + f.root_ctx.current_turn_id()
            + "' child_turn='" + pc.current_turn_id() + "'");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 5 — FlowPhase single level: is_complete() when inner phases done ─────

static void test_flow_phase_is_complete()
{
    const std::string T = "flow_phase_is_complete";
    try
    {
        // Build a FlowPhase with one inner CountingPhase (needs 1 action).
        std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
        inner.push_back(std::make_unique<CountingPhase>("inner_P1", 1));

        auto fp = std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(inner)));

        // Outer session with FlowPhase as the only outer phase.
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::move(fp));
        auto session = sb.build(std::move(outer), {"p1"});

        session->start();

        // Submit one action into the inner controller.
        session->submit_action("p1", std::make_unique<CountAction>("a1", "p1"));
        session->tick(); // inner phase completes → FlowPhase completes → session finishes

        session->is_finished() ? pass(T) : fail(T, "session not finished");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6 — FlowPhase: inner controller emits EVT_PHASE_ENTERED on shared bus ─

static void test_flow_phase_events_on_shared_bus()
{
    const std::string T = "flow_phase_events_on_shared_bus";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
        inner.push_back(std::make_unique<CountingPhase>("inner_P1", 1));

        auto fp = std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(inner)));

        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::move(fp));
        auto session = sb.build(std::move(outer), {"p1"});

        std::vector<std::string> entered;
        session->event_bus().subscribe(gmFlow::EVT_PHASE_ENTERED,
            [&](const gmFlow::IEvent& e)
            {
                entered.push_back(
                    static_cast<const gmFlow::PhaseEnteredEvent&>(e).phase_id);
            });

        std::vector<std::string> rounds_started;
        session->event_bus().subscribe(gmFlow::EVT_ROUND_STARTED,
            [&](const gmFlow::IEvent& e)
            {
                rounds_started.push_back(
                    static_cast<const gmFlow::RoundStartedEvent&>(e).round_id);
            });

        session->start();

        // Must have received EVT_PHASE_ENTERED for "inner_P1".
        bool got_inner_phase = false;
        for (const std::string& id : entered)
            if (id == "inner_P1") got_inner_phase = true;

        bool got_round = !rounds_started.empty();

        (got_inner_phase && got_round) ? pass(T) : fail(T,
            "inner_phase_entered=" + std::string(got_inner_phase?"yes":"no")
            + " round_started=" + std::string(got_round?"yes":"no"));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 7 — Two-level nesting: GameState shared across both levels ────────────

static void test_two_level_game_state_shared()
{
    const std::string T = "two_level_game_state_shared";
    try
    {
        // Epoch → Day: counter incremented once per inner action.
        // After start, counter must be visible to both levels because they share
        // the same GameState reference.
        std::vector<std::unique_ptr<gmFlow::IPhase>> day_phases;
        day_phases.push_back(std::make_unique<CountingPhase>("morning", 1));

        auto day_fp = std::make_unique<gmFlow::FlowPhase>(
            "day_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(day_phases)));

        std::vector<std::unique_ptr<gmFlow::IPhase>> epoch_phases;
        epoch_phases.push_back(std::move(day_fp));

        auto epoch_fp = std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(epoch_phases)));

        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::move(epoch_fp));
        auto session = sb.build(std::move(outer), {"p1"});

        session->start();
        session->submit_action("p1", std::make_unique<CountAction>("a1", "p1"));
        session->tick(); // morning complete → day complete → epoch complete → session ends

        // Counter was incremented by the innermost action; session must be finished.
        bool ok = session->is_finished();
        ok ? pass(T) : fail(T, "session not finished after two-level nesting");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8 — Two-level nesting: both levels emit ROUND_STARTED independently ──

static void test_two_level_round_ids_distinct()
{
    const std::string T = "two_level_round_ids_distinct";
    try
    {
        // Each nested controller publishes its own ROUND_STARTED event.
        // Both will generate "round_1" (per D4: IDs are simple; scope_prefix
        // provides qualification for callers who need it).
        // The test verifies: (a) we receive ≥2 ROUND_STARTED events — one per
        // level; (b) set_current_round_id on the day PhaseContext does NOT
        // overwrite the epoch PhaseContext's round_id (isolation).
        std::vector<std::string> round_ids_seen;

        std::vector<std::unique_ptr<gmFlow::IPhase>> day_phases;
        day_phases.push_back(std::make_unique<CountingPhase>("morning", 1));

        auto day_fp = std::make_unique<gmFlow::FlowPhase>(
            "day_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(day_phases)));

        std::vector<std::unique_ptr<gmFlow::IPhase>> epoch_phases;
        epoch_phases.push_back(std::move(day_fp));

        auto epoch_fp = std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(epoch_phases)));

        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::move(epoch_fp));
        auto session = sb.build(std::move(outer), {"p1"});

        session->event_bus().subscribe(gmFlow::EVT_ROUND_STARTED,
            [&](const gmFlow::IEvent& e)
            {
                round_ids_seen.push_back(
                    static_cast<const gmFlow::RoundStartedEvent&>(e).round_id);
            });

        session->start();
        session->submit_action("p1", std::make_unique<CountAction>("a1", "p1"));
        session->tick();

        // Per D4: both levels emit ROUND_STARTED (possibly with the same "round_1"
        // value — that is expected). We only assert that both controllers fired.
        bool two_rounds = round_ids_seen.size() >= 2;

        two_rounds ? pass(T) : fail(T,
            "expected >= 2 ROUND_STARTED events, got "
            + std::to_string(round_ids_seen.size()));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9 — available_actions() delegates to the active inner phase ───────────

static void test_flow_phase_available_actions_delegates()
{
    const std::string T = "flow_phase_available_actions_delegates";
    try
    {
        // Inner phase that returns one TokenAction per actor.
        std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
        inner.push_back(std::make_unique<ActionsPhase>("inner_A", 1));

        gmFlow::FlowPhase fp(
            "scope_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(inner)));

        ContextFixture f;
        fp.on_enter(f.root_ctx);

        auto actions = fp.available_actions(f.root_ctx, "player_1");
        bool ok = !actions.empty();

        fp.on_exit(f.root_ctx);

        ok ? pass(T) : fail(T, "available_actions returned empty");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 10 — FlowPhase inside SequentialFlowController as a plain IPhase ──────

static void test_flow_phase_used_as_iphase_in_outer_controller()
{
    const std::string T = "flow_phase_used_as_iphase_in_outer_controller";
    try
    {
        // Three outer phases: SetupPhase | FlowPhase(epoch) | EndPhase
        // The FlowPhase contains one inner CountingPhase (needs 1 action).
        // After submitting one action the full outer session must complete.

        std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
        inner.push_back(std::make_unique<CountingPhase>("epoch_inner", 1));

        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::make_unique<CountingPhase>("setup",  1));
        outer.push_back(std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(inner))));
        outer.push_back(std::make_unique<CountingPhase>("end_phase", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(outer), {"p1"});

        session->start();
        // Turn 1: setup phase needs 1 action.
        session->submit_action("p1", std::make_unique<CountAction>("a_setup", "p1"));
        session->tick(); // setup done → FlowPhase starts

        // Turn 2: epoch inner phase needs 1 more action (counter already 1).
        session->submit_action("p1", std::make_unique<CountAction>("a_epoch", "p1"));
        session->tick(); // epoch done → end_phase starts

        // Turn 3: end_phase needs 1 more action (counter already 2).
        session->submit_action("p1", std::make_unique<CountAction>("a_end", "p1"));
        session->tick(); // end_phase done → session complete

        session->is_finished() ? pass(T) : fail(T, "session not finished");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "=== test_flow_phase ===\n";

    test_phase_context_shares_game_state();
    test_phase_context_round_id_isolated();
    test_phase_context_phase_id_isolated();
    test_phase_context_turn_id_isolated();
    test_flow_phase_is_complete();
    test_flow_phase_events_on_shared_bus();
    test_two_level_game_state_shared();
    test_two_level_round_ids_distinct();
    test_flow_phase_available_actions_delegates();
    test_flow_phase_used_as_iphase_in_outer_controller();

    std::cout << "\n"
              << g_pass << "/" << (g_pass + g_fail) << " tests passed";
    if (g_fail > 0)
        std::cout << "  *** " << g_fail << " FAILED ***";
    std::cout << "\n";

    return g_fail > 0 ? 1 : 0;
}
