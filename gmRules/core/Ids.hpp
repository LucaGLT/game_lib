#ifndef GMRULES_CORE_IDS_HPP
#define GMRULES_CORE_IDS_HPP

/**
 * @file core/Ids.hpp
 * @brief String-based ID type aliases used throughout gmRules.
 *
 * All identifiers are `std::string` aliases for serializability and
 * debug readability.  They are trivially compatible with the corresponding
 * IDs in `gmActor`, `gmMap`, `gmDeck`, and `gmFlow`.
 */

#include <string>

namespace gmRules {

using RuleId           = std::string; ///< Rule definition identifier
using ActorId          = std::string; ///< Matches gmActor::ActorId
using LocationId       = std::string; ///< Matches gmMap location identifier
using CardId           = std::string; ///< Matches gmDeck::CardId
using DeckId           = std::string; ///< Matches gmDeck instance identifier
using ItemId           = std::string; ///< Item definition identifier
using StatusId         = std::string; ///< Status definition identifier
using StatusInstanceId = std::string; ///< Runtime status instance identifier
using EffectId         = std::string; ///< Effect definition identifier
using ConditionId      = std::string; ///< Condition definition identifier
using EventType        = std::string; ///< Rule event type tag
using SourceId         = std::string; ///< Effect / modifier source identifier

} // namespace gmRules

#endif // GMRULES_CORE_IDS_HPP
