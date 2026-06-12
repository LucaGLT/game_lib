/**
 * @file tests/test_action_queue.cpp
 * @brief Unit tests for gmFlow::ActionQueue.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmFlow/core/Result.cpp ^
 *       gmFlow/tests/test_action_queue.cpp ^
 *       -o test_action_queue.exe && test_action_queue.exe
 */

#include "gmFlow/actions/ActionQueue.hpp"
#include "gmFlow/actions/ActionStatus.hpp"
#include "gmFlow/core/Result.hpp"
#include "gmFlow/core/Ids.hpp"

#include <cassert>
#include <iostream>
#include <string>

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

// ── Minimal concrete IAction for testing ─────────────────────────────────────

namespace {

class StubAction : public gmFlow::IAction {
public:
    explicit StubAction(std::string id, std::string owner = "actor_1")
        : id_(std::move(id)), owner_(std::move(owner)) {}

    gmFlow::ActionId     id()     const override { return id_; }
    gmFlow::ActorId      owner()  const override { return owner_; }
    gmFlow::ActionStatus status() const override { return gmFlow::ActionStatus::CREATED; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext&) override {
        executed = true;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

    bool executed = false;

private:
    gmFlow::ActionId id_;
    gmFlow::ActorId  owner_;
};

} // anonymous namespace

// ── Test 1: Empty queue ───────────────────────────────────────────────────────

static void test_empty_queue() {
    const std::string T = "empty_queue";
    try {
        gmFlow::ActionQueue q;
        bool ok = q.empty() && q.size() == 0;
        ok ? pass(T) : fail(T, "expected empty queue");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2: Push and front ────────────────────────────────────────────────────

static void test_push_and_front() {
    const std::string T = "push_and_front";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("a1"), gmFlow::ActionPriority::NORMAL);
        bool ok = !q.empty() && q.size() == 1 && q.front().id() == "a1";
        ok ? pass(T) : fail(T, "wrong front after push");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3: Pop removes front ─────────────────────────────────────────────────

static void test_pop() {
    const std::string T = "pop";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("a1"), gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("a2"), gmFlow::ActionPriority::NORMAL);
        q.pop();
        bool ok = q.size() == 1 && q.front().id() == "a2";
        ok ? pass(T) : fail(T, "wrong state after pop");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4: IMMEDIATE before NORMAL ──────────────────────────────────────────

static void test_priority_immediate_before_normal() {
    const std::string T = "priority_immediate_before_normal";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("normal"),   gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("immediate"),gmFlow::ActionPriority::IMMEDIATE);
        bool ok = q.front().id() == "immediate";
        ok ? pass(T) : fail(T, "IMMEDIATE should precede NORMAL");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 5: REACTION between IMMEDIATE and NORMAL ────────────────────────────

static void test_priority_reaction() {
    const std::string T = "priority_reaction";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("normal"),   gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("reaction"), gmFlow::ActionPriority::REACTION);
        q.push(std::make_unique<StubAction>("immediate"),gmFlow::ActionPriority::IMMEDIATE);
        q.pop(); // removes "immediate"
        bool ok = q.front().id() == "reaction";
        ok ? pass(T) : fail(T, "REACTION should follow IMMEDIATE");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6: DEFERRED is last ──────────────────────────────────────────────────

static void test_priority_deferred_last() {
    const std::string T = "priority_deferred_last";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("deferred"), gmFlow::ActionPriority::DEFERRED);
        q.push(std::make_unique<StubAction>("normal"),   gmFlow::ActionPriority::NORMAL);
        bool ok = q.front().id() == "normal";
        ok ? pass(T) : fail(T, "NORMAL should precede DEFERRED");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 7: FIFO within same priority ────────────────────────────────────────

static void test_fifo_same_priority() {
    const std::string T = "fifo_same_priority";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("first"),  gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("second"), gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("third"),  gmFlow::ActionPriority::NORMAL);

        bool ok = q.front().id() == "first";
        q.pop();
        ok = ok && q.front().id() == "second";
        q.pop();
        ok = ok && q.front().id() == "third";

        ok ? pass(T) : fail(T, "FIFO order not preserved within same priority");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8: Clear empties the queue ──────────────────────────────────────────

static void test_clear() {
    const std::string T = "clear";
    try {
        gmFlow::ActionQueue q;
        q.push(std::make_unique<StubAction>("a"), gmFlow::ActionPriority::NORMAL);
        q.push(std::make_unique<StubAction>("b"), gmFlow::ActionPriority::NORMAL);
        q.clear();
        bool ok = q.empty() && q.size() == 0;
        ok ? pass(T) : fail(T, "queue should be empty after clear()");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9: Front on empty queue throws ──────────────────────────────────────

static void test_front_empty_throws() {
    const std::string T = "front_empty_throws";
    try {
        gmFlow::ActionQueue q;
        q.front();
        fail(T, "should have thrown on empty queue");
    } catch (const std::runtime_error&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 10: Pop on empty queue throws ───────────────────────────────────────

static void test_pop_empty_throws() {
    const std::string T = "pop_empty_throws";
    try {
        gmFlow::ActionQueue q;
        q.pop();
        fail(T, "should have thrown on empty queue");
    } catch (const std::runtime_error&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_action_queue ===\n";

    test_empty_queue();
    test_push_and_front();
    test_pop();
    test_priority_immediate_before_normal();
    test_priority_reaction();
    test_priority_deferred_last();
    test_fifo_same_priority();
    test_clear();
    test_front_empty_throws();
    test_pop_empty_throws();

    std::cout << "\nResults: " << g_pass << " passed, " << g_fail << " failed.\n";
    return g_fail == 0 ? 0 : 1;
}
