#ifndef GMACTOR_ACTORS_HEROSTATE_HPP
#define GMACTOR_ACTORS_HEROSTATE_HPP

/**
 * @file actors/HeroState.hpp
 * @brief Mutable runtime state for a player-controlled or player-facing hero.
 *
 * Heroes are the primary player actors.  They carry personal decks, equipment,
 * inventory, and progression data.
 *
 * ## Deck references
 * Only string IDs are stored here.  Actual card zones (hand, discard, memory,
 * banish) belong to `gmDeck` / `gmCompDeck`.
 */

#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/items/EquipmentState.hpp"
#include "gmActor/items/InventoryState.hpp"
#include "gmActor/core/Ids.hpp"

#include <vector>

namespace gmActor {

/**
 * @brief Full mutable state for a hero actor.
 *
 * @note Deck zones (hand, discard, play area, memory, banish) are NOT stored
 *       here — use `gmCompDeck` instances keyed by `total_deck_id` /
 *       `mission_deck_id`.
 */
struct HeroState {
    ActorStateCommon common; ///< Shared actor state (common.kind == ActorKind::HERO)

    // ── Progression ───────────────────────────────────────────────────────────
    int level = 1; ///< Hero level / tier

    // ── Deck / hand limits ────────────────────────────────────────────────────
    int hand_limit         = 0; ///< Maximum cards held in hand
    int memory_limit       = 0; ///< Maximum cards held in memory
    int mission_deck_limit = 0; ///< Maximum cards in the mission deck

    // ── Deck instance references ──────────────────────────────────────────────
    DeckInstanceId total_deck_id;   ///< gmDeck instance for the full personal deck
    DeckInstanceId mission_deck_id; ///< gmDeck instance for the mission sub-deck

    // ── Equipment and inventory ───────────────────────────────────────────────
    EquipmentState equipment; ///< Equipped items by slot
    InventoryState inventory; ///< All carried item instance IDs

    // ── Social / group membership ─────────────────────────────────────────────
    std::vector<AffiliationId> affiliations; ///< Guild, party, or sub-faction memberships

    // ── Mission state ─────────────────────────────────────────────────────────
    bool is_ko = false; ///< Hero-specific KO flag (mirrored from common.life_state)
    std::vector<ItemInstanceId> carried_mission_items; ///< Mission items currently held
};

} // namespace gmActor

#endif // GMACTOR_ACTORS_HEROSTATE_HPP
