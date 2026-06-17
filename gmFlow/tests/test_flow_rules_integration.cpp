/**
 * @file tests/test_flow_rules_integration.cpp
 * @brief Integration tests for gmFlow::FlowRulesGateway and gmFlow::ActionGateway.
 *
 * Covers: lifecycle callback dispatch, ActionGateway pre-check blocking,
 *         post-hook invocation, full turn sequence ordering, FlowPhase
 *         scope_prefix in payload, and regression without gateway.
 *
 * All 10 tests are independent: no shared mutable state.
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
#include "gmFlow/bridges/FlowRulesGateway.hpp"
#include "gmFlow/bridges/ActionGateway.hpp"
#include "gmDispatch/DispatcherFactory.hpp"

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

// ── Shared infrastructure ─────────────────────────────────────────────────────

namespace {

struct SimpleState : public gmFlow::GameState
{
    int  counter       = 0;
    bool execute_called = false;

    const gmFlow::SessionId& session_id() const override { return _id; }
    void on_session_started(const gmFlow::SessionId& id) override { _id = id; }
    void on_session_completed()                          override {}
private:
    gmFlow::SessionId _id;
};

// Phase that completes when state.counter >= needed.
class CountingPhase : public gmFlow::IPhase
{
public:
    explicit CountingPhase(std::string id, int needed = 1)
        : _id(std::move(id)), _needed(needed) {}

    gmFlow::PhaseId id() const override { return _id; }
    void on_enter(gmFlow::GameContext&) override {}
    void on_exit(gmFlow::GameContext&)  override {}

    std::vector<std::unique_ptr<gmFlow::IAction>>
    available_actions(const gmFlow::GameContext&,
                      const gmFlow::ActorId&) const override { return {}; }

    bool is_complete(const gmFlow::GameContext& ctx) const override
    {
        return static_cast<const SimpleState&>(ctx.state()).counter >= _needed;
    }
private:
    std::string _id;
    int         _needed;
};

// Action that increments counter; optionally fails on execute.
class CountAction : public gmFlow::IAction
{
public:
    CountAction(std::string id, std::string owner, bool should_fail = false)
        : _id(std::move(id)), _owner(std::move(owner))
        , _should_fail(should_fail) {}

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
        s.execute_called = true;
        ++s.counter;
        if (_should_fail)
        {
            _status = gmFlow::ActionStatus::FAILED;
            return gmFlow::ActionResult::failure("forced_failure");
        }
        _status = gmFlow::ActionStatus::COMPLETED;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

private:
    gmFlow::ActionId     _id;
    gmFlow::ActorId      _owner;
    gmFlow::ActionStatus _status       = gmFlow::ActionStatus::CREATED;
    bool                 _should_fail;
};

// Helper: build a session.
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

// Build a base FlowRulesPayload from a session context (for ActionGateway).
gmFlow::FlowRulesPayload make_base_payload(const gmFlow::GameContext& ctx,
                                           const std::string&        actor_id = "")
{
    gmFlow::FlowRulesPayload p;
    p.actor_id    = actor_id;
    p.phase_id    = ctx.current_phase_id();
    p.round_id    = ctx.current_round_id();
    p.turn_id     = ctx.current_turn_id();
    return p;
}

} // anonymous namespace

// ── Test 1 — EVT_TURN_STARTED fires gateway with correct actor_id ─────────────

static void test_gateway_turn_started_actor_id()
{
    const std::string T = "gateway_turn_started_actor_id";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"hero"});

        std::string captured_actor;
        gmFlow::register_flow_rules_gateway(
            session->event_bus(),
            [&](const gmFlow::FlowRulesPayload& p) -> bool
            {
                captured_actor = p.actor_id;
                return true;
            },
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr);

        session->start();

        bool ok = captured_actor == "hero";
        ok ? pass(T) : fail(T, "actor_id was '" + captured_actor + "'");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2 — EVT_ROUND_STARTED fires gateway with correct round_id ────────────

static void test_gateway_round_started_round_id()
{
    const std::string T = "gateway_round_started_round_id";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});

        std::string captured_round;
        gmFlow::register_flow_rules_gateway(
            session->event_bus(),
            nullptr,
            nullptr,
            [&](const gmFlow::FlowRulesPayload& p) -> bool
            {
                captured_round = p.round_id;
                return true;
            },
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        session->start();

        bool ok = !captured_round.empty();
        ok ? pass(T) : fail(T, "round_id was empty");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3 — ActionGateway pre-check fail → execute NOT called ────────────────

static void test_action_gateway_precheck_blocks_execute()
{
    const std::string T = "action_gateway_precheck_blocks_execute";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        gmFlow::FlowRulesPayload base_payload;
        base_payload.actor_id = "p1";

        auto inner   = std::make_unique<CountAction>("act1", "p1");
        CountAction* raw_inner = inner.get();
        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::move(inner),
            base_payload,
            [](const gmFlow::FlowRulesPayload&) -> gmFlow::ValidationResult
            {
                return gmFlow::ValidationResult::fail(
                    gmFlow::ValidationError::RULE_VIOLATION, "blocked by rule");
            },
            nullptr);

        auto vr = session->submit_action("p1", std::move(wrapped));

        // submit_action calls validate() which runs pre-check → must fail.
        bool pre_check_blocked = !vr.valid()
            && vr.error() == gmFlow::ValidationError::RULE_VIOLATION;

        // execute_called must be false because the action was rejected.
        const SimpleState& s =
            static_cast<const SimpleState&>(session->context().state());
        bool no_execute = !s.execute_called;

        (void)raw_inner;
        (pre_check_blocked && no_execute) ? pass(T) : fail(T,
            "vr.valid=" + std::string(vr.valid()?"t":"f")
            + " execute_called=" + std::string(s.execute_called?"t":"f"));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4 — ActionGateway pre-check ok → execute IS called ──────────────────

static void test_action_gateway_precheck_allows_execute()
{
    const std::string T = "action_gateway_precheck_allows_execute";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        gmFlow::FlowRulesPayload base_payload;
        base_payload.actor_id = "p1";

        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("act1", "p1"),
            base_payload,
            [](const gmFlow::FlowRulesPayload&) -> gmFlow::ValidationResult
            {
                return gmFlow::ValidationResult::ok();
            },
            nullptr);

        auto vr = session->submit_action("p1", std::move(wrapped));
        session->tick(); // executes the action

        const SimpleState& s =
            static_cast<const SimpleState&>(session->context().state());

        bool ok = vr.valid() && s.execute_called;
        ok ? pass(T) : fail(T,
            "vr.valid=" + std::string(vr.valid()?"t":"f")
            + " execute_called=" + std::string(s.execute_called?"t":"f"));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 5 — Post-hook called with success result ─────────────────────────────

static void test_action_gateway_posthook_success()
{
    const std::string T = "action_gateway_posthook_success";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        gmFlow::FlowRulesPayload base_payload;
        base_payload.actor_id = "p1";

        bool   post_called    = false;
        bool   post_succeeded = false;
        std::string post_event;

        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("act1", "p1"),
            base_payload,
            nullptr,
            [&](const gmFlow::FlowRulesPayload& p,
                const gmFlow::ActionResult&     r)
            {
                post_called    = true;
                post_succeeded = r.succeeded();
                post_event     = p.event_type;
            });

        session->submit_action("p1", std::move(wrapped));
        session->tick();

        bool ok = post_called && post_succeeded
               && post_event == gmFlow::EVT_ACTION_COMPLETED;
        ok ? pass(T) : fail(T,
            "post_called=" + std::string(post_called?"t":"f")
            + " succeeded=" + std::string(post_succeeded?"t":"f")
            + " event=" + post_event);
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6 — Post-hook called with failure result ─────────────────────────────

static void test_action_gateway_posthook_failure()
{
    const std::string T = "action_gateway_posthook_failure";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        // Phase needs counter >= 1; a failing action still increments counter,
        // so after one tick the phase will still complete.
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        gmFlow::FlowRulesPayload base_payload;
        base_payload.actor_id = "p1";

        bool   post_called    = false;
        bool   post_succeeded = true; // expect false
        std::string post_event;

        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("act1", "p1", /*should_fail=*/true),
            base_payload,
            nullptr,
            [&](const gmFlow::FlowRulesPayload& p,
                const gmFlow::ActionResult&     r)
            {
                post_called    = true;
                post_succeeded = r.succeeded();
                post_event     = p.event_type;
            });

        session->submit_action("p1", std::move(wrapped));
        session->tick();

        bool ok = post_called && !post_succeeded
               && post_event == gmFlow::EVT_ACTION_FAILED;
        ok ? pass(T) : fail(T,
            "post_called=" + std::string(post_called?"t":"f")
            + " succeeded=" + std::string(post_succeeded?"t":"f")
            + " event=" + post_event);
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 7 — Full turn sequence: callback order ───────────────────────────────

