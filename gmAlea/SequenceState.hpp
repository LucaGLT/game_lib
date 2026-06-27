#ifndef GMALEA_SEQUENCESTATE_HPP
#define GMALEA_SEQUENCESTATE_HPP

/**
 * @file SequenceState.hpp
 * @brief Immutable snapshot of a card-sequence state for one actor.
 *
 * `SequenceState` is a plain-old-data struct.  It is **never mutated in
 * place**; @ref SequenceEngine produces a new value via
 * @ref SequenceEngine::advance().  Callers own and store the struct;
 * the engine is stateless.
 *
 * The default-constructed state represents "no sequence active, turn not
 * yet started".
 */

#include "gmAlea/CardType.hpp"

namespace gmAlea {

/**
 * @brief Snapshot of the current sequence state for one actor in one turn.
 *
 * @note Equality operators are intentionally omitted.  Comparing two
 *       `SequenceState` values by fields avoids accidental reliance on
 *       `last_type` when `active == false`.
 */
struct SequenceState
{
	/// True while a sequence opened with SEQ_START is still in progress.
	bool active = false;

	/// Type of the last card played during this turn.
	/// Only meaningful when `active == true`; ignored otherwise.
	CardType last_type = CardType::SINGLE;

	/// Total number of cards played in the current turn (including INSTANTs).
	int cards_played = 0;

	/// True if the sequence was cut short by an external effect (e.g. a
	/// monster reaction).  An interrupted state blocks further plays until
	/// the engine resets it.
	bool interrupted = false;
};

} // namespace gmAlea

#endif // GMALEA_SEQUENCESTATE_HPP
