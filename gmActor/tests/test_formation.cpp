/**
 * @file tests/test_formation.cpp
 * @brief Unit tests for gmActor::FormationValidator, FormationResolver,
 *        and the built-in FormationCriteria factories.
 *
 * Covers:
 *   FormationValidator:
 *     - default rules (backline ≤ frontline)
 *     - is_valid: legal and illegal configurations
 *     - lower / upper bound semantics
 *     - backline_requires_frontline = false
 *     - max_frontline / max_backline hard caps
 *     - max_backline_per_frontline > 1 ratio
 *     - backline_overflow: returns correct excess
 *   FormationResolver:
 *     - empty criteria: original order preserved
 *     - single criterion: by_highest_hp sorts correctly
 *     - single criterion: by_lowest_timeline sorts correctly
 *     - criterion chain: tiebreak applied when first is equal
 *     - random criterion: deterministic for same seed, same input
 *     - criteria_count()
 *   FormationCriteria integration:
 *     - combined validator + resolver: correct actors selected for move
 *
 * Build (from game_lib root):
 *   cl /std:c++17 /EHsc /I. ^
 *       gmActor/stats/Health.cpp ^
 *       gmActor/stats/StatBlock.cpp ^
 *       gmActor/modifiers/Modifier.cpp ^
 *       gmActor/statuses/StatusContainer.cpp ^
 *       gmActor/items/InventoryState.cpp ^
 *       gmActor/items/EquipmentState.cpp ^
 *       gmActor/actors/ActorStore.cpp ^
 *       gmActor/actors/ActorQueries.cpp ^
 *       gmActor/formation/FormationValidator.cpp ^
 *       gmActor/formation/FormationCriteria.cpp ^
 *       gmActor/formation/FormationResolver.cpp ^
 *       gmActor/tests/test_formation.cpp ^
 *       /Fe:test_gmActor_formation.exe && test_gmActor_formation.exe
 */

#include "gmActor/formation/FormationRules.hpp"
#include "gmActor/formation/FormationValidator.hpp"
#include "gmActor/formation/FormationCriteria.hpp"
#include "gmActor/formation/FormationResolver.hpp"
#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/core/Enums.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace gmActor;

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static HeroState make_hero(const std::string& id, int hp, int timeline = 0)
{
	HeroState h;
	h.common.actor_id        = id;
	h.common.kind            = ActorKind::HERO;
	h.common.display_name    = id;
	h.common.current_hp      = hp;
	h.common.max_hp          = hp;
	h.common.timeline_position = timeline;
	h.common.life_state      = ActorLifeState::ACTIVE;
	h.common.can_act         = true;
	h.common.can_be_targeted = true;
	return h;
}

// ── FormationValidator tests ──────────────────────────────────────────────────

static void test_validator_default_rules()
{
	const std::string N = "validator_default/";
	FormationValidator v;  // default: backline <= frontline

	v.is_valid(2, 1) ? pass(N + "2fl_1bl_ok")  : fail(N + "2fl_1bl_ok",  "2fl 1bl must be valid");
	v.is_valid(2, 2) ? pass(N + "2fl_2bl_ok")  : fail(N + "2fl_2bl_ok",  "2fl 2bl must be valid");
	v.is_valid(1, 0) ? pass(N + "1fl_0bl_ok")  : fail(N + "1fl_0bl_ok",  "1fl 0bl must be valid");
	v.is_valid(0, 0) ? pass(N + "0fl_0bl_ok")  : fail(N + "0fl_0bl_ok",  "0fl 0bl must be valid");

	!v.is_valid(1, 2) ? pass(N + "1fl_2bl_bad") : fail(N + "1fl_2bl_bad", "1fl 2bl must be invalid");
	!v.is_valid(0, 1) ? pass(N + "0fl_1bl_bad") : fail(N + "0fl_1bl_bad", "0fl 1bl must be invalid (requires_frontline=true)");
}

static void test_validator_backline_equals_frontline_boundary()
{
	const std::string N = "validator_boundary/";
	FormationValidator v;

	// Exactly at limit: backline == frontline
	v.is_valid(3, 3) ? pass(N + "3fl_3bl_eq") : fail(N + "3fl_3bl_eq", "equal is valid");

	// One over limit
	!v.is_valid(3, 4) ? pass(N + "3fl_4bl_over") : fail(N + "3fl_4bl_over", "4bl > 3fl is invalid");
}

static void test_validator_backline_requires_frontline_false()
{
	const std::string N = "validator_no_req_frontline/";

	FormationRules rules;
	rules.backline_requires_frontline = false;

	FormationValidator v(rules);

	// Pure backline is allowed when backline_requires_frontline == false
	v.is_valid(0, 1) ? pass(N + "0fl_1bl_ok") : fail(N + "0fl_1bl_ok", "should be valid");
	v.is_valid(0, 3) ? pass(N + "0fl_3bl_ok") : fail(N + "0fl_3bl_ok", "should be valid");

	// But ratio still applies if frontline > 0
	!v.is_valid(1, 2) ? pass(N + "1fl_2bl_bad") : fail(N + "1fl_2bl_bad", "ratio still applies");
}

