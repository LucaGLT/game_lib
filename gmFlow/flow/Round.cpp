/**
 * @file flow/Round.cpp
 * @brief Implementation of gmFlow::Round.
 */

#include "gmFlow/flow/Round.hpp"

namespace gmFlow {

Round::Round(RoundId id, int index)
    : _id(std::move(id))
    , _index(index)
{}

const RoundId& Round::id()    const { return _id;    }
int            Round::index() const { return _index; }

} // namespace gmFlow