static void test_full_turn_callback_order()
{
    const std::string T = "full_turn_callback_order";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});

        std::vector<std::string> order;

        // Register gateway callbacks.
        gmFlow::register_flow_rules_gateway(
            session->event_bus(),
            [&](const gmFlow::FlowRulesPayload&) -> bool { order.push_back("TURN_STARTED"); return true; },
            [&](const gmFlow::FlowRulesPayload&) -> bool { order.push_back("TURN_ENDED");   return true; },
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            [&](const gmFlow::FlowRulesPayload&) -> bool { order.push_back("ACTION_SUBMITTED"); return true; },
            [&](const gmFlow::FlowRulesPayload&) -> bool { order.push_back("ACTION_COMPLETED"); return true; });

        session->start(); // fires TURN_STARTED

        // Build ActionGateway with a post-hook that adds to order.
        gmFlow::FlowRulesPayload bp;
        bp.actor_id = "p1";

        bool post_hook_called = false;
        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("a1", "p1"),
            bp,
            nullptr,
            [&](const gmFlow::FlowRulesPayload&, const gmFlow::ActionResult&)
            {
                order.push_back("POST_HOOK");
                post_hook_called = true;
            });

        session->submit_action("p1", std::move(wrapped)); // fires ACTION_SUBMITTED
        session->tick(); // fires ACTION_COMPLETED, then TURN_ENDED

        // Expected order: TURN_STARTED, ACTION_SUBMITTED, ACTION_COMPLETED,
        //                 POST_HOOK (inside execute), TURN_ENDED
        bool ok = order.size() >= 4
               && order[0] == "TURN_STARTED"
               && order[1] == "ACTION_SUBMITTED"
               && post_hook_called;

        ok ? pass(T) : fail(T, "order size=" + std::to_string(order.size())
                                + " first=" + (order.empty() ? "" : order[0]));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8 — Pre-check blocks; TURN_ENDED still fires ─────────────────────────

