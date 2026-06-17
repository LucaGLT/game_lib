/**
 * @file tests/test_flow_campaign.cpp
 * @brief Unit tests for gmFlow::Campaign, CampaignState, and SessionDefinition.
 *
 * All tests are pure-logic (no gmDispatch dependency) — Campaign uses a
 * simple callback rather than the EventBus.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmFlow/campaign/Campaign.cpp ^
 *       gmFlow/tests/test_flow_campaign.cpp ^
 *       -o test_flow_campaign.exe && test_flow_campaign.exe
 */

#include "gmFlow/campaign/Campaign.hpp"
#include "gmFlow/campaign/CampaignState.hpp"
#include "gmFlow/campaign/SessionDefinition.hpp"
#include "gmFlow/events/EventType.hpp"

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static gmFlow::SessionDefinition make_def(
    const std::string& id,
    bool               initial_unlock = false,
    std::vector<gmFlow::SessionId> requires_ = {})
{
    gmFlow::SessionDefinition d;
    d.session_id       = id;
    d.display_name     = id;
    d.initial_unlock   = initial_unlock;
    d.unlock_requires  = std::move(requires_);
    return d;
}

// ── CampaignState unit tests ──────────────────────────────────────────────────

// ── Test 1: Initial state — nothing completed or unlocked ─────────────────────

