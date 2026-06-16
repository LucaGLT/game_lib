/**
 * @file tests/test_flow_sequential.cpp
 * @brief Integration tests for gmFlow::SequentialFlowController + GameSession.
 *
 * Covers: session lifecycle, phase transitions, turn sequencing, round caps,
 *         simultaneous turns, action submission and execution, event delivery.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmFlow/core/Result.cpp ^
 *       gmFlow/core/GameContext.cpp ^
 *       gmFlow/actors/ActorRegistry.cpp ^
 *       gmFlow/actions/ActionQueue.cpp ^
 *       gmFlow/actions/ActionWindow.cpp ^
 *       gmFlow/actions/StepBasedAction.cpp ^
 *       gmFlow/flow/Turn.cpp ^
 *       gmFlow/flow/Round.cpp ^
 *       gmFlow/flow/SequentialFlowController.cpp ^
 *       gmFlow/events/EventBus.cpp ^
 *       gmFlow/session/GameSession.cpp ^
 *       gmDispatch/Dispatcher.cpp ^
 *       gmDispatch/DispatcherFactory.cpp ^
 *       gmDispatch/channels/EventBusChannel.cpp ^
 *       gmDispatch/channels/StdoutChannel.cpp ^
 *       gmDispatch/serializers/JsonSerializer.cpp ^
 *       gmDispatch/routers/SyncRouter.cpp ^
 *       gmDispatch/dispatchers/SyncDispatcher.cpp ^
 *       gmFlow/tests/test_flow_sequential.cpp ^
 *       -o test_flow_sequential.exe && test_flow_sequential.exe
 */

#include "gmFlow/session/GameSession.hpp"
#include "gmFlow/session/SessionConfig.hpp"
#include "gmFlow/flow/SequentialFlowController.hpp"
#include "gmFlow/flow/IPhase.hpp"
#include "gmFlow/actions/IAction.hpp"
#include "gmFlow/actions/ActionStatus.hpp"
#include "gmFlow/actions/StepBasedAction.hpp"
#include "gmFlow/core/GameState.hpp"
#include "gmFlow/core/Result.hpp"
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

static void pass(const std::string& name) {
    std::cout << "[PASS] " << name << "\n";
    ++g_pass;
}
static void fail(const std::string& name, const std::string& reason) {
    std::cout << "[FAIL] " << name << " -- " << reason << "\n";
    ++g_fail;
}

// ── Shared test infrastructure ────────────────────────────────────────────────

namespace {

// Minimal concrete GameState used across all tests.
struct SimpleState : public gmFlow::GameState {
    int  actions_executed = 0;
    bool complete         = false;

    const gmFlow::SessionId& session_id() const override { return id_; }
    void on_session_started(const gmFlow::SessionId& id)  override { id_ = id; }
    void on_session_completed()                           override {}

private:
    gmFlow::SessionId id_;
};

// Phase that completes after `n` turns (counts ActionCompletedEvents).
class CountingPhase : public gmFlow::IPhase {
public:
    explicit CountingPhase(std::string id, int turns_to_complete)
        : id_(std::move(id)), turns_needed_(turns_to_complete) {}

    gmFlow::PhaseId id() const override { return id_; }

    void on_enter(gmFlow::GameContext&) override { ++entered; }
    void on_exit(gmFlow::GameContext&)  override { ++exited;  }

    std::vector<std::unique_ptr<gmFlow::IAction>>
    available_actions(const gmFlow::GameContext&, const gmFlow::ActorId&) const override {
        return {};
    }
    bool is_complete(const gmFlow::GameContext& ctx) const override {
        const SimpleState& s = static_cast<const SimpleState&>(ctx.state());
        return s.actions_executed >= turns_needed_;
    }

    int entered = 0;
    int exited  = 0;

private:
    std::string id_;
    int         turns_needed_;
};

// Simple action that increments SimpleState::actions_executed.
class CountAction : public gmFlow::IAction {
public:
    CountAction(std::string id, std::string owner)
        : id_(std::move(id)), owner_(std::move(owner)) {}

    gmFlow::ActionId     id()     const override { return id_; }
    gmFlow::ActorId      owner()  const override { return owner_; }
    gmFlow::ActionStatus status() const override { return status_; }

    gmFlow::ValidationResult validate(const gmFlow::GameContext&) const override {
        return gmFlow::ValidationResult::ok();
    }
    gmFlow::ActionResult execute(gmFlow::GameContext& ctx) override {
        SimpleState& s = static_cast<SimpleState&>(ctx.state());
        ++s.actions_executed;
        status_ = gmFlow::ActionStatus::COMPLETED;
        return gmFlow::ActionResult::success();
    }

    bool is_async()      const override { return false; }
    bool requires_turn() const override { return true;  }
    bool is_multi_step() const override { return false; }

private:
    gmFlow::ActionId     id_;
    gmFlow::ActorId      owner_;
    gmFlow::ActionStatus status_ = gmFlow::ActionStatus::CREATED;
};

// Factory: build a GameSession with one or two actors and the given phases.
struct SessionBuilder {
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
        for (const std::string& id : actor_ids) {
            cfg.actors.emplace_back(id, gmFlow::ActorType::PLAYER);
        }
        cfg.turn_policy  = tp;
        cfg.round_policy = rp;

