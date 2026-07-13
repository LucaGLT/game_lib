#ifndef GMALEA_SEQUENCEENGINE_HPP
#define GMALEA_SEQUENCEENGINE_HPP

/**
 * @file SequenceEngine.hpp
 * @brief Pure state machine for multi-card sequences.
 *
 * `SequenceEngine` has **no mutable state**.  All methods are `const`.
 * The caller owns and stores a @ref SequenceState; the engine answers
 * questions about it and produces new state values.
 *
 * ### Responsibilities
 * - Determine whether a card is legally playable given the current sequence
 *   state (`can_play()`).
 * - Compute the next state after a card is played (`advance()`).
 * - Report whether a play ends the actor's turn (`is_turn_ending()`).
 * - Produce an interrupted state (`interrupt()`).
 * - Reset the state to defaults (`reset()`).
 *
 * ### What it does NOT know
 * - Hit points, actors, decks, map locations, or any game concept.
 * - Whether there are cards available to play.
 * - Player choice ("do I want to stop the sequence?") — that is external.
 *
 * ### Sequence rules enforced
 *
 * | State active? | CardType    | Playable? | Turn ends? |
 * |---------------|-------------|-----------|------------|
 * | No            | SINGLE      | ✅        | ✅         |
 * | No            | SEQ_START   | ✅        | ❌ (may continue) |
 * | No            | SEQ_CONTINUE| ❌        | —          |
 * | No            | SEQ_END     | ❌        | —          |
 * | No            | INSTANT     | ✅        | ❌         |
 * | Yes           | SINGLE      | ❌        | —          |
 * | Yes           | SEQ_START   | ❌        | —          |
 * | Yes           | SEQ_CONTINUE| ✅        | ❌ (may continue) |
 * | Yes           | SEQ_END     | ✅        | ✅         |
 * | Yes           | INSTANT     | ✅        | ❌         |
 *
 * @note An interrupted state (`SequenceState::interrupted == true`) blocks
 *       all plays until `reset()` is called.
 *
 * ### Typical usage
 * @code
 *   gmAlea::SequenceEngine engine;
 *   gmAlea::SequenceState  state;        // default: inactive
 *
 *   if (engine.can_play(CardType::SEQ_START, state))
 *       state = engine.advance(CardType::SEQ_START, state);
 *
 *   if (engine.can_play(CardType::SEQ_CONTINUE, state))
 *       state = engine.advance(CardType::SEQ_CONTINUE, state);
 *
 *   // Player decides to stop — turn ends voluntarily (external decision).
 *   // Or: play SEQ_END to close and end the turn automatically.
 *
 *   // External interrupt:
 *   state = engine.interrupt(state);   // state.interrupted == true
 *   state = engine.reset();            // back to defaults
 * @endcode
 */

#include "gmAlea/CardType.hpp"
#include "gmAlea/SequenceState.hpp"

namespace gmAlea {

/**
 * @class SequenceEngine
 * @brief Stateless validator and transition function for card sequences.
 */
class SequenceEngine
{
public:
	// ── Queries ───────────────────────────────────────────────────────────

	/**
	 * @brief Returns true if `card` is legally playable in `state`.
	 *
	 * An interrupted state always returns false for every card type.
	 *
	 * @param card  CardType the actor wants to play.
	 * @param state Current sequence state.
	 * @return true if the play is legal.
	 */
	bool can_play(CardType card, const SequenceState& state) const;

	/**
	 * @brief Returns true if playing `card` automatically ends the actor's turn.
	 *
	 * Only SINGLE and SEQ_END end the turn unconditionally.
	 * SEQ_START and SEQ_CONTINUE leave the turn-end decision to the actor.
	 * INSTANT never ends the turn (it is an out-of-turn play).
	 *
	 * @param card  CardType being played.
	 * @param state State before the play (unused in V1, reserved for future
	 *              policy extensions).
	 * @return true if the turn ends immediately after this play.
	 */
	bool is_turn_ending(CardType card, const SequenceState& state) const;

	// ── Transitions ───────────────────────────────────────────────────────

	/**
	 * @brief Returns the new SequenceState after playing `card`.
	 *
	 * Does not validate legality; callers should call @ref can_play first.
	 * INSTANT cards increment `cards_played` but do not touch `active` or
	 * `last_type`.
	 *
	 * @param card  CardType that was played.
	 * @param state State before the play.
	 * @return Updated sequence state.
	 */
	SequenceState advance(CardType card, const SequenceState& state) const;

	/**
	 * @brief Returns a state that records an external sequence interruption.
	 *
	 * Sets `interrupted = true` and `active = false`.
	 * `cards_played` and `last_type` are preserved for audit/logging.
	 *
	 * @param state State at the moment of interruption.
	 * @return Interrupted state.
	 */
	SequenceState interrupt(const SequenceState& state) const;

	/**
	 * @brief Returns a default-constructed sequence state.
	 *
	 * Equivalent to `SequenceState{}`.  Provided for symmetry with
	 * `advance()` and `interrupt()`.
	 *
	 * @return Fresh state: `active == false`, `cards_played == 0`,
	 *         `interrupted == false`.
	 */
	SequenceState reset() const;
};

} // namespace gmAlea

#endif // GMALEA_SEQUENCEENGINE_HPP
