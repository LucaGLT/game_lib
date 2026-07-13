#ifndef ELDHOM_SEQUENCE_ELDHOMSEQUENCEADAPTER_HPP
#define ELDHOM_SEQUENCE_ELDHOMSEQUENCEADAPTER_HPP

/**
 * @file sequence/EldhomSequenceAdapter.hpp
 * @brief Thin adapter that connects gmAlea::SequenceEngine to the Eldhom
 *        per-hero SequenceState map.
 *
 * `EldhomSequenceAdapter` is a stateless helper.  It receives the shared
 * `gmAlea::SequenceEngine` and delegates all sequence-validation calls to it.
 * Its only added responsibility is to translate between Eldhom's `CardId`
 * aliases and `gmAlea::CardType` values read from the card catalog.
 *
 * ### Design rationale
 *
 * The adapter keeps `EldhomEngine.cpp` readable by grouping all sequence
 * logic into a single call site.  It does not own state; the caller owns the
 * `SequenceState` map.
 */

#include "gmAlea/SequenceEngine.hpp"
#include "gmAlea/SequenceState.hpp"
#include "gmAlea/CardType.hpp"

#include <unordered_map>
#include <string>

namespace eldhom {

/**
 * @class EldhomSequenceAdapter
 * @brief Stateless wrapper around `gmAlea::SequenceEngine` for Eldhom.
 */
class EldhomSequenceAdapter
{
public:
	// ── Sequence gate ─────────────────────────────────────────────────────────

	/**
	 * @brief Returns true if the card type may be played given `state`.
	 *
	 * Forwards directly to `gmAlea::SequenceEngine::can_play`.
	 *
	 * @param ct    Card type of the card the hero wants to play.
	 * @param state Current sequence state for this hero.
	 */
	bool can_play(gmAlea::CardType ct, const gmAlea::SequenceState& state) const;

	/**
	 * @brief Returns the next sequence state after playing a card.
	 *
	 * Forwards directly to `gmAlea::SequenceEngine::advance`.
	 *
	 * @param ct    Card type of the card that was played.
	 * @param state Current sequence state for this hero.
	 */
	gmAlea::SequenceState advance(
		gmAlea::CardType          ct,
		const gmAlea::SequenceState& state) const;

	/**
	 * @brief Returns true if playing `ct` in `state` ends the hero's turn.
	 *
	 * Forwards directly to `gmAlea::SequenceEngine::is_turn_ending`.
	 *
	 * @param ct    Card type of the card that was (or will be) played.
	 * @param state Current sequence state for this hero.
	 */
	bool is_turn_ending(gmAlea::CardType ct, const gmAlea::SequenceState& state) const;

	/**
	 * @brief Produces an interrupted state (sequence broken externally).
	 * @param state Current sequence state for this hero.
	 */
	gmAlea::SequenceState interrupt(const gmAlea::SequenceState& state) const;

	/** @brief Returns the default (inactive) sequence state. */
	gmAlea::SequenceState reset() const;

private:
	gmAlea::SequenceEngine _engine; ///< Stateless sequence state machine
};

} // namespace eldhom

#endif // ELDHOM_SEQUENCE_ELDHOMSEQUENCEADAPTER_HPP