        auto ctrl = std::make_unique<gmFlow::SequentialFlowController>(
            std::move(phases), tp, rp);

        return std::make_unique<gmFlow::GameSession>(
            std::move(cfg), std::move(ctrl), std::move(state), dispatcher);
    }
};

} // anonymous namespace

// ── Test 1: start() transitions session to RUNNING ───────────────────────────

static void test_start_transitions_to_running() {
    const std::string T = "start_transitions_to_running";
    try {
        SessionBuilder sb;
        auto phases = [] {
            std::vector<std::unique_ptr<gmFlow::IPhase>> v;
            v.push_back(std::make_unique<CountingPhase>("P1", 1));
            return v;
        }();
        auto session = sb.build(std::move(phases), {"p1"});
        bool before = (session->state() == gmFlow::SessionState::CREATED);
        session->start();
        bool after = (session->state() == gmFlow::SessionState::RUNNING);
        (before && after) ? pass(T) : fail(T, "unexpected state transition");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2: EVT_SESSION_STARTED published on start() ─────────────────────────

static void test_session_started_event() {
    const std::string T = "session_started_event";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"p1"});

        bool received = false;
        session->event_bus().subscribe(gmFlow::EVT_SESSION_STARTED,
            [&](const gmFlow::IEvent&) { received = true; });

        session->start();
        received ? pass(T) : fail(T, "EVT_SESSION_STARTED not received");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3: EVT_PHASE_ENTERED on first phase ──────────────────────────────────

static void test_phase_entered_event() {
    const std::string T = "phase_entered_event";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("SETUP", 1));
        auto session = sb.build(std::move(phases), {"p1"});

        std::string phase_id_received;
        session->event_bus().subscribe(gmFlow::EVT_PHASE_ENTERED,
            [&](const gmFlow::IEvent& e) {
                phase_id_received =
                    static_cast<const gmFlow::PhaseEnteredEvent&>(e).phase_id;
            });

        session->start();
        bool ok = phase_id_received == "SETUP";
        ok ? pass(T) : fail(T, "phase_id was '" + phase_id_received + "'");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4: EVT_TURN_STARTED with correct actor ───────────────────────────────

static void test_turn_started_event() {
    const std::string T = "turn_started_event";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"hero"});

        std::vector<gmFlow::ActorId> active;
        session->event_bus().subscribe(gmFlow::EVT_TURN_STARTED,
            [&](const gmFlow::IEvent& e) {
                active = static_cast<const gmFlow::TurnStartedEvent&>(e).active_actors;
            });

        session->start();
        bool ok = active.size() == 1 && active[0] == "hero";
        ok ? pass(T) : fail(T, "wrong active actors in TurnStartedEvent");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 5: Action submission and execution ───────────────────────────────────

static void test_action_submitted_and_executed() {
    const std::string T = "action_submitted_and_executed";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        auto vr = session->submit_action("p1",
            std::make_unique<CountAction>("act1","p1"));

        bool submitted = vr.valid();
        session->tick(); // resolves window → executes action → phase completes

        bool completed = session->is_finished();
        (submitted && completed)
            ? pass(T)
            : fail(T, "submit=" + std::string(submitted?"ok":"fail")
                   + " finished=" + std::string(completed?"yes":"no"));
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6: Submitting when not your turn fails ───────────────────────────────

static void test_submit_not_your_turn() {
    const std::string T = "submit_not_your_turn";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"p1", "p2"});
        session->start(); // p1's turn first

        // p2 tries to act — should be rejected.
        auto vr = session->submit_action("p2",
            std::make_unique<CountAction>("act_p2","p2"));
        bool ok = !vr.valid()
               && vr.error() == gmFlow::ValidationError::NOT_ACTOR_TURN;
        ok ? pass(T) : fail(T, "expected NOT_ACTOR_TURN");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 7: Two-actor sequential turn order ───────────────────────────────────

static void test_sequential_turn_order() {
    const std::string T = "sequential_turn_order";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 2));
        auto session = sb.build(std::move(phases), {"alice", "bob"});

        // Subscribe BEFORE start() so the first TurnStartedEvent is captured too.
        std::vector<std::string> turn_actors;
        session->event_bus().subscribe(gmFlow::EVT_TURN_STARTED,
            [&](const gmFlow::IEvent& e) {
                const auto& ev = static_cast<const gmFlow::TurnStartedEvent&>(e);
                if (!ev.active_actors.empty())
                    turn_actors.push_back(ev.active_actors[0]);
            });

        session->start(); // alice's turn → event captured

        // Turn 1: alice acts.
        session->submit_action("alice", std::make_unique<CountAction>("a1","alice"));
        session->tick(); // bob's turn opens → event captured

        // Turn 2: bob acts.
        session->submit_action("bob",   std::make_unique<CountAction>("a2","bob"));
        session->tick(); // phase complete → session finishes

        bool ok = turn_actors.size() == 2
               && turn_actors[0] == "alice"
               && turn_actors[1] == "bob"
               && session->is_finished();
        ok ? pass(T) : fail(T, "wrong turn order or session not finished");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8: Phase transition fires entered/exited events ─────────────────────

