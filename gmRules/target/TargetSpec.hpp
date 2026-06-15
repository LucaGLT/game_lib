#ifndef GMRULES_TARGET_TARGETSPEC_HPP
#define GMRULES_TARGET_TARGETSPEC_HPP

/**
 * @file target/TargetSpec.hpp
 * @brief Describes what a rule targets and how to select it.
 */

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Classifies the domain of a target reference.
 */
enum class TargetKind
{
    ACTOR,        ///< A single actor
    ACTOR_GROUP,  ///< Multiple actors selected as a group
    LOCATION,     ///< A map location/area
    CARD,         ///< A card
    DECK,         ///< A deck instance
    ITEM,         ///< An item instance
    INTERACTABLE, ///< A game-specific interactable object
    NONE          ///< No target required
};

/**
 * @brief Describes which actor(s) / location(s) to target.
 */
enum class TargetSelector
{
    SELF,                    ///< The actor that triggered the effect
    SOURCE,                  ///< The declared source of the effect
    SELECTED_ACTOR,          ///< A specific actor chosen externally
    SELECTED_ALLY,           ///< A specific ally chosen externally
    SELECTED_ENEMY,          ///< A specific enemy chosen externally
    ALL_ACTORS_IN_LOCATION,  ///< Every actor in the source's location
    ALL_ALLIES_IN_LOCATION,  ///< Every ally in the source's location
    ALL_ENEMIES_IN_LOCATION, ///< Every enemy in the source's location
    ACTORS_WITH_STATUS,      ///< Actors in location that carry a specific status
    LOCATION,                ///< A specific location chosen externally
    SELECTED_CARD,           ///< A specific card chosen externally
    SELECTED_ITEM,           ///< A specific item chosen externally
    MANUAL                   ///< Targets are provided directly by the caller
};

/**
 * @brief Describes the spatial range constraint for target selection.
 */
enum class RangeType
{
    NONE,                  ///< No range constraint
    SAME_LOCATION,         ///< Target must be in the same location as the source
    ADJACENT_LOCATION,     ///< Target must be in an adjacent location
    WITHIN_N_LOCATIONS,    ///< Target within `range_value` steps
    ANY_VISIBLE_LOCATION,  ///< Any location the source can see (game-specific)
    GLOBAL                 ///< No location restriction
};

/**
 * @brief Full specification of a target selection requirement.
 */
struct TargetSpec
{
    TargetKind     kind       = TargetKind::NONE;         ///< Target domain
    TargetSelector selector   = TargetSelector::MANUAL;   ///< Selection strategy
    RangeType      range_type = RangeType::NONE;          ///< Range constraint
    int            range_value = 0;                       ///< Distance for WITHIN_N

    std::vector<std::string> required_tags; ///< Target must have all of these tags
    std::vector<std::string> forbidden_tags;///< Target must have none of these tags

    bool allow_self = true;  ///< Whether the source actor can be selected as target
    bool required   = true;  ///< False = target is optional (no targets = ok)
};

} // namespace gmRules

#endif // GMRULES_TARGET_TARGETSPEC_HPP
