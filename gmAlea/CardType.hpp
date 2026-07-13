#ifndef GMALEA_CARDTYPE_HPP
#define GMALEA_CARDTYPE_HPP

/**
 * @file CardType.hpp
 * @brief Classification of a card's sequencing behaviour.
 *
 * `CardType` is the single piece of metadata that the @ref SequenceEngine
 * needs to decide whether a card can be played and how it affects the current
 * @ref SequenceState.  It carries no game-specific semantics.
 *
 * Game engines store this value in their card-definition data structures and
 * pass it to `SequenceEngine::can_play()` / `SequenceEngine::advance()`.
 */

namespace gmAlea {

/**
 * @enum CardType
 * @brief Describes how a card interacts with the active sequence.
 *
 * | Value        | Can be played without sequence | Can be played inside sequence |
 * |--------------|-------------------------------|-------------------------------|
 * | SINGLE       | ✅ — ends the turn             | ❌                             |
 * | SEQ_START    | ✅ — opens a sequence          | ❌                             |
 * | SEQ_CONTINUE | ❌                             | ✅ — keeps sequence open       |
 * | SEQ_END      | ❌                             | ✅ — closes sequence, ends turn|
 * | INSTANT      | ✅ — does not affect sequence  | ✅ — does not affect sequence  |
 */
enum class CardType
{
	SINGLE,       ///< Standalone — valid without a sequence; ends the turn.
	SEQ_START,    ///< Opens a sequence; may be followed by SEQ_CONTINUE or SEQ_END.
	SEQ_CONTINUE, ///< Extends an active sequence; invalid outside one.
	SEQ_END,      ///< Closes the sequence; invalid outside one; ends the turn.
	INSTANT       ///< Out-of-turn / reaction — valid in any state; does not alter sequence.
};

/**
 * @brief Returns a stable human-readable name for a CardType enumerator.
 *
 * @param type The card type to name.
 * @return Null-terminated string label, e.g. @c "SINGLE", @c "SEQ_START".
 */
inline const char* card_type_name(CardType type)
{
	switch (type)
	{
		case CardType::SINGLE:       return "SINGLE";
		case CardType::SEQ_START:    return "SEQ_START";
		case CardType::SEQ_CONTINUE: return "SEQ_CONTINUE";
		case CardType::SEQ_END:      return "SEQ_END";
		case CardType::INSTANT:      return "INSTANT";
	}
	return "UNKNOWN";
}

} // namespace gmAlea

#endif // GMALEA_CARDTYPE_HPP