static void test_state_initial() {
    const std::string T = "state_initial";
    try {
        gmFlow::CampaignState s;
        bool ok = !s.is_completed("s1")
               && !s.is_unlocked("s1")
               && !s.has_data("key");
        ok ? pass(T) : fail(T, "initial state should be empty");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 2: mark_completed + is_completed ────────────────────────────────────

static void test_state_mark_completed() {
    const std::string T = "state_mark_completed";
    try {
        gmFlow::CampaignState s;
        s.mark_completed("s1", true);
        bool ok = s.is_completed("s1") && s.is_victory("s1")
               && !s.is_completed("s2");
        ok ? pass(T) : fail(T, "unexpected completed state");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 3: unlock + is_unlocked ─────────────────────────────────────────────

static void test_state_unlock() {
    const std::string T = "state_unlock";
    try {
        gmFlow::CampaignState s;
        s.unlock("s2");
        bool ok = s.is_unlocked("s2") && !s.is_unlocked("s1");
        ok ? pass(T) : fail(T, "unexpected unlock state");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 4: set_data / get_data ───────────────────────────────────────────────

static void test_state_data() {
    const std::string T = "state_data";
    try {
        gmFlow::CampaignState s;
        s.set_data("xp", "350");
        bool ok = s.has_data("xp")
               && s.get_data("xp") == "350"
               && s.get_data("missing", "0") == "0";
        ok ? pass(T) : fail(T, "unexpected data state");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Campaign unit tests ───────────────────────────────────────────────────────

// ── Test 5: initial_unlock sessions are unlocked at construction ──────────────

static void test_campaign_initial_unlock() {
    const std::string T = "campaign_initial_unlock";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", /*initial_unlock=*/true));
        defs.push_back(make_def("s2", /*initial_unlock=*/false));

        gmFlow::Campaign c(std::move(defs));
        bool ok = c.state().is_unlocked("s1")
               && !c.state().is_unlocked("s2");
        ok ? pass(T) : fail(T, "only initial sessions should be unlocked");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 6: start_session on locked session throws ───────────────────────────

static void test_campaign_start_locked_throws() {
    const std::string T = "campaign_start_locked_throws";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));
        defs.push_back(make_def("s2", false, {"s1"}));

        gmFlow::Campaign c(std::move(defs));
        c.start_session("s2"); // s2 not yet unlocked
        fail(T, "should have thrown ECampaignError");
    } catch (const gmFlow::ECampaignError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 7: complete_current_session records result ──────────────────────────

static void test_campaign_complete_records_result() {
    const std::string T = "campaign_complete_records_result";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));

        gmFlow::Campaign c(std::move(defs));
        c.start_session("s1");
        c.complete_current_session(/*victory=*/true);

        bool ok = c.state().is_completed("s1")
               && c.state().is_victory("s1");
        ok ? pass(T) : fail(T, "result not recorded");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 8: completing prerequisites unlocks downstream session ───────────────

static void test_campaign_unlock_propagation() {
    const std::string T = "campaign_unlock_propagation";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));
        defs.push_back(make_def("s2", false, {"s1"}));
        defs.push_back(make_def("s3", false, {"s2"}));

        gmFlow::Campaign c(std::move(defs));
        c.start_session("s1");
        c.complete_current_session(true); // unlocks s2

        bool s2_unlocked = c.state().is_unlocked("s2");
        bool s3_locked   = !c.state().is_unlocked("s3");

        c.start_session("s2");
        c.complete_current_session(true); // unlocks s3

        bool s3_unlocked = c.state().is_unlocked("s3");

        (s2_unlocked && s3_locked && s3_unlocked)
            ? pass(T)
            : fail(T, "unlock propagation incorrect");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 9: EVT_CAMPAIGN_SESSION_UNLOCKED callback fired ─────────────────────

static void test_campaign_unlock_callback() {
    const std::string T = "campaign_unlock_callback";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));
        defs.push_back(make_def("s2", false, {"s1"}));

        gmFlow::Campaign c(std::move(defs));

        std::vector<std::string> unlocked_ids;
        c.set_event_callback([&](const std::string& type, const gmFlow::SessionId& id) {
            if (type == gmFlow::EVT_CAMPAIGN_SESSION_UNLOCKED) {
                unlocked_ids.push_back(id);
            }
        });

        c.start_session("s1");
        c.complete_current_session(true);

        bool ok = unlocked_ids.size() == 1 && unlocked_ids[0] == "s2";
        ok ? pass(T) : fail(T, "expected s2 in unlock callback");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 10: EVT_CAMPAIGN_COMPLETED callback when all done ───────────────────

static void test_campaign_completed_callback() {
    const std::string T = "campaign_completed_callback";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));

        gmFlow::Campaign c(std::move(defs));

        bool completed_fired = false;
        c.set_event_callback([&](const std::string& type, const gmFlow::SessionId&) {
            if (type == gmFlow::EVT_CAMPAIGN_COMPLETED) {
                completed_fired = true;
            }
        });

        c.start_session("s1");
        c.complete_current_session(true);

        completed_fired ? pass(T) : fail(T, "EVT_CAMPAIGN_COMPLETED not fired");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 11: is_complete() — true only when all sessions done ─────────────────

static void test_campaign_is_complete() {
    const std::string T = "campaign_is_complete";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));
        defs.push_back(make_def("s2", false, {"s1"}));

        gmFlow::Campaign c(std::move(defs));

        c.start_session("s1");
        c.complete_current_session(true);
        bool not_yet = !c.is_complete();

        c.start_session("s2");
        c.complete_current_session(true);
        bool now_done = c.is_complete();

        (not_yet && now_done)
            ? pass(T)
            : fail(T, "is_complete() returned wrong value");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Test 12: complete_current_session without active session throws ───────────

static void test_complete_without_active_throws() {
    const std::string T = "complete_without_active_throws";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));

        gmFlow::Campaign c(std::move(defs));
        c.complete_current_session(true); // no active session
        fail(T, "should have thrown ECampaignError");
    } catch (const gmFlow::ECampaignError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 13: empty definitions throws at construction ─────────────────────────

static void test_empty_definitions_throws() {
    const std::string T = "empty_definitions_throws";
    try {
        gmFlow::Campaign c({});
        fail(T, "should have thrown ECampaignError");
    } catch (const gmFlow::ECampaignError&) {
        pass(T);
    } catch (...) {
        fail(T, "wrong exception type");
    }
}

// ── Test 14: loss result is recorded correctly ────────────────────────────────

static void test_campaign_loss_result() {
    const std::string T = "campaign_loss_result";
    try {
        std::vector<gmFlow::SessionDefinition> defs;
        defs.push_back(make_def("s1", true));

        gmFlow::Campaign c(std::move(defs));
        c.start_session("s1");
        c.complete_current_session(/*victory=*/false);

        bool ok = c.state().is_completed("s1")
               && !c.state().is_victory("s1");
        ok ? pass(T) : fail(T, "loss result not recorded correctly");
    } catch (...) { fail(T, "unexpected exception"); }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_flow_campaign ===\n";

    test_state_initial();
    test_state_mark_completed();
    test_state_unlock();
    test_state_data();
    test_campaign_initial_unlock();
    test_campaign_start_locked_throws();
    test_campaign_complete_records_result();
    test_campaign_unlock_propagation();
    test_campaign_unlock_callback();
    test_campaign_completed_callback();
    test_campaign_is_complete();
    test_complete_without_active_throws();
    test_empty_definitions_throws();
    test_campaign_loss_result();

    std::cout << "\nResults: " << g_pass << " passed, " << g_fail << " failed.\n";
    return g_fail == 0 ? 0 : 1;
}
