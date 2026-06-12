#include "SimpleDeck.hpp"

namespace gmAlea
{

std::vector<uint32_t> SimpleDeck::_extract_ids(const std::vector<Token>& tokens)
{
	std::vector<uint32_t> ids;
	for (const auto& token : tokens)
	{
		ids.push_back(token.id);
	}
	return ids;
}

SimpleDeck::SimpleDeck(const std::vector<Token>& tokens,
					   std::optional<unsigned int> seed,
					   bool allow_duplicates)
	: _deck(_extract_ids(tokens), seed, true, allow_duplicates),
	  _initial_tokens(tokens),
	  _allow_duplicates(allow_duplicates)
{
	for (const auto& token : tokens)
	{
		_token_db[token.id] = token;
	}
}

Token SimpleDeck::draw_one()
{
	uint32_t id = _deck.draw_one();
	return _token_db.at(id);
}

std::vector<Token> SimpleDeck::draw_many(int k)
{
	std::vector<uint32_t> ids = _deck.draw_many(k);
	std::vector<Token> tokens;
	for (uint32_t id : ids)
	{
		tokens.push_back(_token_db.at(id));
	}
	return tokens;
}

int SimpleDeck::remaining_count() const
{
	return _deck.remaining_count();
}

bool SimpleDeck::is_empty() const
{
	return _deck.is_empty();
}

void SimpleDeck::shuffle()
{
	_deck.shuffle();
}

void SimpleDeck::reset(const std::optional<std::vector<Token>>& tokens)
{
	if (tokens.has_value())
	{
		_initial_tokens = tokens.value();
		_token_db.clear();
		for (const auto& token : tokens.value())
		{
			_token_db[token.id] = token;
		}
		_deck.reset(_extract_ids(tokens.value()));
	}
	else
	{
		_deck.reset(_extract_ids(_initial_tokens));
	}
}

std::vector<Token> SimpleDeck::peek_all() const
{
	std::vector<uint32_t> ids = _deck.peek_all();
	std::vector<Token> tokens;
	for (uint32_t id : ids)
	{
		tokens.push_back(_token_db.at(id));
	}
	return tokens;
}

void SimpleDeck::remove(uint32_t token_id)
{
	_deck.remove(token_id);
}

bool SimpleDeck::contains(uint32_t token_id) const
{
	return _deck.contains(token_id);
}

void SimpleDeck::push_back(const Token& token)
{
	if (!_allow_duplicates && _token_db.count(token.id) > 0)
	{
		throw EAleaDuplicateTokenIdError(
			"Token " + std::to_string(token.id) + " already exists in deck");
	}
	_token_db[token.id] = token;
	_deck.push_back(token.id);
}

void SimpleDeck::push_front(const Token& token)
{
	if (!_allow_duplicates && _token_db.count(token.id) > 0)
	{
		throw EAleaDuplicateTokenIdError(
			"Token " + std::to_string(token.id) + " already exists in deck");
	}
	_token_db[token.id] = token;
	_deck.push_front(token.id);
}

Token SimpleDeck::see_top() const
{
	uint32_t id = _deck.see_top();
	return _token_db.at(id);
}

Token SimpleDeck::see_bottom() const
{
	uint32_t id = _deck.see_bottom();
	return _token_db.at(id);
}

Token SimpleDeck::draw_specific(uint32_t token_id)
{
	_deck.draw_specific(token_id);
	return _token_db.at(token_id);
}

void SimpleDeck::reseed(unsigned int seed)
{
	_deck.reseed(seed);
}

} // namespace gmAlea

