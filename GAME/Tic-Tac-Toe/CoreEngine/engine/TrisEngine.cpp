/**
 * @file engine/TrisEngine.cpp
 * @brief Implementation of the Tris coordinator.
 */

#include "TrisEngine.hpp"

namespace gmTris
{

TrisEngine::TrisEngine(uint16_t events_port) : _gui("127.0.0.1", events_port)
{
}

Mark TrisEngine::parse_mark(const std::string& symbol)
{
	if (symbol == "X")
	{
		return Mark::X;
	}
	if (symbol == "O")
	{
		return Mark::O;
	}
	return Mark::EMPTY;
}

Phase TrisEngine::phase_for(Mark mark) const
{
	return (mark == _starter_mark) ? Phase::PLAYER1_TURN : Phase::PLAYER2_TURN;
}

nlohmann::json TrisEngine::build_actor_snapshot() const
{
	nlohmann::json actors = nlohmann::json::array();
	for (Mark mark : {Mark::X, Mark::O})
	{
		nlohmann::json statuses = nlohmann::json::array();
		if (_flow.is_active() && _players.active() == mark)
		{
			statuses.push_back("ACTIVE_TURN");
		}
		actors.push_back({
		    {"actor_id", _players.actor_id(mark)},
		    {"display_name", _players.display_name(mark)},
		    {"symbol", mark_to_string(mark)},
		    {"statuses", statuses},
		});
	}
	return nlohmann::json{{"actors", actors}};
}

nlohmann::json TrisEngine::build_board_snapshot() const
{
	nlohmann::json cells = nlohmann::json::array();
	for (uint8_t row = 1; row <= Board::SIZE; ++row)
	{
		for (uint8_t col = 1; col <= Board::SIZE; ++col)
		{
			cells.push_back({
			    {"row", row},
			    {"col", col},
			    {"mark", mark_to_string(_board.at(row, col))},
			});
		}
	}
	return nlohmann::json{{"size", Board::SIZE}, {"cells", cells}};
}

void TrisEngine::start_game(StarterMode mode)
{
	_board.reset();
	_players.reset_statuses();

	const StarterResult start = _starter.choose(mode);
	_starter_mark             = start.first;
	_players.set_active(start.first);
	_flow.start(phase_for(start.first));

	_log.info("start_game starter=" + mark_to_string(start.first) +
	          (start.used_dice ? " (dice 1d2 = " + std::to_string(start.roll) + ")"
	                           : " (fixed)"));

	if (start.used_dice)
	{
		_gui.send_event(event_id::DICE_ROLLED,
		                {{"value", start.roll}, {"first", mark_to_string(start.first)}});
	}

	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());
	_gui.send_event(event_id::MAP_SNAPSHOT, build_board_snapshot());
	_gui.send_event(event_id::SESSION_STARTED,
	                {{"phase", phase_to_string(_flow.phase())}});
	_gui.send_event(event_id::STATUS_ADDED,
	                {{"actor_id", _players.actor_id(start.first)},
	                 {"status", "ACTIVE_TURN"}});
}

void TrisEngine::handle_command(const std::string& typeId, const nlohmann::json& data)
{
	if (typeId == command_id::MOVE)
	{
		handle_move(data);
	}
	else if (typeId == command_id::NEW_GAME)
	{
		const std::string mode = data.value("starter_mode", "fixed_x");
		start_game(mode == "dice_1d2" ? StarterMode::DICE_1D2 : StarterMode::FIXED_X);
	}
}

void TrisEngine::handle_move(const nlohmann::json& data)
{
	const Mark    player = parse_mark(data.value("player", ""));
	const uint8_t row    = static_cast<uint8_t>(data.value("row", 0));
	const uint8_t col    = static_cast<uint8_t>(data.value("col", 0));

	// ── Validation ────────────────────────────────────────────────────────────
	std::string reason;
	if (!_flow.is_active())
	{
		reason = "game_over";
	}
	else if (player == Mark::EMPTY || player != _players.active())
	{
		reason = "not_your_turn";
	}
	else if (!_board.in_range(row, col))
	{
		reason = "out_of_range";
	}
	else if (!_board.is_empty(row, col))
	{
		reason = "cell_occupied";
	}

	if (!reason.empty())
	{
		_log.warn("invalid_move reason=" + reason);
		_gui.send_event(event_id::INVALID_MOVE,
		                {{"player", mark_to_string(player)},
		                 {"row", row},
		                 {"col", col},
		                 {"reason", reason}});
		return;
	}

	// ── Apply move ────────────────────────────────────────────────────────────
	_board.set(row, col, player);
	_log.info("move player=" + mark_to_string(player) + " row=" +
	          std::to_string(row) + " col=" + std::to_string(col));
	_gui.send_event(event_id::CELL_CHANGED,
	                {{"row", row}, {"col", col}, {"mark", mark_to_string(player)}});

	// ── Evaluate outcome ──────────────────────────────────────────────────────
	const Evaluation eval = _rules.evaluate(_board);

	if (eval.outcome == Outcome::WIN)
	{
		_players.mark_winner(player);
		_gui.send_event(event_id::STATUS_REMOVED,
		                {{"actor_id", _players.actor_id(player)},
		                 {"status", "ACTIVE_TURN"}});
		_gui.send_event(event_id::STATUS_ADDED,
		                {{"actor_id", _players.actor_id(player)},
		                 {"status", "WINNER"}});
		_flow.set_phase(Phase::GAME_OVER);
		_gui.send_event(event_id::PHASE_CHANGED, {{"phase", "GAME_OVER"}});
		_gui.send_event(event_id::GAME_WON,
		                {{"player", mark_to_string(player)}, {"line", eval.line}});
		_log.info("game_won player=" + mark_to_string(player) + " line=" + eval.line);
		return;
	}

	if (eval.outcome == Outcome::DRAW)
	{
		_players.mark_draw();
		_gui.send_event(event_id::STATUS_REMOVED,
		                {{"actor_id", _players.actor_id(player)},
		                 {"status", "ACTIVE_TURN"}});
		for (Mark mark : {Mark::X, Mark::O})
		{
			_gui.send_event(event_id::STATUS_ADDED,
			                {{"actor_id", _players.actor_id(mark)}, {"status", "DRAW"}});
		}
		_flow.set_phase(Phase::GAME_OVER);
		_gui.send_event(event_id::PHASE_CHANGED, {{"phase", "GAME_OVER"}});
		_gui.send_event(event_id::GAME_DRAW, nlohmann::json::object());
		_log.info("game_draw");
		return;
	}

	// ── Continue: switch the active player ────────────────────────────────────
	const Mark next = Players::opponent(player);
	_gui.send_event(event_id::STATUS_REMOVED,
	                {{"actor_id", _players.actor_id(player)}, {"status", "ACTIVE_TURN"}});
	_players.set_active(next);
	_flow.set_phase(phase_for(next));
	_gui.send_event(event_id::STATUS_ADDED,
	                {{"actor_id", _players.actor_id(next)}, {"status", "ACTIVE_TURN"}});
	_gui.send_event(event_id::PHASE_CHANGED,
	                {{"phase", phase_to_string(_flow.phase())}});
}

} // namespace gmTris
