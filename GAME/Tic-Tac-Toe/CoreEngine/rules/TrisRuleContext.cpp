/**
 * @file rules/TrisRuleContext.cpp
 * @brief Implementation of the Tic-Tac-Toe gmRules::RuleContext adapter.
 */

#include "TrisRuleContext.hpp"

#include "gmRules/core/RuleError.hpp"
#include "gmRules/core/RuleResult.hpp"

#include <string>

namespace gmTris
{

namespace
{

/// @brief Parses a "1".."9" location id into a 1-based cell index, or 0.
int parse_cell(const std::string &location_id)
{
	if (location_id.size() != 1 || location_id[0] < '1' || location_id[0] > '9')
	{
		return 0;
	}
	return location_id[0] - '0';
}

/// @brief Returns a rejecting result for unsupported context operations.
gmRules::RuleResult unsupported()
{
	return gmRules::RuleResult::fail(gmRules::RuleError::CONTEXT_ERROR,
	                                 "operation not supported by TrisRuleContext");
}

} // namespace

TrisRuleContext::TrisRuleContext(const Board &board) : _board(board)
{
}

std::string TrisRuleContext::cell_id(uint8_t row, uint8_t col)
{
	return std::to_string((row - 1) * Board::SIZE + col);
}

std::string TrisRuleContext::mark_tag(Mark mark)
{
	if (mark == Mark::X)
	{
		return "mark_X";
	}
	if (mark == Mark::O)
	{
		return "mark_O";
	}
	return std::string();
}

bool TrisRuleContext::has_location(const gmRules::LocationId &location_id) const
{
	return parse_cell(location_id) != 0;
}

bool TrisRuleContext::location_has_tag(const gmRules::LocationId &location_id,
                                       const std::string &tag) const
{
	const int cell = parse_cell(location_id);
	if (cell == 0)
	{
		return false;
	}
	const uint8_t row  = static_cast<uint8_t>((cell - 1) / Board::SIZE + 1);
	const uint8_t col  = static_cast<uint8_t>((cell - 1) % Board::SIZE + 1);
	const Mark    mark = _board.at(row, col);
	return mark != Mark::EMPTY && mark_tag(mark) == tag;
}

// ── Actor queries (unused) ────────────────────────────────────────────────────

bool TrisRuleContext::has_actor(const gmRules::ActorId &) const
{
	return false;
}

bool TrisRuleContext::actor_has_tag(const gmRules::ActorId &, const std::string &) const
{
	return false;
}

int TrisRuleContext::actor_current_hp(const gmRules::ActorId &) const
{
	return 0;
}

int TrisRuleContext::actor_max_hp(const gmRules::ActorId &) const
{
	return 0;
}

bool TrisRuleContext::actor_has_status(const gmRules::ActorId &,
                                       const gmRules::StatusId &) const
{
	return false;
}

std::vector<gmRules::StatusInstanceId>
TrisRuleContext::statuses_on_actor(const gmRules::ActorId &) const
{
	return {};
}

bool TrisRuleContext::are_allies(const gmRules::ActorId &, const gmRules::ActorId &) const
{
	return false;
}

bool TrisRuleContext::are_enemies(const gmRules::ActorId &, const gmRules::ActorId &) const
{
	return false;
}

int TrisRuleContext::actor_resource(const gmRules::ActorId &, const std::string &) const
{
	return 0;
}

// ── Actor mutation (unused) ───────────────────────────────────────────────────

void TrisRuleContext::modify_actor_hp(const gmRules::ActorId &, int)
{
}

void TrisRuleContext::add_actor_tag(const gmRules::ActorId &, const std::string &)
{
}

void TrisRuleContext::remove_actor_tag(const gmRules::ActorId &, const std::string &)
{
}

void TrisRuleContext::spawn_actor(const gmRules::ActorId &, const std::string &)
{
}

void TrisRuleContext::despawn_actor(const gmRules::ActorId &)
{
}

void TrisRuleContext::revive_actor(const gmRules::ActorId &)
{
}

void TrisRuleContext::change_actor_team(const gmRules::ActorId &, const std::string &)
{
}

void TrisRuleContext::modify_resource(const gmRules::ActorId &, const std::string &, int)
{
}

void TrisRuleContext::set_resource_max(const gmRules::ActorId &, const std::string &, int)
{
}

void TrisRuleContext::equip_item(const gmRules::ActorId &, const std::string &)
{
}

void TrisRuleContext::unequip_item(const gmRules::ActorId &, const std::string &)
{
}

// ── Status mutation (unused) ──────────────────────────────────────────────────

void TrisRuleContext::add_status_instance(const gmRules::StatusInstance &)
{
}

void TrisRuleContext::remove_status_instance(const gmRules::StatusInstanceId &)
{
}

// ── Location queries / mutation (unused) ──────────────────────────────────────

gmRules::LocationId TrisRuleContext::actor_location(const gmRules::ActorId &) const
{
	return std::string();
}

bool TrisRuleContext::are_locations_adjacent(const gmRules::LocationId &,
                                             const gmRules::LocationId &) const
{
	return false;
}

int TrisRuleContext::distance_between_locations(const gmRules::LocationId &,
                                                const gmRules::LocationId &) const
{
	return -1;
}

std::vector<gmRules::ActorId>
TrisRuleContext::actors_in_location(const gmRules::LocationId &) const
{
	return {};
}

bool TrisRuleContext::is_location_reachable(const gmRules::LocationId &,
                                            const gmRules::LocationId &) const
{
	return false;
}

bool TrisRuleContext::has_line_of_sight(const gmRules::LocationId &,
                                        const gmRules::LocationId &) const
{
	return false;
}

int TrisRuleContext::move_cost_between(const gmRules::LocationId &,
                                       const gmRules::LocationId &) const
{
	return -1;
}

void TrisRuleContext::move_actor_to_location(const gmRules::ActorId &,
                                             const gmRules::LocationId &)
{
}

void TrisRuleContext::set_location_passable(const gmRules::LocationId &, bool)
{
}

void TrisRuleContext::add_location_tag(const gmRules::LocationId &, const std::string &)
{
}

void TrisRuleContext::remove_location_tag(const gmRules::LocationId &, const std::string &)
{
}

void TrisRuleContext::set_location_owner(const gmRules::LocationId &, const std::string &)
{
}

void TrisRuleContext::create_barrier(const gmRules::LocationId &,
                                     const gmRules::LocationId &, const std::string &)
{
}

void TrisRuleContext::remove_barrier(const std::string &)
{
}

void TrisRuleContext::spawn_interactable(const gmRules::LocationId &, const std::string &)
{
}

void TrisRuleContext::despawn_interactable(const std::string &)
{
}

// ── Deck / card (unused) ──────────────────────────────────────────────────────

bool TrisRuleContext::has_deck(const gmRules::DeckId &) const
{
	return false;
}

std::vector<gmRules::CardId> TrisRuleContext::draw_cards(const gmRules::DeckId &, int)
{
	return {};
}

gmRules::RuleResult TrisRuleContext::move_card_to_zone(const gmRules::DeckId &,
                                                       const gmRules::CardId &,
                                                       const std::string &)
{
	return unsupported();
}

int TrisRuleContext::deck_zone_count(const gmRules::DeckId &, const std::string &) const
{
	return 0;
}

bool TrisRuleContext::card_in_zone(const gmRules::DeckId &, const gmRules::CardId &,
                                   const std::string &) const
{
	return false;
}

void TrisRuleContext::shuffle_zone(const gmRules::DeckId &, const std::string &)
{
}

std::vector<gmRules::CardId> TrisRuleContext::look_top_cards(const gmRules::DeckId &,
                                                             int) const
{
	return {};
}

std::vector<gmRules::CardId> TrisRuleContext::look_bottom_cards(const gmRules::DeckId &,
                                                                int) const
{
	return {};
}

gmRules::RuleResult TrisRuleContext::select_specific_card(const gmRules::DeckId &,
                                                          const gmRules::CardId &)
{
	return unsupported();
}

gmRules::RuleResult TrisRuleContext::discard_random_cards(const gmRules::DeckId &,
                                                          const std::string &, int)
{
	return unsupported();
}

gmRules::RuleResult TrisRuleContext::place_card_on_top(const gmRules::DeckId &,
                                                       const gmRules::CardId &)
{
	return unsupported();
}

gmRules::RuleResult TrisRuleContext::place_card_on_bottom(const gmRules::DeckId &,
                                                          const gmRules::CardId &)
{
	return unsupported();
}

int TrisRuleContext::roll_dice(const std::string &)
{
	return 0;
}

// ── Events / extended effects (unused) ────────────────────────────────────────

void TrisRuleContext::emit_event(const gmRules::RuleEvent &, const std::string &)
{
}

gmRules::RuleResult TrisRuleContext::apply_extended_effect(const gmRules::EffectSpec &,
                                                           const gmRules::TargetRef &,
                                                           const gmRules::ActorId &,
                                                           gmRules::RuleEvent *)
{
	return unsupported();
}

} // namespace gmTris
