#include "gmDeck.hpp"
#include <algorithm>
#include <sstream>

namespace gmFate {

gmDeck::gmDeck(const std::vector<uint32_t>& token_ids,
               std::optional<unsigned int> seed,
               bool auto_shuffle,
               bool allow_duplicates)
    : _initial_token_ids(token_ids), _seed(seed), _allow_duplicates(allow_duplicates) {
    if (!allow_duplicates) {
        _validate_token_ids(token_ids);
    }
    
    _deck = std::vector<uint32_t>(token_ids);
    
    if (seed.has_value()) {
        _rng.seed(seed.value());
    } else {
        std::random_device rd;
        _rng.seed(rd());
    }
    
    if (auto_shuffle) {
        shuffle();
    }
}

void gmDeck::_validate_token_ids(const std::vector<uint32_t>& token_ids) {
    std::unordered_map<uint32_t, int> counts;
    std::vector<uint32_t> duplicates;
    
    for (uint32_t token_id : token_ids) {
        counts[token_id]++;
        if (counts[token_id] > 1 && std::find(duplicates.begin(), duplicates.end(), token_id) == duplicates.end()) {
            duplicates.push_back(token_id);
        }
    }
    
    if (!duplicates.empty()) {
        std::ostringstream oss;
        oss << "Duplicate token IDs are not allowed: [";
        for (size_t i = 0; i < duplicates.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << duplicates[i];
        }
        oss << "]";
        throw DuplicateTokenIdError(oss.str());
    }
}

void gmDeck::shuffle() {
    std::shuffle(_deck.begin(), _deck.end(), _rng);
}

uint32_t gmDeck::draw_one() {
    if (is_empty()) {
        throw DeckEmptyError("Cannot draw from an empty deck");
    }
    uint32_t token = _deck.front();
    _deck.erase(_deck.begin());
    return token;
}

std::vector<uint32_t> gmDeck::draw_many(int k) {
    if (k <= 0) {
        throw InvalidDrawCountError("k must be greater than zero");
    }
    if (k > remaining_count()) {
        std::ostringstream oss;
        oss << "Cannot draw " << k << " tokens from deck with " 
            << remaining_count() << " remaining";
        throw DeckEmptyError(oss.str());
    }
    
    std::vector<uint32_t> drawn;
    for (int i = 0; i < k; ++i) {
        drawn.push_back(draw_one());
    }
    return drawn;
}

int gmDeck::remaining_count() const {
    return static_cast<int>(_deck.size());
}

bool gmDeck::is_empty() const {
    return _deck.empty();
}

void gmDeck::reset(const std::optional<std::vector<uint32_t>>& token_ids) {
    if (token_ids.has_value()) {
        if (!_allow_duplicates) {
            _validate_token_ids(token_ids.value());
        }
        _initial_token_ids = token_ids.value();
    }
    
    _deck = std::vector<uint32_t>(_initial_token_ids);
    
    if (_seed.has_value()) {
        _rng.seed(_seed.value());
    } else {
        std::random_device rd;
        _rng.seed(rd());
    }
    
    shuffle();
}

std::vector<uint32_t> gmDeck::peek_all() const {
    return std::vector<uint32_t>(_deck);
}

void gmDeck::remove(uint32_t token_id) {
    auto it = std::find(_deck.begin(), _deck.end(), token_id);
    if (it != _deck.end()) {
        _deck.erase(it);
    }
}

bool gmDeck::contains(uint32_t token_id) const {
    return std::find(_deck.begin(), _deck.end(), token_id) != _deck.end();
}

void gmDeck::push_back(uint32_t token_id) {
    if (!_allow_duplicates && contains(token_id)) {
        throw DuplicateTokenIdError(
            "Token " + std::to_string(token_id) + " already exists in deck");
    }
    _deck.push_back(token_id);
}

void gmDeck::push_front(uint32_t token_id) {
    if (!_allow_duplicates && contains(token_id)) {
        throw DuplicateTokenIdError(
            "Token " + std::to_string(token_id) + " already exists in deck");
    }
    _deck.insert(_deck.begin(), token_id);
}

uint32_t gmDeck::draw_specific(uint32_t token_id) {
    if (!contains(token_id)) {
        throw TokenNotFoundError(
            "Token " + std::to_string(token_id) + " not found in deck");
    }
    remove(token_id);
    return token_id;
}

} // namespace gmFate
