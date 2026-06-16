#ifndef GMACTOR_ITEMS_ITEMDEFINITION_HPP
#define GMACTOR_ITEMS_ITEMDEFINITION_HPP

/**
 * @file items/ItemDefinition.hpp
 * @brief Immutable item template shared across all instances.
 *
 * Definitions are loaded from content data and must not be mutated at runtime.
 * All runtime state (charges, exhaustion, equipped flag) lives in `ItemState`.
 *
 * ## Notes
 * - `granted_cards` stores card IDs; actual card zones belong to gmDeck/gmCompDeck.
 * - `use_effect_refs` stores opaque string references; actual effect resolution
 *   belongs to the game-specific engine.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/core/Tags.hpp"
#include "gmActor/modifiers/Modifier.hpp"

#include <string>
#include <vector>

namespace gmActor {

/**
 * @brief Immutable description of an item archetype.
 */
struct ItemDefinition {
    ItemId      id;                             ///< Unique definition identifier
    std::string name;                           ///< Human-readable label
    ItemKind    kind = ItemKind::GENERIC;        ///< Item classification

    std::vector<Tag>               tags;        ///< Classification tags
    std::vector<CardId>            granted_cards;      ///< Card IDs granted when held/equipped
    std::vector<ModifierDefinition> passive_modifiers; ///< Always-on modifiers when equipped
    std::vector<std::string>       use_effect_refs;    ///< Opaque engine-side effect references

    bool consumable  = false;  ///< True if the item is consumed on use
    int  max_charges = 0;      ///< Maximum charge count (0 = no charge mechanic)
};

} // namespace gmActor

#endif // GMACTOR_ITEMS_ITEMDEFINITION_HPP
