/**
 * @file loader/RuleBookLoader.cpp
 * @brief Minimal stdlib-only JSON parser for RuleBookLoader.
 *
 * Parser limitations (by design — these are the only types used in the format):
 *   - Objects  { "key": value }
 *   - Arrays   [ value, value ]
 *   - Strings  "…"  (no escaped surrogate pairs; \n \t \" \\ supported)
 *   - Integers  -?[0-9]+
 *   - No floats, no nulls, no booleans at object-value level.
 */

#include "gmRules/loader/RuleBookLoader.hpp"
#include "gmRules/core/RuleDefinition.hpp"
#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/condition/ConditionSpec.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// Internal: minimal JSON value type
// ─────────────────────────────────────────────────────────────────────────────

namespace {

enum class JType { STR, INT, OBJ, ARR };

struct JVal;
using  JObj = std::unordered_map<std::string, JVal>;
using  JArr = std::vector<JVal>;

struct JVal
{
	JType       type = JType::STR;
	std::string str;
	int         num  = 0;
	JObj        obj;
	JArr        arr;

	bool is(JType t) const { return type == t; }

	const std::string& as_str(const std::string& ctx) const
	{
		if (type != JType::STR)
			throw ERuleBookError("JSON: expected string in " + ctx);
		return str;
	}

	int as_int(const std::string& ctx) const
	{
		if (type != JType::INT)
			throw ERuleBookError("JSON: expected integer in " + ctx);
		return num;
	}

	const JObj& as_obj(const std::string& ctx) const
	{
		if (type != JType::OBJ)
			throw ERuleBookError("JSON: expected object in " + ctx);
		return obj;
	}

	const JArr& as_arr(const std::string& ctx) const
	{
		if (type != JType::ARR)
			throw ERuleBookError("JSON: expected array in " + ctx);
		return arr;
	}

	// Returns string value of key, or "" if key missing.
	std::string opt_str(const std::string& key) const
	{
		if (type != JType::OBJ) return "";
		auto it = obj.find(key);
		if (it == obj.end() || it->second.type != JType::STR) return "";
		return it->second.str;
	}

	// Returns integer value of key, or default_val if key missing.
	int opt_int(const std::string& key, int default_val = 0) const
	{
		if (type != JType::OBJ) return default_val;
		auto it = obj.find(key);
		if (it == obj.end() || it->second.type != JType::INT) return default_val;
		return it->second.num;
	}

	bool has(const std::string& key) const
	{
		return type == JType::OBJ && obj.count(key) > 0;
	}
};

// ── Tokenizer / parser ────────────────────────────────────────────────────────

struct Parser
{
	const std::string& src;
	std::size_t        pos = 0;

	explicit Parser(const std::string& s) : src(s) {}

	void skip_ws()
	{
		while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
			++pos;
	}

	char peek()
	{
		skip_ws();
		if (pos >= src.size())
			throw ERuleBookError("JSON: unexpected end of input");
		return src[pos];
	}

	char consume()
	{
		skip_ws();
		if (pos >= src.size())
			throw ERuleBookError("JSON: unexpected end of input");
		return src[pos++];
	}

	void expect(char c)
	{
		char got = consume();
		if (got != c)
		{
			throw ERuleBookError(
				std::string("JSON: expected '") + c +
				"' but got '" + got + "'");
		}
	}

	std::string parse_string()
	{
		expect('"');
		std::string result;
		while (pos < src.size())
		{
			char c = src[pos++];
			if (c == '"') return result;
			if (c == '\\')
			{
				if (pos >= src.size())
					throw ERuleBookError("JSON: unterminated escape");
				char esc = src[pos++];
				switch (esc)
				{
					case '"':  result += '"';  break;
					case '\\': result += '\\'; break;
					case '/':  result += '/';  break;
					case 'n':  result += '\n'; break;
					case 'r':  result += '\r'; break;
					case 't':  result += '\t'; break;
					default:   result += esc;  break;
				}
			}
			else
			{
				result += c;
			}
		}
		throw ERuleBookError("JSON: unterminated string");
	}