static void test_validator_max_frontline_cap()
{
	const std::string N = "validator_max_frontline/";

	FormationRules rules;
	rules.max_frontline = 3;

	FormationValidator v(rules);

	v.is_valid(3, 1)  ? pass(N + "3fl_ok")  : fail(N + "3fl_ok",  "3 == cap must be valid");
	!v.is_valid(4, 1) ? pass(N + "4fl_bad") : fail(N + "4fl_bad", "4 > cap must be invalid");
}

static void test_validator_max_backline_cap()
{
	const std::string N = "validator_max_backline/";

	FormationRules rules;
	rules.max_backline = 2;

	FormationValidator v(rules);

	v.is_valid(4, 2)  ? pass(N + "4fl_2bl_ok")  : fail(N + "4fl_2bl_ok",  "2bl == cap valid");
	!v.is_valid(4, 3) ? pass(N + "4fl_3bl_bad") : fail(N + "4fl_3bl_bad", "3bl > cap invalid");
}

static void test_validator_ratio_2()
{
	const std::string N = "validator_ratio2/";

	FormationRules rules;
	rules.max_backline_per_frontline = 2;

	FormationValidator v(rules);

	v.is_valid(2, 4)  ? pass(N + "2fl_4bl_ok")  : fail(N + "2fl_4bl_ok",  "4 <= 2*2 valid");
	!v.is_valid(2, 5) ? pass(N + "2fl_5bl_bad") : fail(N + "2fl_5bl_bad", "5 > 2*2 invalid");
}

static void test_validator_overflow()
{
	const std::string N = "validator_overflow/";
	FormationValidator v;

	(v.backline_overflow(2, 2) == 0) ? pass(N + "no_overflow")   : fail(N + "no_overflow",   "expected 0");
	(v.backline_overflow(2, 3) == 1) ? pass(N + "overflow_1")    : fail(N + "overflow_1",    "expected 1");
	(v.backline_overflow(1, 4) == 3) ? pass(N + "overflow_3")    : fail(N + "overflow_3",    "expected 3");
	(v.backline_overflow(0, 2) == 2) ? pass(N + "no_frontline")  : fail(N + "no_frontline",  "expected 2 (all in overflow)");
	(v.backline_overflow(3, 1) == 0) ? pass(N + "below_limit")   : fail(N + "below_limit",   "expected 0");
}

static void test_validator_negative_counts_invalid()
{
	const std::string N = "validator_negative/";
	FormationValidator v;

	!v.is_valid(-1, 0) ? pass(N + "neg_fl_bad") : fail(N + "neg_fl_bad", "negative frontline invalid");
	!v.is_valid(0, -1) ? pass(N + "neg_bl_bad") : fail(N + "neg_bl_bad", "negative backline invalid");
}

// ── FormationResolver tests ───────────────────────────────────────────────────

static void test_resolver_empty_criteria()
{
	const std::string N = "resolver_empty/";

	FormationResolver resolver({});
	ActorStore        store;
	store.add_hero(make_hero("a", 10));
	store.add_hero(make_hero("b", 20));
	store.add_hero(make_hero("c", 5));

	std::vector<ActorId> input = { "a", "b", "c" };
	std::vector<ActorId> result = resolver.sort_candidates(input, store);

	(result == input)
		? pass(N + "original_order")
		: fail(N + "original_order", "no criteria should preserve order");

	(resolver.criteria_count() == 0)
		? pass(N + "count_zero")
		: fail(N + "count_zero", "expected 0 criteria");
}

static void test_resolver_by_highest_hp()
{
	const std::string N = "resolver_hp/";

	FormationResolver resolver({ FormationCriteria::by_highest_hp() });
	ActorStore        store;
	store.add_hero(make_hero("low_hp",  5));
	store.add_hero(make_hero("high_hp", 20));
	store.add_hero(make_hero("mid_hp",  10));

	std::vector<ActorId> result =
	    resolver.sort_candidates({ "low_hp", "mid_hp", "high_hp" }, store);

	(result[0] == "high_hp" && result[1] == "mid_hp" && result[2] == "low_hp")
		? pass(N + "sorted_desc_hp")
		: fail(N + "sorted_desc_hp",
		       "expected high_hp > mid_hp > low_hp, got "
		       + result[0] + " " + result[1] + " " + result[2]);
}

static void test_resolver_by_lowest_timeline()
{
	const std::string N = "resolver_timeline/";

	FormationResolver resolver({ FormationCriteria::by_lowest_timeline() });
	ActorStore        store;
	store.add_hero(make_hero("t5",  10, /*timeline=*/5));
	store.add_hero(make_hero("t1",  10, /*timeline=*/1));
	store.add_hero(make_hero("t9",  10, /*timeline=*/9));

	std::vector<ActorId> result =
	    resolver.sort_candidates({ "t5", "t9", "t1" }, store);

	(result[0] == "t1" && result[1] == "t5" && result[2] == "t9")
		? pass(N + "sorted_asc_timeline")
		: fail(N + "sorted_asc_timeline",
		       "expected t1 < t5 < t9, got "
		       + result[0] + " " + result[1] + " " + result[2]);
}

