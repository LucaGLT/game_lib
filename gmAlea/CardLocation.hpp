#ifndef GMALEA_CARDLOCATION_HPP
#define GMALEA_CARDLOCATION_HPP

/**
 * @file CardLocation.hpp
 * @brief Zone identifier enum and helper for the GmCompDeck system.
 */

#include <string>

namespace gmAlea
{

/**
 * @enum ZoneId
 * @brief Identifies one of the five canonical zones managed by @ref GmCompDeck.
 *
 * Each token tracked by a `GmCompDeck` instance resides in exactly one zone
 * at all times.  `NOT_FOUND` is a sentinel returned by @ref GmCompDeck::locate
 * when the token is not registered with this composite deck.
 */
enum class ZoneId
{
	MAIN_DECK,  ///< Primary shuffled draw deck
	HAND,       ///< Cards held by the owner (private hand)
	PLAY_AREA,  ///< Cards currently in play on the table
	MEMORY,     ///< Ordered list of retained cards (active/controlled zone)
	DISCARD,    ///< Ordered discard pile — insertion order preserved
	BANISHED,   ///< Permanently removed from the game (insert-only)
	NOT_FOUND   ///< Sentinel: token not tracked by this CompDeck
};

/**
 * @brief Returns a human-readable uppercase label for a ZoneId.
 *
 * @param zone The zone to name.
 * @return String label, e.g. `"MAIN_DECK"`, `"HAND"`, `"NOT_FOUND"`.
 */
inline std::string zone_name(ZoneId zone)
{
	switch (zone)
	{
		case ZoneId::MAIN_DECK:  return "MAIN_DECK";
		case ZoneId::HAND:       return "HAND";
		case ZoneId::PLAY_AREA:  return "PLAY_AREA";
		case ZoneId::MEMORY:     return "MEMORY";
		case ZoneId::DISCARD:    return "DISCARD";
		case ZoneId::BANISHED:   return "BANISHED";
		case ZoneId::NOT_FOUND:  return "NOT_FOUND";
	}
	return "UNKNOWN";
}

} // namespace gmAlea

#endif // GMALEA_CARDLOCATION_HPP
