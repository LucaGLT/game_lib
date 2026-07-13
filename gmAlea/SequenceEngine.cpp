/**
 * @file SequenceEngine.cpp
 * @brief Implementation of the SequenceEngine pure state machine.
 */

#include "gmAlea/SequenceEngine.hpp"

namespace gmAlea {

// ── can_play ──────────────────────────────────────────────────────────────────

bool SequenceEngine::can_play(CardType card, const SequenceState& state) const
{
	// An interrupted state blocks all plays until the engine is reset.
	if (state.interrupted)
		return false;

	switch (card)
	{
		case CardType::SINGLE:
			return !state.active;

		case CardType::SEQ_START:
			return !state.active;

		case CardType::SEQ_CONTINUE:
			return state.active;

		case CardType::SEQ_END:
			return state.active;

		case CardType::INSTANT:
			// INSTANTs are always valid — they are out-of-turn or reaction plays.
			return true;
	}
	return false;
}

// ── is_turn_ending ────────────────────────────────────────────────────────────

bool SequenceEngine::is_turn_ending(CardType card, const SequenceState& /*state*/) const
{
	// SINGLE and SEQ_END unconditionally end the turn.
	// SEQ_START and SEQ_CONTINUE leave the choice to the actor.
	// INSTANT is an out-of-turn play and never ends the active turn.
	return card == CardType::SINGLE || card == CardType::SEQ_END;
}

// ── advance ───────────────────────────────────────────────────────────────────

SequenceState SequenceEngine::advance(CardType card, const SequenceState& state) const
{
	SequenceState next  = state;
	next.interrupted    = false;

	switch (card)
	{
		case CardType::SINGLE:
			next.active       = false;
			next.last_type    = CardType::SINGLE;
			next.cards_played = state.cards_played + 1;
			break;

		case CardType::SEQ_START:
			next.active       = true;
			next.last_type    = CardType::SEQ_START;
			next.cards_played = state.cards_played + 1;
			break;

		case CardType::SEQ_CONTINUE:
			next.active       = true;
			next.last_type    = CardType::SEQ_CONTINUE;
			next.cards_played = state.cards_played + 1;
			break;

		case CardType::SEQ_END:
			next.active       = false;
			next.last_type    = CardType::SEQ_END;
			next.cards_played = state.cards_played + 1;
			break;

		case CardType::INSTANT:
			// Active and last_type are unchanged: INSTANT is a side-channel play.
			next.cards_played = state.cards_played + 1;
			break;
	}

	return next;
}

// ── interrupt ─────────────────────────────────────────────────────────────────

SequenceState SequenceEngine::interrupt(const SequenceState& state) const
{
	SequenceState next = state;
	next.active        = false;
	next.interrupted   = true;
	return next;
}

// ── reset ─────────────────────────────────────────────────────────────────────

SequenceState SequenceEngine::reset() const
{
	return SequenceState{};
}

} // namespace gmAlea
