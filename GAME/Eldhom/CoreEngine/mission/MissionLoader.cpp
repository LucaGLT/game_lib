/**
 * @file mission/MissionLoader.cpp
 * @brief Implementation of MissionLoader — JSON to C++ data structures.
 */

#include "GAME/Eldhom/CoreEngine/mission/MissionLoader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace eldhom {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

nlohmann::json read_json_file(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open())
	{
		throw std::runtime_error("MissionLoader: cannot open file: " + path);
	}
	nlohmann::json j;
	try
	{
		f >> j;
	}
	catch (const nlohmann::json::parse_error& ex)
	{
		throw std::runtime_error(
			"MissionLoader: JSON parse error in " + path + ": " + ex.what());
	}
	return j;
}

gmAlea::CardType parse_card_type(const std::string& s)
{
	if (s == "SEQ_START")    { return gmAlea::CardType::SEQ_START; }
	if (s == "SEQ_CONTINUE") { return gmAlea::CardType::SEQ_CONTINUE; }
	if (s == "SEQ_END")      { return gmAlea::CardType::SEQ_END; }
	if (s == "INSTANT")      { return gmAlea::CardType::INSTANT; }
	return gmAlea::CardType::SINGLE; // default
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// parse_hero_card
// ─────────────────────────────────────────────────────────────────────────────

EldhomCard MissionLoader::parse_hero_card(const nlohmann::json& j)
{
	EldhomCard c;
	c.card_id           = j.value("card_id", std::string{});
	c.name              = j.value("name", std::string{});
	c.origin            = j.value("origin", std::string{});
	c.card_type         = parse_card_type(j.value("card_type", std::string{"SINGLE"}));
	c.timeline_cost     = j.value("timeline_cost", 2);
	c.can_target_backline = j.value("can_target_backline", false);
	c.reaction_trigger  = j.value("reaction_trigger", std::string{});

	if (j.contains("effects"))
	{
		for (const nlohmann::json& ej : j["effects"])
		{
			EldhomEffect eff;
			eff.effect_type = ej.value("effect_type", std::string{});
			eff.amount      = ej.value("amount", 0);
			eff.target      = ej.value("target", std::string{});
			eff.condition   = ej.value("condition", std::string{});
			eff.value       = ej.value("value", std::string{});
			c.effects.push_back(std::move(eff));
		}
	}
	return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_behavior_card
// ─────────────────────────────────────────────────────────────────────────────

gmActor::BehaviorCard MissionLoader::parse_behavior_card(const nlohmann::json& j)
{
	gmActor::BehaviorCard bc;
	bc.card_id           = j.value("card_id", std::string{});
	bc.reaction_trigger  = j.value("reaction_trigger", std::string{});
	bc.reaction_interrupts = j.value("reaction_interrupts", false);

	auto parse_steps = [](const nlohmann::json& arr) {
		std::vector<gmActor::BehaviorStep> out;
		for (const nlohmann::json& sj : arr)
		{
			gmActor::BehaviorStep step;
			step.effect_type   = sj.value("effect_type", std::string{});
			step.amount        = sj.value("amount", 0);
			step.value         = sj.value("value", std::string{});
			step.timeline_cost = sj.value("timeline_cost", 1);
			step.optional      = sj.value("optional", false);
			out.push_back(std::move(step));
		}
		return out;
	};

	if (j.contains("steps"))        { bc.steps          = parse_steps(j["steps"]); }
	if (j.contains("fallback_steps")) { bc.fallback_steps = parse_steps(j["fallback_steps"]); }
	if (j.contains("reaction_steps")) { bc.reaction_steps = parse_steps(j["reaction_steps"]); }

	return bc;
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_mission
// ─────────────────────────────────────────────────────────────────────────────

MissionDefinition MissionLoader::parse_mission(const nlohmann::json& j)
{
	MissionDefinition def;
	def.mission_id  = j.value("mission_id", std::string{});
	def.title       = j.value("title",      std::string{});
	def.description = j.value("description", std::string{});

	// ── Locations ─────────────────────────────────────────────────────────────
	if (j.contains("locations"))
	{
		for (const nlohmann::json& lj : j["locations"])
		{
			LocationNode loc;
			loc.id   = lj.value("id", std::string{});
			loc.name = lj.value("name", std::string{});
			if (lj.contains("adjacent"))
			{
				for (const nlohmann::json& adj : lj["adjacent"])
				{
					loc.adjacent.push_back(adj.get<std::string>());
				}
			}
			def.locations.push_back(std::move(loc));
		}
	}

	// ── PG roster ─────────────────────────────────────────────────────────────
	if (j.contains("pg_roster"))
	{
		for (const nlohmann::json& pj : j["pg_roster"])
		{
			PgEntry pg;
			pg.hero_id        = pj.value("hero_id",       std::string{});
			pg.display_name   = pj.value("display_name",  std::string{});
			pg.class_name     = pj.value("class_name",    std::string{});
			pg.faction_id     = pj.value("faction_id",    std::string{"HEROES"});
			pg.start_location = pj.value("start_location", std::string{});
			pg.start_position = pj.value("start_position", std::string{"FRONTLINE"});
			pg.max_hp         = pj.value("max_hp",        6);
			pg.hand_limit     = pj.value("hand_limit",    5);
			pg.level          = pj.value("level",         1);
			pg.start_timeline = pj.value("start_timeline", 0);

			if (pj.contains("mission_deck"))
			{
				for (const nlohmann::json& cid : pj["mission_deck"])
				{
					pg.mission_deck.push_back(cid.get<std::string>());
				}
			}
			def.pg_roster.push_back(std::move(pg));
		}
	}

	// ── Monster groups ────────────────────────────────────────────────────────
	if (j.contains("monster_groups"))
	{
		for (const nlohmann::json& gj : j["monster_groups"])
		{
			MonsterGroupEntry grp;
			grp.group_id      = gj.value("group_id",      std::string{});
			grp.display_name  = gj.value("display_name",  std::string{});
			grp.monster_type  = gj.value("monster_type",  std::string{});
			grp.faction_id    = gj.value("faction_id",    std::string{});
			grp.start_location = gj.value("start_location", std::string{});
			grp.start_timeline = gj.value("start_timeline", 4);
			grp.tie_break_rank = gj.value("tie_break_rank", 3);

			if (gj.contains("behavior_deck"))
			{
				for (const nlohmann::json& bid : gj["behavior_deck"])
				{
					grp.behavior_deck.push_back(bid.get<std::string>());
				}
			}

			if (gj.contains("instances"))
			{
				for (const nlohmann::json& ij : gj["instances"])
				{
					MonsterInstanceEntry inst;
					inst.instance_id = ij.value("instance_id", std::string{});
					inst.position    = ij.value("position",    std::string{"FRONTLINE"});
					inst.start_location = ij.value("start_location", grp.start_location);
					inst.max_hp      = ij.value("max_hp",      3);
					inst.damage      = ij.value("damage",      1);
					inst.movement    = ij.value("movement",    2);
					grp.instances.push_back(std::move(inst));
				}
			}
			def.monster_groups.push_back(std::move(grp));
		}
	}

	// ── Special objects ───────────────────────────────────────────────────────
	if (j.contains("special_objects"))
	{
		for (const nlohmann::json& so : j["special_objects"])
		{
			SpecialObject obj;
			obj.object_id   = so.value("object_id",   std::string{});
			obj.name        = so.value("name",         std::string{});
			obj.type        = so.value("type",         std::string{});
			obj.location_id = so.value("location_id",  std::string{});

			if (so.contains("on_interact"))
			{
				const nlohmann::json& oi = so["on_interact"];
				obj.on_interact.type = oi.value("type", std::string{});

				// Support both "adjacency" and "adjacency_unlock" keys (PICKUP_TESORO uses
				// "adjacency_unlock" in the JSON to distinguish from LEVER "adjacency").
				const char* adj_key = oi.contains("adjacency") ? "adjacency"
				                                                : "adjacency_unlock";
				if (oi.contains(adj_key))
				{
					for (const nlohmann::json& pair : oi[adj_key])
					{
						if (pair.is_array() && pair.size() == 2)
						{
							obj.on_interact.adjacency.emplace_back(
								pair[0].get<std::string>(),
								pair[1].get<std::string>());
						}
					}
				}
			}

			def.special_objects.push_back(std::move(obj));
		}
	}

	// ── Victory / defeat conditions ───────────────────────────────────────────
	if (j.contains("victory_conditions"))
	{
		for (const nlohmann::json& vc : j["victory_conditions"])
		{
			VictoryCondition v;
			v.type            = vc.value("type",            std::string{});
			v.target_location = vc.value("target_location", std::string{});
			v.require_item    = vc.value("require_item",    std::string{});
			def.victory_conditions.push_back(v);
		}
	}

	if (j.contains("defeat_conditions"))
	{
		for (const nlohmann::json& dc : j["defeat_conditions"])
		{
			DefeatCondition d;
			d.type      = dc.value("type",      std::string{});
			d.threshold = dc.value("threshold", 60);
			def.defeat_conditions.push_back(d);
		}
	}

	// ── Timeline events ───────────────────────────────────────────────────────
	if (j.contains("timeline_events"))
	{
		for (const nlohmann::json& te : j["timeline_events"])
		{
			TimelineEvent ev;
			ev.at_time     = te.value("at_time",     0);
			ev.effect_type = te.value("effect_type", std::string{});
			ev.payload     = te.value("payload",     std::string{});
			ev.repeating   = te.value("repeating",   false);
			def.timeline_events.push_back(ev);
		}
	}

	return def;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

MissionDefinition MissionLoader::load_mission(
	const std::string& data_dir,
	const std::string& mission_id)
{
	// mission_id "missione_01" → file "mission_01.json"
	// Strip "missione_" prefix if present to build filename
	std::string file_stem = mission_id;
	if (file_stem.rfind("missione_", 0) == 0)
	{
		file_stem = "mission_" + file_stem.substr(std::string{"missione_"}.size());
	}

	std::string path = data_dir + "/" + file_stem + ".json";
	return parse_mission(read_json_file(path));
}

std::unordered_map<CardId, EldhomCard>
MissionLoader::load_card_catalog(const std::string& data_dir)
{
	namespace fs = std::filesystem;
	std::unordered_map<CardId, EldhomCard> catalog;

	// Load all files matching "cards_*.json" in data_dir.
	// This includes cards_base.json and any mission-specific card files
	// (e.g., cards_mission_0.json, cards_etnia_*.json, etc.).
	std::error_code ec;
	for (const fs::directory_entry& entry :
		 fs::directory_iterator(data_dir, ec))
	{
		if (!entry.is_regular_file()) { continue; }
		const std::string filename = entry.path().filename().string();
		if (filename.rfind("cards_", 0) != 0) { continue; }
		if (filename.size() < 12) { continue; } // "cards_" (6) + name (1+) + ".json" (5) = 12
		if (filename.substr(filename.size() - 5) != ".json") { continue; }
		try
		{
			nlohmann::json j = read_json_file(entry.path().string());
			for (const nlohmann::json& cj : j)
			{
				EldhomCard card = parse_hero_card(cj);
				catalog[card.card_id] = std::move(card);
			}
		}
		catch (const std::exception& ex)
		{
			std::cerr << "[MissionLoader] Warning: cannot load " << filename
			          << ": " << ex.what() << "\n";
		}
	}
	return catalog;
}

std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>
MissionLoader::load_behavior_catalog(const std::string& path)
{
	nlohmann::json j = read_json_file(path);

	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> catalog;
	for (const nlohmann::json& bj : j)
	{
		gmActor::BehaviorCard bc = parse_behavior_card(bj);
		catalog[bc.card_id] = std::move(bc);
	}
	return catalog;
}

std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>
MissionLoader::load_behavior_catalogs_for_mission(
	const MissionDefinition& def,
	const std::string&       data_dir)
{
	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> merged;

	// Collect unique monster types
	std::vector<std::string> loaded_types;
	for (const MonsterGroupEntry& grp : def.monster_groups)
	{
		const std::string& mt = grp.monster_type;
		bool already = false;
		for (const std::string& lt : loaded_types)
		{
			if (lt == mt) { already = true; break; }
		}
		if (already) { continue; }
		loaded_types.push_back(mt);

		std::string path = data_dir + "/behavior_" + mt + ".json";
		auto partial = load_behavior_catalog(path);
		for (auto& kv : partial)
		{
			merged[kv.first] = std::move(kv.second);
		}
	}
	return merged;
}

std::vector<std::string> MissionLoader::list_missions(const std::string& data_dir)
{
	std::vector<std::string> result;
	namespace fs = std::filesystem;

	std::error_code ec;
	for (const fs::directory_entry& entry :
		 fs::directory_iterator(data_dir, ec))
	{
		if (!entry.is_regular_file()) { continue; }
		const std::string filename = entry.path().filename().string();
		if (filename.rfind("mission_", 0) == 0 &&
		    filename.size() > 8 &&
		    filename.substr(filename.size() - 5) == ".json")
		{
			result.push_back(filename);
		}
	}
	std::sort(result.begin(), result.end());
	return result;
}

} // namespace eldhom
