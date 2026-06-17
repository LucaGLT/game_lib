/**
 * @file rules/WinRules.cpp
 * @brief Win/draw evaluation expressed through the gmRules engine.
 *
 * Each of the 8 winning lines is encoded as an ALL_OF composite of three
 * LOCATION_HAS_TAG conditions ("mark_X" / "mark_O"). The conditions are
 * evaluated by gmRules::gmRulesEngine against a TrisRuleContext that exposes
 * the board cells as tagged locations.
 */

#include "WinRules.hpp"

#include "TrisRuleContext.hpp"

#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/facade/gmRulesEngine.hpp"

#include <array>
#include <cstdint>

namespace gmTris
{

namespace
{

/// @brief A winning line as three (row, col) 1-based coordinate pairs plus id.
struct Line
{
	uint8_t     r1, c1, r2, c2, r3, c3;
	const char* id;
};

/// @brief The 8 winning lines of Tic-Tac-Toe.
constexpr std::array<Line, 8> WINNING_LINES = {{
    {1, 1, 1, 2, 1, 3, "row_1"},
    {2, 1, 2, 2, 2, 3, "row_2"},
    {3, 1, 3, 2, 3, 3, "row_3"},
    {1, 1, 2, 1, 3, 1, "col_1"},
    {1, 2, 2, 2, 3, 2, "col_2"},
    {1, 3, 2, 3, 3, 3, "col_3"},
    {1, 1, 2, 2, 3, 3, "diag_main"},
    {1, 3, 2, 2, 3, 1, "diag_anti"},
}};

/// @brief Builds an ALL_OF condition: the three @p line cells carry @p tag.
gmRules::ConditionSpec line_condition(const Line &line, const std::string &tag)
{
	gmRules::ConditionSpec spec;
	spec.op = gmRules::CompositeOperator::ALL_OF;
	for (const std::pair<uint8_t, uint8_t> &cell :
	     {std::make_pair(line.r1, line.c1), std::make_pair(line.r2, line.c2),
	      std::make_pair(line.r3, line.c3)})
	{
		gmRules::ConditionSpec atom;
		atom.type       = gmRules::ConditionType::LOCATION_HAS_TAG;
		atom.subject_id = TrisRuleContext::cell_id(cell.first, cell.second);
		atom.value      = tag;
		spec.children.push_back(atom);
	}
	return spec;
}

} // namespace

Evaluation WinRules::evaluate(const Board& board) const
{
	TrisRuleContext       ctx(board);
	gmRules::gmRulesEngine engine;

	for (Mark mark : {Mark::X, Mark::O})
	{
		const std::string tag = TrisRuleContext::mark_tag(mark);
		for (const Line& line : WINNING_LINES)
		{
			const gmRules::ConditionSpec spec = line_condition(line, tag);
			if (engine.evaluate_condition(spec, ctx).valid())
			{
				return Evaluation{Outcome::WIN, mark, line.id};
			}
		}
	}

	if (board.is_full())
	{
		return Evaluation{Outcome::DRAW, Mark::EMPTY, ""};
	}

	return Evaluation{Outcome::NONE, Mark::EMPTY, ""};
}

} // namespace gmTris
