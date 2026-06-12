/**
 * @file tests/test_action_window.cpp
 * @brief Unit tests for gmFlow::ActionWindow and CompletionPolicy.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmFlow/core/Result.cpp ^
 *       gmFlow/core/GameContext.cpp ^
 *       gmFlow/actors/ActorRegistry.cpp ^
 *       gmFlow/actions/ActionQueue.cpp ^
 *       gmFlow/actions/ActionWindow.cpp ^
 *       gmFlow/events/EventBus.cpp ^
 *       gmDispatch/Dispatcher.cpp ^
 *       gmDispatch/DispatcherFactory.cpp ^
 *       gmDispatch/channels/EventBusChannel.cpp ^
 *       gmDispatch/channels/StdoutChannel.cpp ^
 *       gmDispatch/serializers/JsonSerializer.cpp ^
 *       gmDispatch/routers/SyncRouter.cpp ^
 *       gmDispatch/dispatchers/SyncDispatcher.cpp ^
 *       gmFlow/tests/test_action_window.cpp ^
 *       -o test_action_window.exe && test_action_window.exe
 */

#include "gmFlow/actions/ActionWindow.hpp"
#include "gmFlow/actions/ActionStatus.hpp"
#include "gmFlow/core/GameContext.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/actors/ActorRegistry.hpp"
#include "gmFlow/events/EventBus.hpp"
#include "gmFlow/events/EventType.hpp"

#include "gmDispatch/DispatcherFactory.hpp"

#include <cassert>
#include <iostream>
#include <string>
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

// ── Minimal concrete stubs ────────────────────────────────────────────────────

namespace {

// Minimal GameState for test contexts.
class NullState : public gmFlow::GameState {
public:
    const gmFlow::SessionId& session_id() const override { return id_; }
    void on_session_started(const gmFlow::SessionId& id) override { id_ = id; }
    void on_session_completed() override {}
private:
    gmFlow::SessionId id_;
};

// Stub IAction that records whether execute() was called.
class TrackingAction : public gmFlow::IAction {
public:
    explicit TrackingAction(std::string id, std::string owner)
        : id_(std::move(id)), owner_(std::move(owner)) {}

    gmFlow::ActionId     id()     const override { return id_; }
    gmFlow::ActorId      owner()  const override { return owner_; }
    gmFlow::ActionStatus status() const override { return status_; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext&) override {
        executed = true;
        status_  = gmFlow::ActionStatus::COMPLETED;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

    bool executed = false;

private:
    gmFlow::ActionId     id_;
    gmFlow::ActorId      owner_;
    gmFlow::ActionStatus status_ = gmFlow::ActionStatus::CREATED;
};

// Stub action that always fails execute().
class FailingAction : public gmFlow::IAction {
public:
    explicit FailingAction(std::string id, std::string owner)
        : id_(std::move(id)), owner_(std::move(owner)) {}