static void test_precheck_blocks_turn_ends_anyway()
{
    const std::string T = "precheck_blocks_turn_ends_anyway";
    try
    {
        // Phase needs counter >= 1; since action is blocked, we need a second
        // submit path. Instead we use a plain action after the blocked one.
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});

        bool turn_ended   = false;
        int  rule_blocks  = 0;

        gmFlow::register_flow_rules_gateway(
            session->event_bus(),
            nullptr,
            [&](const gmFlow::FlowRulesPayload&) -> bool
            {
                turn_ended = true;
                return true;
            },
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr);

        session->start();

        // Submit a blocked action.
        gmFlow::FlowRulesPayload bp;
        bp.actor_id = "p1";
        auto blocked = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("blocked", "p1"),
            bp,
            [&](const gmFlow::FlowRulesPayload&) -> gmFlow::ValidationResult
            {
                ++rule_blocks;
                return gmFlow::ValidationResult::fail(
                    gmFlow::ValidationError::RULE_VIOLATION, "rule says no");
            },
            nullptr);

        auto vr = session->submit_action("p1", std::move(blocked));
        bool action_blocked = !vr.valid()
            && vr.error() == gmFlow::ValidationError::RULE_VIOLATION;

        // Now submit a normal action to complete the turn.
        session->submit_action("p1", std::make_unique<CountAction>("ok", "p1"));
        session->tick();

        (action_blocked && turn_ended && rule_blocks == 1) ? pass(T) : fail(T,
            "blocked=" + std::string(action_blocked?"t":"f")
            + " turn_ended=" + std::string(turn_ended?"t":"f")
            + " rule_blocks=" + std::to_string(rule_blocks));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9 — FlowPhase: gateway receives scope_prefix in payload ──────────────

