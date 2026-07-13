#ifndef ELDHOM_MISSION_MISSIONLOADER_HPP
#define ELDHOM_MISSION_MISSIONLOADER_HPP

/**
 * @file mission/MissionLoader.hpp
 * @brief Loads a MissionDefinition + card catalogs from JSON files on disk.
 *
 * Three JSON files are parsed:
 * - `cards_base.json`             → `std::unordered_map<CardId, EldhomCard>`
 * - `behavior_<type>.json` files  → `std::unordered_map<CardId, BehaviorCard>`
 * - `mission_NN.json`             → `MissionDefinition`
 *
 * The behavior JSON files to load are inferred from the monster types listed
 * in the mission file (e.g. monster_type "brigante_comune" → loads
 * "behavior_brigante_comune.json" from the same data directory).
 *
 * All paths are relative to the provided `data_dir` argument.
 * Throws `std::runtime_error` on parse or I/O failure.
 */

#include "GAME/Eldhom/CoreEngine/engine/CardData.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"

#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmSave/json.hpp"

#include <string>
#include <unordered_map>

namespace eldhom {

/**
 * @class MissionLoader
 * @brief Static factory: loads mission definition and card catalogs from JSON.
 */
class MissionLoader
{
public:
	MissionLoader() = delete;

	/**
	 * @brief Loads `mission_NN.json` from `data_dir` plus the associated card
	 *        and behavior catalogs.
	 *
	 * @param data_dir   Path to the directory containing the JSON files
	 *                   (e.g. "GAME/Eldhom/data").
	 * @param mission_id Mission identifier (e.g. "missione_01" → reads
	 *                   "mission_01.json").
	 * @return Populated `MissionDefinition`.
	 * @throws std::runtime_error on file/parse error.
	 */
	static MissionDefinition load_mission(
		const std::string& data_dir,
		const std::string& mission_id);

	/**
	 * @brief Loads `cards_base.json` from `data_dir`.
	 *
	 * @param data_dir Path to the data directory.
	 * @return Map from CardId to EldhomCard.
	 * @throws std::runtime_error on file/parse error.
	 */
	static std::unordered_map<CardId, EldhomCard>
	load_card_catalog(const std::string& data_dir);

	/**
	 * @brief Loads one behavior JSON file.
	 *
	 * @param path Full path to the behavior JSON file.
	 * @return Map from BehaviorCard::card_id to BehaviorCard.
	 * @throws std::runtime_error on file/parse error.
	 */
	static std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>
	load_behavior_catalog(const std::string& path);

	/**
	 * @brief Loads all behavior JSON files needed for a mission.
	 *
	 * Iterates over the monster groups in `def`, determines each unique
	 * `monster_type`, and loads `behavior_<monster_type>.json`.
	 *
	 * @param def      Already-loaded MissionDefinition.
	 * @param data_dir Path to the data directory.
	 * @return Merged map of all behavior cards.
	 */
	static std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>
	load_behavior_catalogs_for_mission(
		const MissionDefinition& def,
		const std::string&       data_dir);

	/**
	 * @brief Returns the filenames of all mission JSON files in `data_dir`.
	 *
	 * Used by the GUI mission-selection screen to list available missions.
	 *
	 * @param data_dir Path to scan.
	 * @return Sorted list of "mission_XX.json" filenames (no directory prefix).
	 */
	static std::vector<std::string> list_missions(const std::string& data_dir);

private:
	static MissionDefinition   parse_mission(const nlohmann::json& j);
	static EldhomCard          parse_hero_card(const nlohmann::json& j);
	static gmActor::BehaviorCard parse_behavior_card(const nlohmann::json& j);
};

} // namespace eldhom

#endif // ELDHOM_MISSION_MISSIONLOADER_HPP