	JVal parse_object()
	{
		expect('{');
		JVal v;
		v.type = JType::OBJ;
		skip_ws();
		if (peek() == '}') { consume(); return v; }
		while (true)
		{
			std::string key = parse_string();
			skip_ws();
			expect(':');
			v.obj[key] = parse_value();
			skip_ws();
			char sep = peek();
			if (sep == '}') { consume(); break; }
			if (sep == ',') { consume(); continue; }
			throw ERuleBookError("JSON: expected ',' or '}' in object");
		}
		return v;
	}

	JVal parse_array()
	{
		expect('[');
		JVal v;
		v.type = JType::ARR;
		skip_ws();
		if (peek() == ']') { consume(); return v; }
		while (true)
		{
			v.arr.push_back(parse_value());
			skip_ws();
			char sep = peek();
			if (sep == ']') { consume(); break; }
			if (sep == ',') { consume(); continue; }
			throw ERuleBookError("JSON: expected ',' or ']' in array");
		}
		return v;
	}

	JVal parse_int()
	{
		JVal v;
		v.type = JType::INT;
		std::string s;
		if (pos < src.size() && src[pos] == '-') s += src[pos++];
		while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos])))
			s += src[pos++];
		if (s.empty() || s == "-")
			throw ERuleBookError("JSON: invalid integer");
		try
		{
			v.num = std::stoi(s);
		}
		catch (const std::exception&)
		{
			throw ERuleBookError("JSON: integer out of range: " + s);
		}
		return v;
	}

	JVal parse_value()
	{
		skip_ws();
		if (pos >= src.size())
			throw ERuleBookError("JSON: expected value but got end of input");
		char c = src[pos];
		if (c == '"')                          return [&]{ JVal v; v.type = JType::STR; v.str = parse_string(); return v; }();
		if (c == '{')                          return parse_object();
		if (c == '[')                          return parse_array();
		if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_int();
		throw ERuleBookError(std::string("JSON: unexpected character '") + c + "'");
	}
};

JVal parse_json(const std::string& src)
{
	Parser p(src);
	JVal   root = p.parse_value();
	p.skip_ws();
	if (p.pos != src.size())
		throw ERuleBookError("JSON: trailing content after root value");
	return root;
}

// ── Enum converters ───────────────────────────────────────────────────────────

// Converts string to uppercase for case-insensitive matching.
std::string to_upper(std::string s)
{
	for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	return s;
}

