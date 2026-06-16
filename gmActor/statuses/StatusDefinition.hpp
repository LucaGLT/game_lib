#ifndef GMACTOR_STATUSES_STATUSDEFINITION_HPP
#define GMACTOR_STATUSES_STATUSDEFINITION_HPP

/**
 * @file statuses/StatusDefinition.hpp
 * @brief Immutable template describing a status effect.
 *
 * Definitions are shared, read-only data (e.g. loaded from a content table).
 * At runtime, applying a status to an actor creates a `StatusInstance`.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Tags.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Immutable description of a named status effect.
 *
 * @par Example
 * @code
 *   StatusDefinition poisoned;
 *   poisoned.id          = "poisoned";
 *   poisoned.name        = "Poisoned";
 *   poisoned.stackable   = true;
 * @endcode
 */
struct StatusDefinition {
    StatusId    id;                  ///< Unique identifier
    std::string name;                ///< Human-readable label
    std::string description;         ///< Flavour / rules text
    std::vector<Tag> tags;           ///< Classification tags (e.g. "debuff", "poison")
    bool stackable = false;          ///< Whether multiple stacks can accumulate
};

} // namespace gmActor

#endif // GMACTOR_STATUSES_STATUSDEFINITION_HPP
