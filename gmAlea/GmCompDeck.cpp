/**
 * @file GmCompDeck.cpp
 * @brief Implementation of the GmCompDeck multi-zone orchestrator.
 */

#include "GmCompDeck.hpp"

namespace gmAlea
{

// ─────────────────────────────────────────────────────────────────────────────
// Static sentinel
// ─────────────────────────────────────────────────────────────────────────────

const std::string GmCompDeck::_EMPTY_RULE_GROUP_ID{};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

GmCompDeck::GmCompDeck(std::string                  owner_name,
					   const std::vector<uint32_t>& deck_tokens,
					   std::optional<unsigned int>  seed)
	: _owner_name(std::move(owner_name))
	, _main_deck("MAIN_DECK", deck_tokens, seed)  // shuffled if MainDeckPolicy::can_shuffle
	, _hand("HAND")
	, _play_area("PLAY_AREA")
	, _memory("MEMORY")
	, _discard("DISCARD")
	, _banish_zone("BANISHED")
{}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-zone moves
// ─────────────────────────────────────────────────────────────────────────────

void GmCompDeck::draw_to_hand(int count)
{
	if (count <= 0)
	{
		throw EAleaInvalidDrawCountError(
			"draw_to_hand: count must be > 0, got " + std::to_string(count));
	}
	for (int i = 0; i < count; ++i)
	{
		uint32_t token_id = _main_deck.draw();   // throws EAleaDeckEmptyError if empty
		_hand.add(token_id);
		_fire_zone_change(token_id, ZoneId::MAIN_DECK, ZoneId::HAND);
	}
}

void GmCompDeck::draw_specific_to_hand(uint32_t token_id)
{
	uint32_t drawn = _main_deck.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_hand.add(drawn);
	_fire_zone_change(token_id, ZoneId::MAIN_DECK, ZoneId::HAND);
}

void GmCompDeck::play_card(uint32_t token_id)
{
	uint32_t card = _hand.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_play_area.add(card);
	_fire_zone_change(token_id, ZoneId::HAND, ZoneId::PLAY_AREA);
}

void GmCompDeck::resolve_card(uint32_t token_id)
{
	uint32_t card = _play_area.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_discard.add(card);
	_fire_zone_change(token_id, ZoneId::PLAY_AREA, ZoneId::DISCARD);
}

void GmCompDeck::discard_from_hand(uint32_t token_id)
{
	uint32_t card = _hand.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_discard.add(card);
	_fire_zone_change(token_id, ZoneId::HAND, ZoneId::DISCARD);
}

void GmCompDeck::discard_from_table(uint32_t token_id)
{
	uint32_t card = _play_area.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_discard.add(card);
	_fire_zone_change(token_id, ZoneId::PLAY_AREA, ZoneId::DISCARD);
}

void GmCompDeck::take_from_discard(uint32_t token_id)
{
	uint32_t card = _discard.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_hand.add(card);
	_fire_zone_change(token_id, ZoneId::DISCARD, ZoneId::HAND);
}

void GmCompDeck::return_from_discard_to_deck(uint32_t token_id)
{
	uint32_t card = _discard.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_main_deck.add(card);  // inserted at back (bottom of deck)
	_fire_zone_change(token_id, ZoneId::DISCARD, ZoneId::MAIN_DECK);
}

void GmCompDeck::banish(uint32_t token_id)
{
	ZoneId loc = locate(token_id);

	if (loc == ZoneId::NOT_FOUND)
	{
		throw EAleaTokenNotFoundError(
			"banish: token " + std::to_string(token_id) +
			" is not tracked by owner '" + _owner_name + "'");
	}
	if (loc == ZoneId::BANISHED)
	{
		throw EAleaTokenNotFoundError(
			"banish: token " + std::to_string(token_id) +
			" is already banished — cannot banish twice");
	}

	_remove_from_zone(loc, token_id);
	_banish_zone.add(token_id);
	_fire_zone_change(token_id, loc, ZoneId::BANISHED);
}

void GmCompDeck::reshuffle_discard_into_deck()
{
	// Take a snapshot of the discard pile (copy — not a live reference)
	std::vector<uint32_t> all = _discard.peek_all();

	for (uint32_t token_id : all)
	{
		_discard.take_specific(token_id);
		_main_deck.add(token_id);
		_fire_zone_change(token_id, ZoneId::DISCARD, ZoneId::MAIN_DECK);
	}

	// Reshuffle the combined main deck
	_main_deck.shuffle();
}

// ───────────────────────────────────────────────────────────────────────────
// Memory zone moves
// ───────────────────────────────────────────────────────────────────────────

void GmCompDeck::remember_from_hand(uint32_t token_id)
{
	uint32_t card = _hand.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_memory.add(card);
	_fire_zone_change(token_id, ZoneId::HAND, ZoneId::MEMORY);
}

void GmCompDeck::remember_from_play_area(uint32_t token_id)
{
	uint32_t card = _play_area.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_memory.add(card);
	_fire_zone_change(token_id, ZoneId::PLAY_AREA, ZoneId::MEMORY);
}

void GmCompDeck::remember_from_discard(uint32_t token_id)
{
	uint32_t card = _discard.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_memory.add(card);
	_fire_zone_change(token_id, ZoneId::DISCARD, ZoneId::MEMORY);
}

void GmCompDeck::play_from_memory(uint32_t token_id)
{
	uint32_t card = _memory.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_play_area.add(card);
	_fire_zone_change(token_id, ZoneId::MEMORY, ZoneId::PLAY_AREA);
}

void GmCompDeck::return_memory_to_hand(uint32_t token_id)
{
	uint32_t card = _memory.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_hand.add(card);
	_fire_zone_change(token_id, ZoneId::MEMORY, ZoneId::HAND);
}

void GmCompDeck::discard_from_memory(uint32_t token_id)
{
	uint32_t card = _memory.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_discard.add(card);
	_fire_zone_change(token_id, ZoneId::MEMORY, ZoneId::DISCARD);
}

void GmCompDeck::banish_from_memory(uint32_t token_id)
{
	uint32_t card = _memory.take_specific(token_id);  // throws EAleaTokenNotFoundError
	_banish_zone.add(card);
	_fire_zone_change(token_id, ZoneId::MEMORY, ZoneId::BANISHED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

ZoneId GmCompDeck::locate(uint32_t token_id) const
{
	if (_main_deck.contains(token_id))   return ZoneId::MAIN_DECK;
	if (_hand.contains(token_id))        return ZoneId::HAND;
	if (_play_area.contains(token_id))   return ZoneId::PLAY_AREA;
	if (_memory.contains(token_id))      return ZoneId::MEMORY;
	if (_discard.contains(token_id))     return ZoneId::DISCARD;
	if (_banish_zone.contains(token_id)) return ZoneId::BANISHED;
	return ZoneId::NOT_FOUND;
}

int GmCompDeck::count_in(ZoneId zone) const
{
	switch (zone)
	{
		case ZoneId::MAIN_DECK:  return _main_deck.count();
		case ZoneId::HAND:       return _hand.count();
		case ZoneId::PLAY_AREA:  return _play_area.count();
		case ZoneId::MEMORY:     return _memory.count();
		case ZoneId::DISCARD:    return _discard.count();
		case ZoneId::BANISHED:   return _banish_zone.count();
		case ZoneId::NOT_FOUND:  return 0;
	}
	return 0;  // unreachable — suppresses compiler warnings
}

int GmCompDeck::total_count() const
{
	return _main_deck.count()
		 + _hand.count()
		 + _play_area.count()
		 + _memory.count()
		 + _discard.count()
		 + _banish_zone.count();
}

const std::string& GmCompDeck::owner_name() const
{
	return _owner_name;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void GmCompDeck::_remove_from_zone(ZoneId zone, uint32_t token_id)
{
	switch (zone)
	{
		case ZoneId::MAIN_DECK:  _main_deck.take_specific(token_id);  return;
		case ZoneId::HAND:       _hand.take_specific(token_id);       return;
		case ZoneId::PLAY_AREA:  _play_area.take_specific(token_id);  return;
		case ZoneId::MEMORY:     _memory.take_specific(token_id);     return;
		case ZoneId::DISCARD:    _discard.take_specific(token_id);    return;
		case ZoneId::BANISHED:
		case ZoneId::NOT_FOUND:
			throw EAleaTokenNotFoundError(
				"_remove_from_zone: cannot remove token " +
				std::to_string(token_id) +
				" from zone '" + zone_name(zone) + "'");
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule-group binding
// ─────────────────────────────────────────────────────────────────────────────

void GmCompDeck::register_rule_group(uint32_t token_id, const std::string& rule_group_id)
{
	if (rule_group_id.empty())
	{
		_rule_group_map.erase(token_id);
	}
	else
	{
		_rule_group_map[token_id] = rule_group_id;
	}
}

const std::string& GmCompDeck::rule_group_of(uint32_t token_id) const
{
	auto it = _rule_group_map.find(token_id);
	if (it == _rule_group_map.end())
	{
		return _EMPTY_RULE_GROUP_ID;
	}
	return it->second;
}

void GmCompDeck::set_zone_change_callback(ZoneChangeCallback cb)
{
	_zone_change_cb = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void GmCompDeck::_fire_zone_change(uint32_t token_id, ZoneId from, ZoneId to) const
{
	if (!_zone_change_cb)
	{
		return;
	}
	const std::string& rg = rule_group_of(token_id);
	_zone_change_cb(token_id, rg, from, to);
}

} // namespace gmAlea
