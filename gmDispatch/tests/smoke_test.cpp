/**
 * @file tests/smoke_test.cpp
 * @brief Phase 2 smoke test for the gmDispatch library.
 *
 * Exercises:
 *  1. createSyncDispatcher + EventBusChannel subscription
 *  2. Exact-match routing
 *  3. Wildcard "*" routing
 *  4. subscribe / unsubscribe lifecycle
 *  5. createDebugDispatcher (StdoutChannel + JsonSerializer)
 *  6. Auto-timestamp injection
 *  7. Payload round-trip via std::any_cast
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. \
 *     gmDispatch/tests/smoke_test.cpp \
 *     gmDispatch/Dispatcher.cpp \
 *     gmDispatch/DispatcherFactory.cpp \
 *     gmDispatch/channels/EventBusChannel.cpp \
 *     gmDispatch/channels/StdoutChannel.cpp \
 *     gmDispatch/serializers/JsonSerializer.cpp \
 *     gmDispatch/routers/SyncRouter.cpp \
 *     gmDispatch/dispatchers/SyncDispatcher.cpp \
 *     -o smoke_gmDispatch && ./smoke_gmDispatch
 */

#include "gmDispatch/DispatcherFactory.hpp"
#include "gmDispatch/channels/EventBusChannel.hpp"

#include <any>
#include <cassert>
#include <iostream>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

static void pass(const std::string& name)
{
    std::cout << "[PASS] " << name << "\n";
}

static void fail(const std::string& name, const std::string& reason)
{
    std::cout << "[FAIL] " << name << " — " << reason << "\n";
}

// ── Test 1: exact-match routing via EventBusChannel ───────────────────────────

static void test_exactMatch()
{
    const std::string TEST = "test_exactMatch";

    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createSyncDispatcher("Bus");

    std::shared_ptr<GmDispatch::EventBusChannel> ch =
        std::make_shared<GmDispatch::EventBusChannel>();

    int count = 0;
    ch->addHandler([&](const GmDispatch::Envelope& env) {
        if (env.typeId == "engine.tick") ++count;
    });

    bus.subscribe("engine.tick", ch);

    GmDispatch::Envelope env;
    env.typeId = "engine.tick";
    env.source = "CoreEngine";
    bus.dispatch(env);
    bus.dispatch(env);

    // "input.key" should not reach the channel
    GmDispatch::Envelope other;
    other.typeId = "input.key";
    bus.dispatch(other);

    if (count == 2) pass(TEST);
    else            fail(TEST, "expected 2 calls, got " + std::to_string(count));
}

// ── Test 2: wildcard "*" routing ──────────────────────────────────────────────

static void test_wildcardRouting()
{
    const std::string TEST = "test_wildcardRouting";

    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createSyncDispatcher("Bus");

    std::shared_ptr<GmDispatch::EventBusChannel> ch =
        std::make_shared<GmDispatch::EventBusChannel>();

    int count = 0;
    ch->addHandler([&](const GmDispatch::Envelope&) { ++count; });

    bus.subscribe("*", ch);

    GmDispatch::Envelope e1; e1.typeId = "engine.tick";
    GmDispatch::Envelope e2; e2.typeId = "input.key";
    GmDispatch::Envelope e3; e3.typeId = "ui.button";

    bus.dispatch(e1);
    bus.dispatch(e2);
    bus.dispatch(e3);

    if (count == 3) pass(TEST);
    else            fail(TEST, "expected 3 calls, got " + std::to_string(count));
}

// ── Test 3: unsubscribe stops delivery ───────────────────────────────────────

static void test_unsubscribe()
{
    const std::string TEST = "test_unsubscribe";

    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createSyncDispatcher("Bus");

    std::shared_ptr<GmDispatch::EventBusChannel> ch =
        std::make_shared<GmDispatch::EventBusChannel>();

    int count = 0;
    ch->addHandler([&](const GmDispatch::Envelope&) { ++count; });

    bus.subscribe("evt", ch);

    GmDispatch::Envelope env; env.typeId = "evt";
    bus.dispatch(env);          // count → 1

    bus.unsubscribe("evt", ch);
    bus.dispatch(env);          // should be ignored

    if (count == 1) pass(TEST);
    else            fail(TEST, "expected 1 call, got " + std::to_string(count));
}

// ── Test 4: auto-timestamp injection ─────────────────────────────────────────

static void test_autoTimestamp()
{
    const std::string TEST = "test_autoTimestamp";

    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createSyncDispatcher("Bus");

    std::shared_ptr<GmDispatch::EventBusChannel> ch =
        std::make_shared<GmDispatch::EventBusChannel>();

    std::chrono::system_clock::time_point captured;
    ch->addHandler([&](const GmDispatch::Envelope& env) {
        captured = env.timestamp;
    });

    bus.subscribe("t", ch);

    GmDispatch::Envelope env;
    env.typeId = "t";
    // Leave timestamp at default epoch
    bus.dispatch(env);

    const bool stamped =
        (captured != std::chrono::system_clock::time_point{});

    if (stamped) pass(TEST);
    else         fail(TEST, "timestamp was not auto-set");
}

// ── Test 5: payload round-trip ────────────────────────────────────────────────

static void test_payloadRoundTrip()
{
    const std::string TEST = "test_payloadRoundTrip";

    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createSyncDispatcher("Bus");

    std::shared_ptr<GmDispatch::EventBusChannel> ch =
        std::make_shared<GmDispatch::EventBusChannel>();

    struct TickData { int frame; float dt; };

    TickData received{};
    bool     ok = false;

    ch->addHandler([&](const GmDispatch::Envelope& env) {
        if (env.payload.type() == typeid(TickData)) {
            received = std::any_cast<TickData>(env.payload);
            ok = true;
        }
    });

    bus.subscribe("engine.tick", ch);

    GmDispatch::Envelope env;
    env.typeId  = "engine.tick";
    env.payload = TickData{42, 0.016f};
    bus.dispatch(env);

    if (ok && received.frame == 42) pass(TEST);
    else                            fail(TEST, "payload cast failed or wrong value");
}

// ── Test 6: createDebugDispatcher prints to stdout ───────────────────────────

static void test_debugDispatcher()
{
    const std::string TEST = "test_debugDispatcher";

    // Just verify it constructs and dispatches without crashing.
    GmDispatch::Dispatcher bus =
        GmDispatch::DispatcherFactory::createDebugDispatcher("DebugBus");

    std::cout << "  [debug output below \u2014 one JSON line expected]\n";

    GmDispatch::Envelope env;
    env.typeId    = "engine.tick";
    env.source    = "CoreEngine";
    env.messageId = "msg-001";
    env.targets   = {"UI", "AI"};

    struct TickData { int frame; };
    env.payload = TickData{1};

    bus.dispatch(env);

    pass(TEST);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "=== gmDispatch Phase 2 smoke test ===\n\n";

    test_exactMatch();
    test_wildcardRouting();
    test_unsubscribe();
    test_autoTimestamp();
    test_payloadRoundTrip();
    test_debugDispatcher();

    std::cout << "\nDone.\n";
    return 0;
}
