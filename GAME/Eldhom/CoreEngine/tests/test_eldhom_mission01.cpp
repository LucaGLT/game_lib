/**
 * @file tests/test_eldhom_mission01.cpp
 * @brief Functional test for Le Pergamene di Eldhom — P6 (Turno PG + Turno Gruppo Mostri).
 *
 * Tests the full turn cycle:
 *   1. Thael (FRONTLINE, ingresso) plays base_colpo_secco → no enemy in location → move first
 *   2. Thael moves to corridoio (Mossa Tattica)
 *   3. Thael attacks (base_colpo_secco) → hits brigante_A1 (1 damage)
 *   4. Velyr moves to corridoio
 *   5. Briganti A activate (Assalto card) → attack Thael
 *   6. Briganti B stay in sala (no PGs nearby)
 *   7. Continue until all Briganti eliminated → VICTORY
 *   8. Check defeat by time limit (separate scenario)
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomEngine.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/engine/CardData.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionEventSystem.hpp"

#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/behavior/BehaviorStep.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

static int s_pass = 0;
static int s_fail = 0;

void check(bool condition, const std::string& label)
{
	if (condition)
	{
		std::cout << "  [PASS] " << label << "\n";
		++s_pass;
	}
	else
	{
		std::cout << "  [FAIL] " << label << "\n";
		++s_fail;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Card catalog builder
// ─────────────────────────────────────────────────────────────────────────────

eldhom::EldhomCard make_card(
	const eldhom::CardId&    id,
	const std::string&       name,
	gmAlea::CardType         type,
	int                      cost,
	const std::vector<eldhom::EldhomEffect>& effects)
{
	eldhom::EldhomCard c;
	c.card_id       = id;
	c.name          = name;
	c.card_type     = type;
	c.timeline_cost = cost;
	c.effects       = effects;
	return c;
}

std::unordered_map<eldhom::CardId, eldhom::EldhomCard> build_card_catalog()
{
	std::unordered_map<eldhom::CardId, eldhom::EldhomCard> cat;

	// base_colpo_secco: SINGLE, 2⌛, DAMAGE 1 nearest enemy
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "NEAREST_ENEMY_FRONTLINE";
		cat["base_colpo_secco"] =
			make_card("base_colpo_secco", "Colpo Secco",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	// base_mossa_tattica: SINGLE, 1⌛, MOVE (destination passed externally)
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "MOVE";
		eff.amount      = 1;
		cat["base_mossa_tattica"] =
			make_card("base_mossa_tattica", "Mossa Tattica",
			          gmAlea::CardType::SINGLE, 1, { eff });
	}

	// base_attacco_inizio_seq: SEQ_START, 2⌛, DAMAGE 1
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		cat["base_attacco_inizio_seq"] =
			make_card("base_attacco_inizio_seq", "Attacco Inizio",
			          gmAlea::CardType::SEQ_START, 2, { eff });
	}

	// base_attacco_chiudi_seq: SEQ_END, 2⌛, DAMAGE 2
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 2;
		cat["base_attacco_chiudi_seq"] =
			make_card("base_attacco_chiudi_seq", "Attacco Chiudi",
			          gmAlea::CardType::SEQ_END, 2, { eff });
	}

	// velyr_cura_rapida: SINGLE, 2⌛, HEAL 2 self
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "HEAL";
		eff.amount      = 2;
		eff.target      = "SELF";
		cat["velyr_cura_rapida"] =
			make_card("velyr_cura_rapida", "Cura Rapida",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	return cat;
}

// ─────────────────────────────────────────────────────────────────────────────
// Behavior card catalog builder
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> build_behavior_catalog()
{
	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> cat;

	// brigante_assalto: MOVE_TOWARD_PG (optional,1⌛) → DEAL_DAMAGE (mandatory,2⌛)
	{
		gmActor::BehaviorCard card;
		card.card_id = "brigante_assalto";

		gmActor::BehaviorStep s1;
		s1.effect_type    = "MOVE_TOWARD_PG";
		s1.amount         = 1;
		s1.timeline_cost  = 1;
		s1.optional       = true;

		gmActor::BehaviorStep s2;
		s2.effect_type    = "DEAL_DAMAGE";
		s2.amount         = 1;
		s2.timeline_cost  = 2;
		s2.optional       = false;

		card.steps.push_back(s1);
		card.steps.push_back(s2);

		// fallback: just move
		gmActor::BehaviorStep fb;
		fb.effect_type   = "MOVE_TOWARD_PG";
		fb.amount        = 1;
		fb.timeline_cost = 1;
		fb.optional      = false;
		card.fallback_steps.push_back(fb);

		cat["brigante_assalto"] = card;
	}

	// brigante_avanzata_rapida: MOVE_TOWARD_PG (mandatory, 1⌛)
	{
		gmActor::BehaviorCard card;
		card.card_id = "brigante_avanzata_rapida";

		gmActor::BehaviorStep s;
		s.effect_type   = "MOVE_TOWARD_PG";
		s.amount        = 1;
		s.timeline_cost = 1;
		s.optional      = false;

		card.steps.push_back(s);

		// fallback: WAIT
		gmActor::BehaviorStep fb;
		fb.effect_type   = "WAIT";
		fb.timeline_cost = 2;
		fb.optional      = false;
		card.fallback_steps.push_back(fb);

		cat["brigante_avanzata_rapida"] = card;
	}

	return cat;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mission definition builder (mission_01 in code)
// ─────────────────────────────────────────────────────────────────────────────

eldhom::MissionDefinition build_mission_01()
{
	eldhom::MissionDefinition def;
	def.mission_id = "missione_01";
	def.title      = "L'Ombra sul Corridoio";

	// Locations
	eldhom::LocationNode ingresso;
	ingresso.id       = "ingresso";
	ingresso.adjacent = { "corridoio" };

	eldhom::LocationNode corridoio;
	corridoio.id       = "corridoio";
	corridoio.adjacent = { "ingresso", "sala" };

	eldhom::LocationNode sala;
	sala.id       = "sala";
	sala.adjacent = { "corridoio" };

	def.locations = { ingresso, corridoio, sala };

	// PGs
	eldhom::PgEntry thael;
	thael.hero_id        = "thael";
	thael.display_name   = "Thael";
	thael.class_name     = "Guerriero";
	thael.faction_id     = "HEROES";
	thael.start_location = "ingresso";
	thael.start_position = "FRONTLINE";
	thael.max_hp         = 6;
	thael.hand_limit     = 5;
	thael.level          = 1;
	thael.start_timeline = 0;

	eldhom::PgEntry velyr;
	velyr.hero_id        = "velyr";
	velyr.display_name   = "Velyr";
	velyr.class_name     = "Supporto";
	velyr.faction_id     = "HEROES";
	velyr.start_location = "ingresso";
	velyr.start_position = "BACKLINE";
	velyr.max_hp         = 5;
	velyr.hand_limit     = 5;
	velyr.level          = 1;
	velyr.start_timeline = 0;

	def.pg_roster = { thael, velyr };

	// Monster groups
	eldhom::MonsterGroupEntry briganti_A;
	briganti_A.group_id       = "briganti_A";
	briganti_A.display_name   = "Briganti A";
	briganti_A.monster_type   = "brigante_comune";
	briganti_A.faction_id     = "BRIGANTI";
	briganti_A.start_location = "corridoio";
	briganti_A.start_timeline = 4;
	briganti_A.tie_break_rank = 3;
	briganti_A.behavior_deck  = { "brigante_assalto", "brigante_avanzata_rapida",
	                               "brigante_assalto" };

	eldhom::MonsterInstanceEntry ba1;
	ba1.instance_id = "brigante_A1";
	ba1.position    = "FRONTLINE";
	ba1.max_hp      = 3;
	ba1.damage      = 1;
	ba1.movement    = 2;

	eldhom::MonsterInstanceEntry ba2;
	ba2.instance_id = "brigante_A2";
	ba2.position    = "BACKLINE";
	ba2.max_hp      = 1;   // low HP so we can kill it quickly
	ba2.damage      = 1;
	ba2.movement    = 2;

	briganti_A.instances = { ba1, ba2 };

	eldhom::MonsterGroupEntry briganti_B;
	briganti_B.group_id       = "briganti_B";
	briganti_B.display_name   = "Briganti B";
	briganti_B.monster_type   = "brigante_comune";
	briganti_B.faction_id     = "BRIGANTI";
	briganti_B.start_location = "sala";
	briganti_B.start_timeline = 4;
	briganti_B.tie_break_rank = 3;
	briganti_B.behavior_deck  = { "brigante_avanzata_rapida", "brigante_assalto" };

	eldhom::MonsterInstanceEntry bb1;
	bb1.instance_id = "brigante_B1";
	bb1.position    = "FRONTLINE";
	bb1.max_hp      = 1;   // easy to kill
	bb1.damage      = 1;
	bb1.movement    = 2;

	briganti_B.instances = { bb1 };

	def.monster_groups = { briganti_A, briganti_B };

	// Victory: ALL_MONSTERS_ELIMINATED
	eldhom::VictoryCondition vc;
	vc.type = "ALL_MONSTERS_ELIMINATED";
	def.victory_conditions = { vc };

	// Defeat: TIME_LIMIT 60
	eldhom::DefeatCondition dc;
	dc.type      = "TIME_LIMIT";
	dc.threshold = 60;
	def.defeat_conditions = { dc };

	// Defeat: ALL_PG_KO
	eldhom::DefeatCondition dc2;
	dc2.type      = "ALL_PG_KO";
	dc2.threshold = 0;
	def.defeat_conditions.push_back(dc2);

	return def;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test suites
// ─────────────────────────────────────────────────────────────────────────────

void test_initial_state()
{
	std::cout << "\n=== test_initial_state ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string& p) {
			events.push_back(t + ":" + a + ":" + p);
		});

	check(engine.timeline_position("thael")     == 0,  "Thael timeline starts at 0");
	check(engine.timeline_position("velyr")     == 0,  "Velyr timeline starts at 0");
	check(engine.timeline_position("briganti_A") == 4, "Briganti_A timeline starts at 4");
	check(engine.timeline_position("briganti_B") == 4, "Briganti_B timeline starts at 4");

	// Lowest timeline = heroes (0 < 4), tie-break RANK_HERO=1
	check(engine.next_actor() == "thael" || engine.next_actor() == "velyr",
	      "Next actor is a hero");
	check(engine.next_actor_kind() == gmActor::ActorKind::HERO,
	      "Next actor kind is HERO");

	check(!engine.is_over(), "Mission not over at start");
	check(engine.mission_time() == 0, "Mission time starts at 0");

	const gmAlea::SequenceState& seq = engine.sequence_state("thael");
	check(!seq.active, "Thael sequence starts inactive");
}

void test_pg_turn_simple_action_move()
{
	std::cout << "\n=== test_pg_turn_simple_action_move ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael is next. Move to corridoio.
	eldhom::ActionResult r = engine.do_simple_action(
		"thael", eldhom::SimpleActionType::MOVE, "corridoio");

	check(r.ok(), "Thael MOVE to corridoio OK");
	// "ingresso" and "corridoio" are different zones (no shared numeric
	// suffix), so this MOVE crosses a still-closed zone-boundary door and
	// pays +1 extra timeline cost on top of COST_SIMPLE_MOVE.
	check(engine.timeline_position("thael") == eldhom::COST_SIMPLE_MOVE + 1,
	      "Thael timeline = COST_SIMPLE_MOVE + 1 after crossing a zone door");

	// Next actor should be Velyr (timeline 0 < 1)
	check(engine.next_actor() == "velyr", "Velyr is next after Thael moves");

	// Velyr can also move
	eldhom::ActionResult r2 = engine.do_simple_action(
		"velyr", eldhom::SimpleActionType::MOVE, "corridoio");
	check(r2.ok(), "Velyr MOVE to corridoio OK");
}

void test_pg_turn_not_your_turn()
{
	std::cout << "\n=== test_pg_turn_not_your_turn ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Try to play as briganti_A before their turn
	eldhom::ActionResult r = engine.do_simple_action(
		"briganti_A", eldhom::SimpleActionType::MOVE, "ingresso");
	check(r.code == eldhom::ActionResultCode::ERR_NOT_YOUR_TURN,
	      "Monster group cannot act on PG turn");
}

void test_pg_turn_play_card_single()
{
	std::cout << "\n=== test_pg_turn_play_card_single ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string& p) {
			events.push_back(t);
		});

	// Thael moves to corridoio where enemies are
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	// Now Velyr is next — skip Velyr with a MOVE back
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "ingresso");

	// Now Thael has timeline=1, Velyr=1, monsters=4.
	// Next is a hero (Thael or Velyr), Thael=1, Velyr=1 → same rank, Thael has actor_id thael
	// Thael plays base_colpo_secco against Brigante_A1 (FRONTLINE in corridoio)
	events.clear();
	eldhom::ActionResult r = engine.play_card("thael", "base_colpo_secco");
	check(r.ok(), "Thael plays base_colpo_secco OK");
	// Start=0, +MOVE(1), +card_cost(2) = 3
	check(engine.timeline_position("thael") == eldhom::COST_SIMPLE_MOVE + 2,
	      "Thael timeline advanced by card cost");

	// Check that PG_PLAYED_CARD event was emitted
	bool played_card_event = false;
	for (const std::string& ev : events)
	{
		if (ev == eldhom::EVT_PG_PLAYED_CARD) { played_card_event = true; break; }
	}
	check(played_card_event, "EVT_PG_PLAYED_CARD was emitted");

	// brigante_A1 should have taken 1 damage (max_hp=3, now hp=2)
	const gmActor::MonsterInstanceState& ba1 =
		engine.actor_store().monster_instance("brigante_A1");
	check(ba1.common.current_hp == 2, "brigante_A1 HP is now 2 after 1 damage");
}

void test_pg_turn_sequence()
{
	std::cout << "\n=== test_pg_turn_sequence ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael moves to corridoio
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	// Velyr moves to ingresso
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "ingresso");

	// Thael plays SEQ_START — turn should NOT end
	eldhom::ActionResult r1 = engine.play_card("thael", "base_attacco_inizio_seq");
	check(r1.ok(), "SEQ_START played OK");

	const gmAlea::SequenceState& seq1 = engine.sequence_state("thael");
	check(seq1.active, "Sequence is active after SEQ_START");

	// Thael is still next (turn not over)
	check(engine.next_actor() == "thael", "Thael still next during sequence");

	// Thael plays SEQ_END — turn ends
	eldhom::ActionResult r2 = engine.play_card("thael", "base_attacco_chiudi_seq");
	check(r2.ok(), "SEQ_END played OK");

	const gmAlea::SequenceState& seq2 = engine.sequence_state("thael");
	check(!seq2.active, "Sequence closed after SEQ_END");

	// SEQ_END is turn-ending — Velyr should be next
	check(engine.next_actor() != "thael", "Thael's turn ended after SEQ_END");
}

void test_monster_group_turn()
{
	std::cout << "\n=== test_monster_group_turn ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string&) {
			events.push_back(t + ":" + a);
		});

	// Advance PG timelines past 4 using RECOVER (cost=3) twice per hero.
	// After 2 RECOVERs each PG is at timeline=6 > 4.
	// next_actor() alternates naturally between heroes.
	for (int i = 0; i < 4; ++i)
	{
		if (engine.next_actor_kind() != gmActor::ActorKind::HERO) { break; }
		const std::string& next = engine.next_actor();
		engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
	}

	check(engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP,
	      "Monster group is next after PGs advance past timeline=4");

	// Get Thael HP before monster attacks
	int thael_hp_before = engine.actor_store().hero("thael").common.current_hp;
	(void)thael_hp_before;

	events.clear();
	eldhom::ActionResult r = engine.resolve_next_group_turn();
	check(r.ok(), "resolve_next_group_turn OK");

	// Check group activated event
	bool group_activated = false;
	for (const std::string& ev : events)
	{
		if (ev.find(eldhom::EVT_GROUP_ACTIVATED) != std::string::npos)
		{
			group_activated = true; break;
		}
	}
	check(group_activated, "EVT_GROUP_ACTIVATED emitted");

	// Monster group timeline should have advanced
	check(engine.timeline_position("briganti_A") > 4,
	      "briganti_A timeline advanced after turn");
}

void test_victory_all_monsters_eliminated()
{
	std::cout << "\n=== test_victory_all_monsters_eliminated ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// Use low-HP monsters (1 HP each) so they die in one hit
	// Already configured that way in build_mission_01 (ba2=1HP, bb1=1HP)
	// ba1 has 3HP but colpo_secco does 1 damage — we'll use colpo_secco multiple times

	std::vector<std::string> victory_events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&victory_events](const eldhom::EventType& t,
		                  const std::string&,
		                  const std::string&) {
			if (t == eldhom::EVT_MISSION_VICTORY) { victory_events.push_back(t); }
		});

	// Thael moves to corridoio to be in range of Briganti A
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "corridoio");
	// Push ahead to make monsters go next
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "ingresso");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "ingresso");
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "corridoio");

	// Let briganti_A activate first (Assalto card — they are in corridoio)
	if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
	{
		engine.resolve_next_group_turn();
	}
	if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
	{
		engine.resolve_next_group_turn();
	}

	// Now kill ALL monsters with simple attacks (Thael and Velyr take turns)
	// brigante_A1: 3HP — needs 3 attacks
	// brigante_A2: 1HP — needs 1 attack (but also dies from formation targeting)
	// brigante_B1: 1HP — needs 1 attack after moving to sala

	// Run turns for up to 30 steps without a loop
	int steps = 0;
	while (!engine.is_over() && steps < 30)
	{
		++steps;
		const std::string& next = engine.next_actor();
		if (next.empty()) { break; }

		gmActor::ActorKind kind = engine.next_actor_kind();

		if (kind == gmActor::ActorKind::HERO)
		{
			const gmActor::HeroState& h = engine.actor_store().hero(next);
			const std::string& loc      = h.common.area_id;

			// Check if we can attack here
			bool has_enemy = false;
			for (const auto& kv : engine.actor_store().monster_instances())
			{
				if (!kv.second.common.removed &&
				    kv.second.common.area_id == loc)
				{
					has_enemy = true;
					break;
				}
			}

			if (has_enemy)
			{
				engine.play_card(next, "base_colpo_secco");
			}
			else
			{
				// Move toward monsters
				// Simple heuristic: if in ingresso move to corridoio, if in corridoio move to sala
				std::string dest;
				if (loc == "ingresso")     { dest = "corridoio"; }
				else if (loc == "corridoio") { dest = "sala"; }
				else                       { dest = "corridoio"; }

				eldhom::ActionResult r = engine.do_simple_action(
					next, eldhom::SimpleActionType::MOVE, dest);
				if (!r.ok())
				{
					// Can't move, just attack in current loc
					engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
				}
			}
		}
		else if (kind == gmActor::ActorKind::MONSTER_GROUP)
		{
			engine.resolve_next_group_turn();
		}
		else
		{
			break;
		}
	}

	check(!victory_events.empty() || engine.mission_outcome() == eldhom::MissionOutcome::VICTORY,
	      "Mission ended in VICTORY (all monsters eliminated)");
	check(steps < 30, "Mission completed within 30 steps");
}

void test_defeat_time_limit()
{
	std::cout << "\n=== test_defeat_time_limit ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// Set time limit to 10 ticks for quick defeat
	def.defeat_conditions.clear();
	eldhom::DefeatCondition dc;
	dc.type      = "TIME_LIMIT";
	dc.threshold = 10;
	def.defeat_conditions.push_back(dc);
	// Remove ALL_PG_KO to avoid early defeat
	// (monsters can't reach PGs in ingresso if we keep them away)

	std::vector<std::string> defeat_events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&defeat_events](const eldhom::EventType& t,
		                 const std::string&,
		                 const std::string&) {
			if (t == eldhom::EVT_MISSION_DEFEAT) { defeat_events.push_back(t); }
		});

	// Do 5 RECOVER actions (cost 3 each) on Thael to advance time > 10
	for (int i = 0; i < 4 && !engine.is_over(); ++i)
	{
		const std::string& next = engine.next_actor();
		if (engine.next_actor_kind() == gmActor::ActorKind::HERO)
		{
			engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
		}
		else
		{
			engine.resolve_next_group_turn();
		}
	}

	check(!defeat_events.empty() || engine.mission_outcome() == eldhom::MissionOutcome::DEFEAT,
	      "Mission ended in DEFEAT (time limit exceeded)");
}

void test_zone_door_blocks_monster_until_pg_crosses()
{
	std::cout << "\n=== test_zone_door_blocks_monster_until_pg_crosses ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Move whichever hero is next into corridoio, twice, so both heroes end
	// up there (opens the ingresso<->corridoio door; irrelevant to this test).
	for (int i = 0; i < 2; ++i)
	{
		const std::string next = engine.next_actor();
		check(engine.do_simple_action(next, eldhom::SimpleActionType::MOVE, "corridoio").ok(),
		      next + " moves to corridoio");
	}

	// briganti_B starts in "sala" (a different zone from "corridoio"): the
	// "sala"<->"corridoio" door is still closed, so briganti_B must NOT be
	// able to cross it even though PGs are waiting right next door.
	engine.resolve_group_turn_for("briganti_B");
	const gmActor::MonsterInstanceState& bb1_before =
		engine.actor_store().monster_instance("brigante_B1");
	check(bb1_before.common.area_id == "sala",
	      "briganti_B stays in sala while the zone door is still closed");

	// Whichever hero is next crosses into sala (opens the zone door), then
	// walks back to corridoio once it is their turn again (the other hero
	// may take filler RECOVER turns in between; turn order does not matter
	// for this test since both heroes stay put in corridoio/sala).
	const std::string crosser = engine.next_actor();
	check(engine.do_simple_action(crosser, eldhom::SimpleActionType::MOVE, "sala").ok(),
	      crosser + " crosses into sala (opens the zone door)");

	int guard = 0;
	while (engine.next_actor() != crosser && guard < 20)
	{
		if (engine.next_actor_kind() == gmActor::ActorKind::HERO)
		{
			engine.do_simple_action(engine.next_actor(), eldhom::SimpleActionType::RECOVER);
		}
		else if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
		{
			// Whichever group activates early finds its target still in
			// "sala" (crosser hasn't walked back yet) and simply stays put
			// ("already in contact"), which does not affect this test.
			engine.resolve_next_group_turn();
		}
		else
		{
			break;
		}
		++guard;
	}
	check(engine.do_simple_action(crosser, eldhom::SimpleActionType::MOVE, "corridoio").ok(),
	      crosser + " walks back to corridoio");

	// Now that the door is open, briganti_B should be able to cross it.
	engine.resolve_group_turn_for("briganti_B");
	const gmActor::MonsterInstanceState& bb1_after =
		engine.actor_store().monster_instance("brigante_B1");
	check(bb1_after.common.area_id == "corridoio",
	      "briganti_B crosses into corridoio once the zone door is open");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "Le Pergamene di Eldhom — test_eldhom_mission01\n";
	std::cout << "================================================\n";

	test_initial_state();
	test_pg_turn_simple_action_move();
	test_pg_turn_not_your_turn();
	test_pg_turn_play_card_single();
	test_pg_turn_sequence();
	test_monster_group_turn();
	test_victory_all_monsters_eliminated();
	test_defeat_time_limit();
	test_zone_door_blocks_monster_until_pg_crosses();

	std::cout << "\n================================================\n";
	std::cout << "PASS: " << s_pass << "   FAIL: " << s_fail << "\n";

	return s_fail == 0 ? 0 : 1;
}