static void test_phase_transition_events() {
    const std::string T = "phase_transition_events";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("SETUP", 1));
        phases.push_back(std::make_unique<CountingPhase>("COMBAT", 1));
        auto session = sb.build(std::move(phases), {"p1"});

        std::vector<std::string> entered_phases, exited_phases;
        session->event_bus().subscribe(gmFlow::EVT_PHASE_ENTERED,
            [&](const gmFlow::IEvent& e) {
                entered_phases.push_back(
                    static_cast<const gmFlow::PhaseEnteredEvent&>(e).phase_id);
            });
        session->event_bus().subscribe(gmFlow::EVT_PHASE_EXITED,
            [&](const gmFlow::IEvent& e) {
                exited_phases.push_back(
                    static_cast<const gmFlow::PhaseExitedEvent&>(e).phase_id);
            });

        session->start(); // SETUP entered
        session->submit_action("p1", std::make_unique<CountAction>("a1","p1"));
        session->tick();  // SETUP completes → exits → COMBAT entered
        session->submit_action("p1", std::make_unique<CountAction>("a2","p1"));
        session->tick();  // COMBAT completes → session finished

        bool ok = entered_phases.size() == 2
               && entered_phases[0] == "SETUP"
               && entered_phases[1] == "COMBAT"
               && exited_phases.size() == 2  // SETUP and COMBAT exit events
               && exited_phases[0] == "SETUP";
        ok ? pass(T) : fail(T, "phase events not as expected");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9: Round counter — max_rounds ends session ───────────────────────────

static void test_max_rounds_ends_session() {
    const std::string T = "max_rounds_ends_session";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        // Phase never self-completes; only rounds_exhausted_ can end it.
        phases.push_back(std::make_unique<CountingPhase>("INFINITE", 999));

        gmFlow::TurnPolicy  tp;
        gmFlow::RoundPolicy rp;
        rp.max_rounds = 2;  // session must end after 2 complete actor cycles

        auto session = sb.build(std::move(phases), {"p1"}, tp, rp);
        session->start();

        // Each tick: p1 submits, window resolves (ANY_SUBMITTED → closed after 1).
        for (int i = 0; i < 10 && !session->is_finished(); ++i) {
            if (!session->is_finished()) {
                session->submit_action("p1",
                    std::make_unique<CountAction>("a"+std::to_string(i),"p1"));
                session->tick();
            }
        }

        session->is_finished()
            ? pass(T)
            : fail(T, "session should have ended after max_rounds");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 10: Simultaneous turns — both actors submit before window closes ─────

static void test_simultaneous_turns() {
    const std::string T = "simultaneous_turns";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("SIM", 2));

        gmFlow::TurnPolicy tp;
        tp.allow_simultaneous_turns = true;

        auto session = sb.build(std::move(phases), {"p1","p2"}, tp);
        session->start();

        // In simultaneous mode both actors are eligible at the same time.
        auto vr1 = session->submit_action("p1",
            std::make_unique<CountAction>("a1","p1"));
        auto vr2 = session->submit_action("p2",
            std::make_unique<CountAction>("a2","p2"));

        bool both_accepted = vr1.valid() && vr2.valid();
        session->tick();

        (both_accepted && session->is_finished())
            ? pass(T)
            : fail(T, "both actors should act simultaneously");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 11: Pause / Resume ───────────────────────────────────────────────────

static void test_pause_resume() {
    const std::string T = "pause_resume";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();

        session->pause();
        bool paused = session->is_paused();
        session->resume();
        bool running = (session->state() == gmFlow::SessionState::RUNNING);

        (paused && running) ? pass(T) : fail(T, "unexpected state during pause/resume");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 12: start() on non-CREATED session throws ───────────────────────────

static void test_double_start_throws() {
    const std::string T = "double_start_throws";
    try {
        SessionBuilder sb;
        std::vector<std::unique_ptr<gmFlow::IPhase>> phases;
        phases.push_back(std::make_unique<CountingPhase>("P1", 1));
        auto session = sb.build(std::move(phases), {"p1"});
        session->start();
        session->start(); // should throw
        fail(T, "second start() should have thrown");
    } catch (const gmFlow::EGameSessionError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_flow_sequential ===\n";

    test_start_transitions_to_running();
    test_session_started_event();
    test_phase_entered_event();
    test_turn_started_event();
    test_action_submitted_and_executed();
    test_submit_not_your_turn();
    test_sequential_turn_order();
    test_phase_transition_events();
    test_max_rounds_ends_session();
    test_simultaneous_turns();
    test_pause_resume();
    test_double_start_throws();

    std::cout << "\nResults: " << g_pass << " passed, " << g_fail << " failed.\n";
    return g_fail == 0 ? 0 : 1;
}
