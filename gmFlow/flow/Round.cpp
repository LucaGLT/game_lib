/**
 * @file flow/Round.cpp
 * @brief Implementation of gmFlow::Round.
 */

#include "gmFlow/flow/Round.hpp"

namespace gmFlow {

Round::Round(RoundId id, int index)
    : id_(std::move(id))
    , index_(index)
{}

const RoundId& Round::id()    const { return id_;    }
int            Round::index() const { return index_; }

} // namespace gmFlow