EffectType parse_effect_type(const std::string& raw)
{
	static const std::unordered_map<std::string, EffectType> TABLE =
	{
		{ "DEAL_DAMAGE",         EffectType::DEAL_DAMAGE         },
		{ "HEAL",                EffectType::HEAL                },
		{ "MOVE_ACTOR",          EffectType::MOVE_ACTOR          },
		{ "SHIFT_POSITION",      EffectType::SHIFT_POSITION      },
		{ "DRAW_CARDS",          EffectType::DRAW_CARDS          },
		{ "DISCARD_CARDS",       EffectType::DISCARD_CARDS       },
		{ "MOVE_CARD_TO_ZONE",   EffectType::MOVE_CARD_TO_ZONE   },
		{ "APPLY_STATUS",        EffectType::APPLY_STATUS        },
		{ "REMOVE_STATUS",       EffectType::REMOVE_STATUS       },
		{ "ADD_MODIFIER",        EffectType::ADD_MODIFIER        },
		{ "REMOVE_MODIFIER",     EffectType::REMOVE_MODIFIER     },
		{ "ADD_TAG",             EffectType::ADD_TAG             },
		{ "REMOVE_TAG",          EffectType::REMOVE_TAG          },
		{ "SET_STATE",           EffectType::SET_STATE           },
		{ "SPAWN_ACTOR",         EffectType::SPAWN_ACTOR         },
		{ "DESPAWN_ACTOR",       EffectType::DESPAWN_ACTOR       },
		{ "REVIVE_ACTOR",        EffectType::REVIVE_ACTOR        },
		{ "CHANGE_TEAM",         EffectType::CHANGE_TEAM         },
		{ "MODIFY_RESOURCE",     EffectType::MODIFY_RESOURCE     },
		{ "SET_RESOURCE_MAX",    EffectType::SET_RESOURCE_MAX    },
		{ "EQUIP_ITEM",          EffectType::EQUIP_ITEM          },
		{ "UNEQUIP_ITEM",        EffectType::UNEQUIP_ITEM        },
		{ "SHUFFLE_ZONE",        EffectType::SHUFFLE_ZONE        },
		{ "LOOK_TOP_CARD",       EffectType::LOOK_TOP_CARD       },
		{ "LOOK_BOTTOM_CARD",    EffectType::LOOK_BOTTOM_CARD    },
		{ "SELECT_SPECIFIC_CARD",EffectType::SELECT_SPECIFIC_CARD},
		{ "DISCARD_RANDOM",      EffectType::DISCARD_RANDOM      },
		{ "PLACE_ON_TOP",        EffectType::PLACE_ON_TOP        },
		{ "PLACE_ON_BOTTOM",     EffectType::PLACE_ON_BOTTOM     },
		{ "ROLL_DICE",             EffectType::ROLL_DICE             },
		{ "EMIT_EVENT",           EffectType::EMIT_EVENT           },
		{ "MANUAL_EFFECT",        EffectType::MANUAL_EFFECT        },
		{ "CUSTOM",               EffectType::CUSTOM               },
		{ "SET_ACTOR_RESOURCE",   EffectType::SET_ACTOR_RESOURCE   },
		{ "TRIGGER_RULE",         EffectType::TRIGGER_RULE         },
		{ "SCALE_EFFECT",         EffectType::SCALE_EFFECT         },
		{ "CHAIN_EFFECT",         EffectType::CHAIN_EFFECT         },
		{ "DELAY_EFFECT",         EffectType::DELAY_EFFECT         },
	};
	auto it = TABLE.find(to_upper(raw));
	if (it == TABLE.end())
		throw ERuleBookError("RuleBookLoader: unknown effect type '" + raw + "'");
	return it->second;
}

TargetSelector parse_target_selector(const std::string& raw)
{
	static const std::unordered_map<std::string, TargetSelector> TABLE =
	{
		{ "SELF",                    TargetSelector::SELF                    },
		{ "SOURCE",                  TargetSelector::SOURCE                  },
		{ "SELECTED_ACTOR",          TargetSelector::SELECTED_ACTOR          },
		{ "SELECTED_ALLY",           TargetSelector::SELECTED_ALLY           },
		{ "SELECTED_ENEMY",          TargetSelector::SELECTED_ENEMY          },
		{ "ALL_ACTORS_IN_LOCATION",  TargetSelector::ALL_ACTORS_IN_LOCATION  },
		{ "ALL_ALLIES_IN_LOCATION",  TargetSelector::ALL_ALLIES_IN_LOCATION  },
		{ "ALL_ENEMIES_IN_LOCATION", TargetSelector::ALL_ENEMIES_IN_LOCATION },
		{ "ACTORS_WITH_STATUS",      TargetSelector::ACTORS_WITH_STATUS      },
		{ "LOCATION",                TargetSelector::LOCATION                },
		{ "SELECTED_CARD",           TargetSelector::SELECTED_CARD           },
		{ "SELECTED_ITEM",           TargetSelector::SELECTED_ITEM           },
		{ "MANUAL",                  TargetSelector::MANUAL                  },
	};
	auto it = TABLE.find(to_upper(raw));
	if (it == TABLE.end())
		throw ERuleBookError("RuleBookLoader: unknown target selector '" + raw + "'");
	return it->second;
}