static void test_flow_phase_gateway_scope_prefix()
{
    const std::string T = "flow_phase_gateway_scope_prefix";
    try
    {
        // Build a session with a FlowPhase("epoch_1") containing one inner phase.
        std::vector<std::unique_ptr<gmFlow::IPhase>> inner;
        inner.push_back(std::make_unique<CountingPhase>("epoch_inner", 1));

        auto fp = std::make_unique<gmFlow::FlowPhase>(
            "epoch_1",
            std::make_unique<gmFlow::SequentialFlowController>(std::move(inner)));

        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> outer;
        outer.push_back(std::move(fp));
        auto session = sb.build(std::move(outer), {"p1"});

        session->start();

        // We build the payload with scope_prefix set manually (as game code
        // would do when it knows it is inside a FlowPhase).
        gmFlow::FlowRulesPayload bp;
        bp.actor_id    = "p1";
        bp.scope_prefix = "epoch_1";

        std::string received_scope;
        bool        pre_called = false;

        auto wrapped = std::make_unique<gmFlow::ActionGateway>(
            std::make_unique<CountAction>("a_epoch", "p1"),
            bp,
            [&](const gmFlow::FlowRulesPayload& p) -> gmFlow::ValidationResult
            {
                pre_called     = true;
                received_scope = p.scope_prefix;
                return gmFlow::ValidationResult::ok();
            },
            nullptr);

        session->submit_action("p1", std::move(wrapped));
        session->tick();

        bool ok = pre_called && received_scope == "epoch_1";
        ok ? pass(T) : fail(T,
            "pre_called=" + std::string(pre_called?"t":"f")
            + " scope='" + received_scope + "'");
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 10 — Regression: plain action without gateway works as before ─────────

static void test_regression_plain_action_no_gateway()
{
    const std::string T = "regression_plain_action_no_gateway";
    try
    {
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));

        SessionBuilder sb;
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        // No gateway registered — plain submit_action.
        auto vr = session->submit_action(
            "p1", std::make_unique<CountAction>("plain", "p1"));
        session->tick();

        bool ok = vr.valid() && session->is_finished();
        ok ? pass(T) : fail(T,
            "vr.valid=" + std::string(vr.valid()?"t":"f")
            + " finished=" + std::string(session->is_finished()?"t":"f"));
    }
    catch (...) { fail(T, "unexpected exception"); }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "=== test_flow_rules_integration ===\n";

    test_gateway_turn_started_actor_id();
    test_gateway_round_started_round_id();
    test_action_gateway_precheck_blocks_execute();
    test_action_gateway_precheck_allows_execute();
    test_action_gateway_posthook_success();
    test_action_gateway_posthook_failure();
    test_full_turn_callback_order();
    test_precheck_blocks_turn_ends_anyway();
    test_flow_phase_gateway_scope_prefix();
    test_regression_plain_action_no_gateway();

    std::cout << "\n"
              << g_pass << "/" << (g_pass + g_fail) << " tests passed";
    if (g_fail > 0)
        std::cout << "  *** " << g_fail << " FAILED ***";
    std::cout << "\n";

    return g_fail > 0 ? 1 : 0;
}