static void test_resolver_criterion_chain_tiebreak()
{
	const std::string N = "resolver_chain/";

	// First criterion: by_highest_hp.  Actors a and b have equal HP.
	// Tiebreak: by_lowest_timeline.  a has lower timeline => a first.
	FormationResolver resolver({
	    FormationCriteria::by_highest_hp(),
	    FormationCriteria::by_lowest_timeline()
	});

	ActorStore store;
	store.add_hero(make_hero("b", /*hp=*/10, /*timeline=*/9));
	store.add_hero(make_hero("a", /*hp=*/10, /*timeline=*/2));

	std::vector<ActorId> result = resolver.sort_candidates({ "b", "a" }, store);

	(result[0] == "a" && result[1] == "b")
		? pass(N + "tiebreak_timeline")
		: fail(N + "tiebreak_timeline",
		       "expected a then b (lower timeline), got "
		       + result[0] + " then " + result[1]);
}

static void test_resolver_random_deterministic()
{
	const std::string N = "resolver_random/";

	// Same seed, same input => same output on two calls.
	FormationResolver resolver({ FormationCriteria::random(12345) });
	ActorStore        store;
	store.add_hero(make_hero("x", 10));
	store.add_hero(make_hero("y", 10));
	store.add_hero(make_hero("z", 10));

	std::vector<ActorId> r1 = resolver.sort_candidates({ "x", "y", "z" }, store);
	std::vector<ActorId> r2 = resolver.sort_candidates({ "x", "y", "z" }, store);

	(r1 == r2)
		? pass(N + "same_result_twice")
		: fail(N + "same_result_twice", "random criterion must be deterministic");

	(resolver.criteria_count() == 1)
		? pass(N + "count_one")
		: fail(N + "count_one", "expected 1 criterion");
}

static void test_resolver_does_not_modify_input()
{
	const std::string N = "resolver_no_modify/";

	FormationResolver resolver({ FormationCriteria::by_highest_hp() });
	ActorStore        store;
	store.add_hero(make_hero("a", 10));
	store.add_hero(make_hero("b", 20));

	std::vector<ActorId> input = { "a", "b" };
	resolver.sort_candidates(input, store);

	(input[0] == "a" && input[1] == "b")
		? pass(N + "input_unchanged")
		: fail(N + "input_unchanged", "input vector must not be modified");
}

// ── Integration: validator + resolver ────────────────────────────────────────

static void test_integration_pick_overflow_actors()
{
	const std::string N = "integration/";

	// Scenario: 1 frontliner, 3 backliners (default rules: max 1 backline).
	// Overflow = 2.  Resolver selects the 2 with highest HP to move first.
	FormationValidator validator;
	FormationResolver  resolver({ FormationCriteria::by_highest_hp() });

	ActorStore store;
	store.add_hero(make_hero("low",  5));
	store.add_hero(make_hero("high", 30));
	store.add_hero(make_hero("mid",  15));

	int fl = 1;
	int bl = 3;

	(!validator.is_valid(fl, bl))
		? pass(N + "invalid_formation")
		: fail(N + "invalid_formation", "1fl 3bl must be invalid");

	int overflow = validator.backline_overflow(fl, bl);
	(overflow == 2)
		? pass(N + "overflow_2")
		: fail(N + "overflow_2", "expected 2 overflow, got " + std::to_string(overflow));

	std::vector<ActorId> sorted =
	    resolver.sort_candidates({ "low", "high", "mid" }, store);

	// First `overflow` entries should be "high" and "mid" (top 2 by HP).
	bool ok = sorted.size() == 3
	       && sorted[0] == "high"
	       && sorted[1] == "mid"
	       && sorted[2] == "low";

	ok ? pass(N + "correct_move_order")
	   : fail(N + "correct_move_order",
	          "expected high,mid,low got "
	          + sorted[0] + "," + sorted[1] + "," + sorted[2]);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== Formation unit tests ===\n";

	test_validator_default_rules();
	test_validator_backline_equals_frontline_boundary();
	test_validator_backline_requires_frontline_false();
	test_validator_max_frontline_cap();
	test_validator_max_backline_cap();
	test_validator_ratio_2();
	test_validator_overflow();
	test_validator_negative_counts_invalid();

	test_resolver_empty_criteria();
	test_resolver_by_highest_hp();
	test_resolver_by_lowest_timeline();
	test_resolver_criterion_chain_tiebreak();
	test_resolver_random_deterministic();
	test_resolver_does_not_modify_input();

	test_integration_pick_overflow_actors();

	std::cout << "\n=== Results: " << g_pass << " passed, "
	          << g_fail << " failed ===\n";
	return g_fail == 0 ? 0 : 1;
}
