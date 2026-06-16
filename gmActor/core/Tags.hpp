#ifndef GMACTOR_CORE_TAGS_HPP
#define GMACTOR_CORE_TAGS_HPP

/**
 * @file core/Tags.hpp
 * @brief Lightweight tag helpers used by actors, items, statuses, and modifiers.
 *
 * Tags are plain strings that allow the game engine to attach arbitrary
 * classification labels to any entity without extending the core data model.
 *
 * Examples: `"undead"`, `"flying"`, `"stunned"`, `"fire_immune"`.
 */

#include "gmActor/core/Ids.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Returns true if `tags` contains the given tag.
 *
 * @param tags  The tag list to search.
 * @param tag   The tag to look for.
 */
inline bool has_tag(const std::vector<Tag>& tags, const Tag& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

/**
 * @brief Adds `tag` to `tags` if it is not already present.
 *
 * @param tags  The tag list to modify.
 * @param tag   The tag to add.
 */
inline void add_tag(std::vector<Tag>& tags, const Tag& tag) {
    if (!has_tag(tags, tag)) {
        tags.push_back(tag);
    }
}

/**
 * @brief Removes `tag` from `tags`.  No-op if not present.
 *
 * @param tags  The tag list to modify.
 * @param tag   The tag to remove.
 */
inline void remove_tag(std::vector<Tag>& tags, const Tag& tag) {
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

} // namespace gmActor

#endif // GMACTOR_CORE_TAGS_HPP
