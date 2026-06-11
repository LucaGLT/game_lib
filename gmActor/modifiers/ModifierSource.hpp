#ifndef GMACTOR_MODIFIERS_MODIFIERSOURCE_HPP
#define GMACTOR_MODIFIERS_MODIFIERSOURCE_HPP

/**
 * @file modifiers/ModifierSource.hpp
 * @brief Describes the origin of a modifier or status effect.
 *
 * `ModifierSource` is a lightweight value object.  It records what entity
 * or effect applied a modifier so that the game engine can remove modifiers
 * from a specific source (e.g. when an item is unequipped or a status ends).
 */

#include "gmActor/core/Ids.hpp"

#include <string>

namespace gmActor {

/**
 * @brief Identifies the origin of a modifier or status application.
 *
 * @par Usage
 * @code
 *   ModifierSource src;
 *   src.source_id    = "sword_of_flames";
 *   src.source_kind  = "item";
 * @endcode
 */
struct ModifierSource {
    SourceId    source_id;           ///< Unique ID of the originating entity
    std::string source_kind;         ///< Free-form kind label (e.g. "item", "status", "ability", "terrain")
    std::string description;         ///< Optional human-readable description
};

} // namespace gmActor

#endif // GMACTOR_MODIFIERS_MODIFIERSOURCE_HPP
