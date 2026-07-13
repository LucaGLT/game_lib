/**
 * @file sequence/EldhomSequenceAdapter.cpp
 * @brief Implementation of EldhomSequenceAdapter.
 */

#include "GAME/Eldhom/CoreEngine/sequence/EldhomSequenceAdapter.hpp"

namespace eldhom {

bool EldhomSequenceAdapter::can_play(
	gmAlea::CardType             ct,
	const gmAlea::SequenceState& state) const
{
	return _engine.can_play(ct, state);
}

gmAlea::SequenceState EldhomSequenceAdapter::advance(
	gmAlea::CardType             ct,
	const gmAlea::SequenceState& state) const
{
	return _engine.advance(ct, state);
}

bool EldhomSequenceAdapter::is_turn_ending(
	gmAlea::CardType             ct,
	const gmAlea::SequenceState& state) const
{
	return _engine.is_turn_ending(ct, state);
}

gmAlea::SequenceState EldhomSequenceAdapter::interrupt(
	const gmAlea::SequenceState& state) const
{
	return _engine.interrupt(state);
}

gmAlea::SequenceState EldhomSequenceAdapter::reset() const
{
	return _engine.reset();
}

} // namespace eldhom
