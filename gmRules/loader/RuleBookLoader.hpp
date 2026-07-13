#ifndef GMRULES_LOADER_RULEBOOKLOADER_HPP
#define GMRULES_LOADER_RULEBOOKLOADER_HPP

/**
 * @file loader/RuleBookLoader.hpp
 * @brief Loads RuleDefinitions from JSON into a RuleBook.
 *
 * `RuleBookLoader` converts a JSON file into `RuleDefinition` objects and
 * registers them in a `RuleBook`.  It uses only the C++17 standard library
 * (no external JSON library).  The parser handles the subset of JSON needed
 * by the rule definition format: objects, arrays, strings, and integers.
 *
 * ## Expected JSON format
 * @code
 * {
 *   "rules": [
 *     {
 *       "rule_id":     "r_add_action_1",
 *       "description": "+1 Azione",
 *       "preconditions": [],
 *       "effects": [
 *         {
 *           "type":   "MODIFY_RESOURCE",
 *           "target": "SELF",
 *           "value":  "actions",
 *           "amount": 1
 *         }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 *
 * ## Supported `effect.type` values (case-insensitive)
 * All names from `EffectType` enum are accepted.  Unknown names produce
 * `ERuleBookError` with the name included in the message.
 *
 * ## Supported `effect.target` values (case-insensitive)
 * `SELF`, `SOURCE`, `SELECTED_ACTOR`, `ALL_ALLIES_IN_LOCATION`,
 * `ALL_ENEMIES_IN_LOCATION`, `MANUAL`.  Defaults to `SELF` if omitted.
 *
 * ## Error handling
 * All structural errors (missing key, wrong type, unknown enum) throw
 * `ERuleBookError` with a descriptive message that includes the rule_id
 * being parsed when available.
 */

#include "gmRules/core/RuleBook.hpp"

#include <string>

namespace gmRules {

/**
 * @class RuleBookLoader
 * @brief Static-only utility for loading rule definitions into a RuleBook.
 *
 * All methods are static; do not instantiate this class.
 */
class RuleBookLoader
{
public:
	RuleBookLoader()  = delete;
	~RuleBookLoader() = delete;

	/**
	 * @brief Loads rules from a JSON file and registers them in `book`.
	 *
	 * Existing rules in `book` are NOT cleared — this method only adds.
	 * Duplicate `rule_id` values throw `ERuleBookError` (via `RuleBook`).
	 *
	 * @param path  Path to the JSON file.
	 * @param book  RuleBook to populate.
	 * @throws ERuleBookError if the file cannot be opened, is malformed,
	 *         or contains unknown enum values.
	 */
	static void load_json(const std::string& path, RuleBook& book);

	/**
	 * @brief Loads rules from an in-memory JSON string and registers them.
	 *
	 * Useful for tests without touching the filesystem.
	 *
	 * @param json_text  JSON content as a string.
	 * @param book       RuleBook to populate.
	 * @throws ERuleBookError on parse or registration failure.
	 */
	static void load_json_string(const std::string& json_text, RuleBook& book);
};

} // namespace gmRules

#endif // GMRULES_LOADER_RULEBOOKLOADER_HPP
