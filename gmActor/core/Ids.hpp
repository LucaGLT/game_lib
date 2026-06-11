#ifndef GMACTOR_CORE_IDS_HPP
#define GMACTOR_CORE_IDS_HPP

/**
 * @file core/Ids.hpp
 * @brief String-based ID type aliases used throughout gmActor.
 *
 * All identifiers are `std::string` aliases so that they are trivially
 * compatible with `gmFlow::ActorId` (also `std::string`) and printable
 * in log messages without conversion.
 *
 * Do not replace these with integer types without updating all consumers.
 */

#include <string>

namespace gmActor {

using ActorId           = std::string; ///< Unique actor identifier
using FactionId         = std::string; ///< Faction / team group identifier
using AreaId            = std::string; ///< Location / area identifier (gmMap reference)
using ItemId            = std::string; ///< Canonical item definition identifier
using ItemInstanceId    = std::string; ///< Unique runtime item instance identifier
using CardId            = std::string; ///< Card identifier (gmDeck reference)
using DeckInstanceId    = std::string; ///< Deck instance identifier (gmDeck reference)
using StatusId          = std::string; ///< Status effect identifier
using ModifierId        = std::string; ///< Modifier definition/instance identifier
using TraitId           = std::string; ///< Passive trait identifier
using AffiliationId     = std::string; ///< Affiliation or sub-faction identifier
using MonsterTypeId     = std::string; ///< Monster type / archetype identifier
using MonsterGroupId    = std::string; ///< Monster group identifier
using MonsterInstanceId = std::string; ///< Individual monster body identifier
using ObjectiveId       = std::string; ///< Mission objective identifier
using SourceId          = std::string; ///< Effect / modifier source identifier
using Tag               = std::string; ///< Lightweight classification tag

} // namespace gmActor

#endif // GMACTOR_CORE_IDS_HPP