// ── Object → struct converters ────────────────────────────────────────────────

TargetSpec build_target_spec(const std::string& selector_str)
{
	TargetSpec ts;
	ts.kind     = TargetKind::ACTOR;
	ts.selector = selector_str.empty()
	              ? TargetSelector::SELF
	              : parse_target_selector(selector_str);
	return ts;
}

EffectSpec build_effect_spec(const JObj& obj, const std::string& rule_id)
{
	auto ctx = "effect in rule '" + rule_id + "'";

	auto it_type = obj.find("type");
	if (it_type == obj.end())
		throw ERuleBookError("RuleBookLoader: missing 'type' in " + ctx);

	EffectSpec spec;
	spec.type        = parse_effect_type(it_type->second.as_str(ctx));
	spec.source_id   = rule_id;
	spec.amount      = (obj.count("amount") > 0) ? obj.at("amount").as_int(ctx) : 0;
	spec.value       = (obj.count("value")  > 0) ? obj.at("value").as_str(ctx)  : "";

	std::string target_str = (obj.count("target") > 0)
	                         ? obj.at("target").as_str(ctx)
	                         : "SELF";
	spec.target = build_target_spec(target_str);

	spec.chain_count     = (obj.count("chain_count") > 0)
	                       ? obj.at("chain_count").as_int(ctx) : 0;
	spec.optional        = false;
	spec.stop_on_failure = true;
	return spec;
}

RuleDefinition build_rule_definition(const JObj& obj)
{
	auto it_id = obj.find("rule_id");
	if (it_id == obj.end())
		throw ERuleBookError("RuleBookLoader: rule object missing 'rule_id'");

	RuleDefinition def;
	def.rule_id     = it_id->second.as_str("rule_id");
	def.description = (obj.count("description") > 0)
	                  ? obj.at("description").as_str("description")
	                  : "";

	// effects
	if (obj.count("effects") > 0)
	{
		const JArr& arr = obj.at("effects").as_arr("effects in rule '" + def.rule_id + "'");
		def.effects.reserve(arr.size());
		for (const JVal& ev : arr)
		{
			def.effects.push_back(
				build_effect_spec(ev.as_obj("effect entry in rule '" + def.rule_id + "'"),
				                  def.rule_id));
		}
	}

	// preconditions — stored as empty (optional field); advanced conditions
	// require ConditionSpec which is loaded separately if needed.
	// For the current rule format only simple effects without preconditions
	// are supported.  An explicit "preconditions" array is accepted but
	// silently ignored if the condition type is not yet wired.

	return def;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void RuleBookLoader::load_json(const std::string& path, RuleBook& book)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw ERuleBookError("RuleBookLoader: cannot open file '" + path + "'");
	}
	std::ostringstream buf;
	buf << file.rdbuf();
	load_json_string(buf.str(), book);
}

void RuleBookLoader::load_json_string(const std::string& json_text, RuleBook& book)
{
	JVal root;
	try
	{
		root = parse_json(json_text);
	}
	catch (const ERuleBookError&)
	{
		throw; // already descriptive
	}
	catch (const std::exception& ex)
	{
		throw ERuleBookError(
			std::string("RuleBookLoader: JSON parse error: ") + ex.what());
	}

	const JObj& top = root.as_obj("root");

	auto it_rules = top.find("rules");
	if (it_rules == top.end())
	{
		throw ERuleBookError("RuleBookLoader: root JSON object missing 'rules' array");
	}

	const JArr& rules_arr = it_rules->second.as_arr("rules");
	for (const JVal& rv : rules_arr)
	{
		RuleDefinition def = build_rule_definition(
			rv.as_obj("rule array entry"));
		book.register_rule(def);
	}
}

} // namespace gmRules
