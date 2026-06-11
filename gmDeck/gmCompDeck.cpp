/**
 * @file gmCompDeck.cpp
 * @brief Implementation of the gmCompDeck multi-zone orchestrator.
 */

#include "gmCompDeck.hpp"

#include <sstream>

namespace gmFate {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

gmCompDeck::gmCompDeck(std::string                  owner_name,
                       const std::vector<uint32_t>& deck_tokens,
                       std::optional<unsigned int>  seed)
    : owner_name_(std::move(owner_name))
    , main_deck_("MAIN_DECK", deck_tokens, seed)  // shuffled if MainDeckPolicy::can_shuffle
    , hand_("HAND")
    , play_area_("PLAY_AREA")
    , discard_("DISCARD")
    , banish_zone_("BANISHED")
{}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-zone moves
// ─────────────────────────────────────────────────────────────────────────────

void gmCompDeck::draw_to_hand(int count) {
    if (count <= 0) {
        throw InvalidDrawCountError(
            "draw_to_hand: count must be > 0, got " + std::to_string(count));
    }
    for (int i = 0; i < count; ++i) {
        uint32_t token_id = main_deck_.draw();   // throws DeckEmptyError if empty
        hand_.add(token_id);
    }
}

void gmCompDeck::draw_specific_to_hand(uint32_t token_id) {
    uint32_t drawn = main_deck_.take_specific(token_id);  // throws TokenNotFoundError
    hand_.add(drawn);
}

void gmCompDeck::play_card(uint32_t token_id) {
    uint32_t card = hand_.take_specific(token_id);  // throws TokenNotFoundError
    play_area_.add(card);
}

void gmCompDeck::resolve_card(uint32_t token_id) {
    uint32_t card = play_area_.take_specific(token_id);  // throws TokenNotFoundError
    discard_.add(card);
}

void gmCompDeck::discard_from_hand(uint32_t token_id) {
    uint32_t card = hand_.take_specific(token_id);  // throws TokenNotFoundError
    discard_.add(card);
}

void gmCompDeck::discard_from_table(uint32_t token_id) {
    uint32_t card = play_area_.take_specific(token_id);  // throws TokenNotFoundError
    discard_.add(card);
}

void gmCompDeck::take_from_discard(uint32_t token_id) {
    uint32_t card = discard_.take_specific(token_id);  // throws TokenNotFoundError
    hand_.add(card);
}

void gmCompDeck::return_from_discard_to_deck(uint32_t token_id) {
    uint32_t card = discard_.take_specific(token_id);  // throws TokenNotFoundError
    main_deck_.add(card);  // inserted at back (bottom of deck)
}

void gmCompDeck::banish(uint32_t token_id) {
    ZoneId loc = locate(token_id);

    if (loc == ZoneId::NOT_FOUND) {
        throw TokenNotFoundError(
            "banish: token " + std::to_string(token_id) +
            " is not tracked by owner '" + owner_name_ + "'");
    }
    if (loc == ZoneId::BANISHED) {
        throw TokenNotFoundError(
            "banish: token " + std::to_string(token_id) +
            " is already banished — cannot banish twice");
    }

    _remove_from_zone(loc, token_id);
    banish_zone_.add(token_id);
}

void gmCompDeck::reshuffle_discard_into_deck() {
    // Take a snapshot of the discard pile (copy — not a live reference)
    std::vector<uint32_t> all = discard_.peek_all();

    for (uint32_t token_id : all) {
        discard_.take_specific(token_id);
        main_deck_.add(token_id);
    }

    // Reshuffle the combined main deck
    main_deck_.shuffle();
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

ZoneId gmCompDeck::locate(uint32_t token_id) const {
    if (main_deck_.contains(token_id))   return ZoneId::MAIN_DECK;
    if (hand_.contains(token_id))        return ZoneId::HAND;
    if (play_area_.contains(token_id))   return ZoneId::PLAY_AREA;
    if (discard_.contains(token_id))     return ZoneId::DISCARD;
    if (banish_zone_.contains(token_id)) return ZoneId::BANISHED;
    return ZoneId::NOT_FOUND;
}

int gmCompDeck::count_in(ZoneId zone) const {
    switch (zone) {
        case ZoneId::MAIN_DECK:  return main_deck_.count();
        case ZoneId::HAND:       return hand_.count();
        case ZoneId::PLAY_AREA:  return play_area_.count();
        case ZoneId::DISCARD:    return discard_.count();
        case ZoneId::BANISHED:   return banish_zone_.count();
        case ZoneId::NOT_FOUND:  return 0;
    }
    return 0;  // unreachable — suppresses compiler warnings
}

int gmCompDeck::total_count() const {
    return main_deck_.count()
         + hand_.count()
         + play_area_.count()
         + discard_.count()
         + banish_zone_.count();
}

const std::string& gmCompDeck::owner_name() const {
    return owner_name_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void gmCompDeck::_remove_from_zone(ZoneId zone, uint32_t token_id) {
    switch (zone) {
        case ZoneId::MAIN_DECK:  main_deck_.take_specific(token_id);  return;
        case ZoneId::HAND:       hand_.take_specific(token_id);       return;
        case ZoneId::PLAY_AREA:  play_area_.take_specific(token_id);  return;
        case ZoneId::DISCARD:    discard_.take_specific(token_id);    return;
        case ZoneId::BANISHED:
        case ZoneId::NOT_FOUND:
            throw TokenNotFoundError(
                "_remove_from_zone: cannot remove token " +
                std::to_string(token_id) +
                " from zone '" + zone_name(zone) + "'");
    }
}

} // namespace gmFate