    gmFlow::ActionId     id()     const override { return id_; }
    gmFlow::ActorId      owner()  const override { return owner_; }
    gmFlow::ActionStatus status() const override { return gmFlow::ActionStatus::CREATED; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext&) override {
        return gmFlow::ActionResult::failure("intentional failure");
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

private:
    gmFlow::ActionId id_;
    gmFlow::ActorId  owner_;
};

// Helper: build a GameContext with no actors and a null state.
struct TestCtx {
    NullState                           state;
    gmFlow::ActorRegistry               registry;
    std::shared_ptr<GmDispatch::Dispatcher> dispatcher =
        std::make_shared<GmDispatch::Dispatcher>(
            GmDispatch::DispatcherFactory::createSyncDispatcher("test"));
    gmFlow::EventBus                    bus{dispatcher};
    gmFlow::GameContext                 ctx{"test_session", state, registry, bus};
};

} // anonymous namespace

// ── Test 1: Constructor — window open, not closed ─────────────────────────────

static void test_construction() {
    const std::string T = "construction";
    try {
        gmFlow::ActionWindow w({"p1", "p2"}, gmFlow::CompletionPolicy::ALL_SUBMITTED);
        bool ok = !w.is_closed()
               && w.eligible_actors().size() == 2
               && w.submission_count() == 0;
        ok ? pass(T) : fail(T, "unexpected initial state");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2: can_submit — eligible actor ───────────────────────────────────────

static void test_can_submit_eligible() {
    const std::string T = "can_submit_eligible";
    try {
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        bool ok = w.can_submit("p1");
        ok ? pass(T) : fail(T, "eligible actor should be able to submit");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3: can_submit — non-eligible actor ───────────────────────────────────

static void test_can_submit_not_eligible() {
    const std::string T = "can_submit_not_eligible";
    try {
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        bool ok = !w.can_submit("p2");
        ok ? pass(T) : fail(T, "non-eligible actor should not be able to submit");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4: can_submit — already submitted ────────────────────────────────────

static void test_can_submit_already_submitted() {
    const std::string T = "can_submit_already_submitted";
    try {
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::MANUAL_CLOSE);
        auto vr = w.submit("p1", std::make_unique<TrackingAction>("a1", "p1"));
        bool ok = vr.valid() && !w.can_submit("p1");
        ok ? pass(T) : fail(T, "actor should not submit twice");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 5: ANY_SUBMITTED — complete after first submission ───────────────────

static void test_policy_any_submitted() {
    const std::string T = "policy_any_submitted";
    try {
        TestCtx tctx;
        gmFlow::ActionWindow w({"p1","p2"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        bool before = !w.is_complete(tctx.ctx);
        w.submit("p1", std::make_unique<TrackingAction>("a1","p1"));
        bool after  = w.is_complete(tctx.ctx);
        bool ok = before && after;
        ok ? pass(T) : fail(T, "window should complete after first submission");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6: ALL_SUBMITTED — complete only after all submit ────────────────────

static void test_policy_all_submitted() {
    const std::string T = "policy_all_submitted";
    try {
        TestCtx tctx;
        gmFlow::ActionWindow w({"p1","p2"}, gmFlow::CompletionPolicy::ALL_SUBMITTED);
        w.submit("p1", std::make_unique<TrackingAction>("a1","p1"));
        bool partial = !w.is_complete(tctx.ctx);
        w.submit("p2", std::make_unique<TrackingAction>("a2","p2"));
        bool full = w.is_complete(tctx.ctx);
        bool ok = partial && full;
        ok ? pass(T) : fail(T, "window should only be complete after all submissions");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 7: MANUAL_CLOSE — never complete until force_close() ────────────────

static void test_policy_manual_close() {
    const std::string T = "policy_manual_close";
    try {
        TestCtx tctx;
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::MANUAL_CLOSE);
        w.submit("p1", std::make_unique<TrackingAction>("a1","p1"));
        bool before = !w.is_complete(tctx.ctx);
        w.force_close();
        bool after = w.is_closed();
        bool ok = before && after;
        ok ? pass(T) : fail(T, "manual close window should only close via force_close()");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8: UNTIL_ALL_PASSED — complete when all pass ────────────────────────

static void test_policy_until_all_passed() {
    const std::string T = "policy_until_all_passed";
    try {
        TestCtx tctx;
        gmFlow::ActionWindow w({"p1","p2"}, gmFlow::CompletionPolicy::UNTIL_ALL_PASSED);
        w.pass("p1");
        bool partial = !w.is_complete(tctx.ctx);
        w.pass("p2");
        bool full = w.is_complete(tctx.ctx);
        bool ok = partial && full;
        ok ? pass(T) : fail(T, "window should complete only after all actors pass");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9: resolve() executes submitted action ───────────────────────────────

static void test_resolve_executes_action() {
    const std::string T = "resolve_executes_action";
    try {
        TestCtx tctx;
        auto* raw = new TrackingAction("a1","p1");
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        w.submit("p1", std::unique_ptr<TrackingAction>(raw));
        w.resolve(tctx.ctx);
        bool ok = raw->executed && w.is_closed();
        ok ? pass(T) : fail(T, "resolve() should execute action and close window");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 10: resolve() emits WindowClosed event ───────────────────────────────

static void test_resolve_emits_window_closed() {
    const std::string T = "resolve_emits_window_closed";
    try {
        TestCtx tctx;
        bool received = false;
        tctx.bus.subscribe(gmFlow::EVT_WINDOW_CLOSED,
            [&](const gmFlow::IEvent&) { received = true; });

        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        w.submit("p1", std::make_unique<TrackingAction>("a1","p1"));
        w.resolve(tctx.ctx);

        received ? pass(T) : fail(T, "EVT_WINDOW_CLOSED not received");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 11: submit() to closed window returns fail ───────────────────────────

static void test_submit_to_closed_window_fails() {
    const std::string T = "submit_to_closed_window_fails";
    try {
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::ANY_SUBMITTED);
        w.force_close();
        auto vr = w.submit("p1", std::make_unique<TrackingAction>("a1","p1"));
        bool ok = !vr.valid();
        ok ? pass(T) : fail(T, "submitting to closed window should fail");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 12: pass() by non-eligible actor is silently ignored ─────────────────

static void test_pass_non_eligible_ignored() {
    const std::string T = "pass_non_eligible_ignored";
    try {
        TestCtx tctx;
        gmFlow::ActionWindow w({"p1"}, gmFlow::CompletionPolicy::UNTIL_ALL_PASSED);
        w.pass("p_stranger"); // not eligible
        bool ok = !w.is_complete(tctx.ctx);
        ok ? pass(T) : fail(T, "pass by non-eligible actor should be ignored");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_action_window ===\n";

    test_construction();
    test_can_submit_eligible();
    test_can_submit_not_eligible();
    test_can_submit_already_submitted();
    test_policy_any_submitted();
    test_policy_all_submitted();
    test_policy_manual_close();
    test_policy_until_all_passed();
    test_resolve_executes_action();
    test_resolve_emits_window_closed();
    test_submit_to_closed_window_fails();
    test_pass_non_eligible_ignored();

    std::cout << "\nResults: " << g_pass << " passed, " << g_fail << " failed.\n";
    return g_fail == 0 ? 0 : 1;
}
