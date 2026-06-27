#ifndef ELDHOM_MISSION_MISSIONDEFINITION_HPP
#define ELDHOM_MISSION_MISSIONDEFINITION_HPP

/**
 * @file mission/MissionDefinition.hpp
 * @brief Static data that describes a mission scenario.
 *
 * `MissionDefinition` is a pure data struct loaded from a JSON file
 * (e.g. `data/mission_01.json`).  It contains:
 * - Location graph (nodes + adjacency)
 * - PG roster (IDs, stats, starting positions, deck contents)
 * - Monster groups (IDs, type, instances, starting locations)
 * - Victory / defeat conditions
 * - Timeline events (thresholds and their effects)
 *
 * ### Loading
 *
 * Use `MissionLoader::load_from_json(path)` in a later phase.
 * For the initial prototype, `MissionBuilder` constructs `mission_01` in
 * code without parsing JSON.
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace eldhom {

// ─────────────────────────────────────────────────────────────────────────────
// Location graph
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct LocationNode
 * @brief One node in the mission location graph.
 */
struct LocationNode {
	LocationId              id;
	std::string             name;
	std::vector<LocationId> adjacent; ///< Directly reachable location IDs
};

// ─────────────────────────────────────────────────────────────────────────────
// PG roster entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct PgEntry
 * @brief Initial state for one player character.
 */
struct PgEntry {
	HeroId             hero_id;
	std::string        display_name;
	std::string        class_name;   ///< e.g. "Guerriero", "Supporto"
	std::string        faction_id;   ///< e.g. "HEROES"
	LocationId         start_location;
	std::string        start_position; ///< "FRONTLINE" or "BACKLINE"
	int                max_hp        = 6;
	int                hand_limit    = 5;
	int                level         = 1;
	std::vector<CardId> mission_deck; ///< Ordered list of card IDs for the mission deck
	int                start_timeline = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Monster group roster entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct MonsterInstanceEntry
 * @brief Initial state for one monster instance within a group.
 */
struct MonsterInstanceEntry {
	InstanceId  instance_id;
	std::string position;  ///< "FRONTLINE" or "BACKLINE"
	int         max_hp    = 3;
	int         damage    = 1;
	int         movement  = 2;
};

/**
 * @struct MonsterGroupEntry
 * @brief Initial state for one monster group.
 */
struct MonsterGroupEntry {
	GroupId                        group_id;
	std::string                    display_name;
	std::string                    monster_type;  ///< e.g. "brigante_comune"
	std::string                    faction_id;    ///< e.g. "BRIGANTI"
	LocationId                     start_location;
	int                            start_timeline = 4;
	int                            tie_break_rank = 3; ///< §2.2
	std::vector<MonsterInstanceEntry> instances;
	std::vector<BCardId>           behavior_deck; ///< Ordered behavior card IDs
};

// ─────────────────────────────────────────────────────────────────────────────
// Victory / defeat conditions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct VictoryCondition
 * @brief Encodes one victory condition for the mission.
 *
 * type values:
 * - `"ALL_MONSTERS_ELIMINATED"` — all monster groups removed
 * - `"OBJECTIVE_REACHED"` — a PG is in `target_location`
 */
struct VictoryCondition {
	std::string type;
	LocationId  target_location; ///< Used by OBJECTIVE_REACHED
};

/**
 * @struct DefeatCondition
 * @brief Encodes one defeat condition for the mission.
 *
 * type values:
 * - `"TIME_LIMIT"` — mission time >= `threshold`
 * - `"ALL_PG_KO"` — all PG actors are KO or dead
 */
struct DefeatCondition {
	std::string type;
	int         threshold = 60; ///< Used by TIME_LIMIT (⌛ value)
};

// ─────────────────────────────────────────────────────────────────────────────
// Timeline event (milestone)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct TimelineEvent
 * @brief An effect triggered when mission time reaches a threshold.
 */
struct TimelineEvent {
	int         at_time;       ///< Mission time (⌛) at which the event fires
	std::string effect_type;   ///< e.g. "SPAWN_GROUP", "NARRATIVE_LOG", "MAP_EFFECT"
	std::string payload;       ///< JSON string or simple string depending on effect_type
	bool        repeating = false; ///< If true, fires every `at_time` ticks
};

// ─────────────────────────────────────────────────────────────────────────────
// MissionDefinition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct MissionDefinition
 * @brief Complete static description of one mission scenario.
 */
struct MissionDefinition {
	std::string                    mission_id;
	std::string                    title;
	std::string                    description;

	// ── World ─────────────────────────────────────────────────────────────────
	std::vector<LocationNode>      locations;

	// ── Actors ────────────────────────────────────────────────────────────────
	std::vector<PgEntry>           pg_roster;
	std::vector<MonsterGroupEntry> monster_groups;

	// ── Win / lose ────────────────────────────────────────────────────────────
	std::vector<VictoryCondition>  victory_conditions;
	std::vector<DefeatCondition>   defeat_conditions;

	// ── Timeline events ───────────────────────────────────────────────────────
	std::vector<TimelineEvent>     timeline_events;
};

} // namespace eldhom

#endif // ELDHOM_MISSION_MISSIONDEFINITION_HPP
