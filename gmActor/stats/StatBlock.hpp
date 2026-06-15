#ifndef GMACTOR_STATS_STATBLOCK_HPP
#define GMACTOR_STATS_STATBLOCK_HPP

/**
 * @file stats/StatBlock.hpp
 * @brief Generic string-keyed stat map for arbitrary numeric actor statistics.
 *
 * `StatBlock` stores base stat values by string key.  The game engine defines
 * what keys are meaningful (e.g. `"base_damage"`, `"base_movement"`,
 * `"hand_limit"`, `"memory_limit"`).
 *
 * `StatBlock` holds **base** values only.  Compute effective values by
 * calling `apply_modifiers()` from `modifiers/Modifier.hpp`.
 */

#include "gmActor/core/Ids.hpp"

#include <string>
#include <unordered_map>

namespace gmActor {

/**
 * @brief String-keyed map of base numeric stat values.
 *
 * @par Example
 * @code
 *   StatBlock sb;
 *   sb.set("base_damage",   3.0);
 *   sb.set("base_movement", 2.0);
 *   double dmg = sb.get("base_damage", 1.0);  // default 1.0 if absent
 * @endcode
 */
class StatBlock {
public:
    /**
     * @brief Sets or overwrites a stat value.
     *
     * @param key   Stat key (e.g. `"base_damage"`).
     * @param value Base value.
     */
    void set(const std::string& key, double value);

    /**
     * @brief Returns the stored base value for `key`.
     *
     * @param key          Stat key.
     * @param default_val  Returned if the key is absent.
     * @return             Stored or default value.
     */
    double get(const std::string& key, double default_val = 0.0) const;

    /**
     * @brief Returns true if the stat block contains an entry for `key`.
     */
    bool has(const std::string& key) const;

    /**
     * @brief Removes the entry for `key`.  No-op if absent.
     */
    void remove(const std::string& key);

    /**
     * @brief Read-only access to the underlying map.
     */
    const std::unordered_map<std::string, double>& data() const;

private:
    std::unordered_map<std::string, double> data_;
};

} // namespace gmActor

#endif // GMACTOR_STATS_STATBLOCK_HPP
