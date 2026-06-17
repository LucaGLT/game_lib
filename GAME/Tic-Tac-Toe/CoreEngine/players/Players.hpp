#ifndef GMTRIS_PLAYERS_HPP
#define GMTRIS_PLAYERS_HPP

/**
 * @file players/Players.hpp
 * @brief Player identity and per-player status tracking, backed by gmActor.
 *
 * Phase 2 backs the two players with a gmActor::ActorStore holding two heroes
 * ("Player_X" / "Player_O"). Turn ownership and end-game results are expressed
 * as gmActor status instances (ACTIVE_TURN / WINNER / DRAW) attached to each
 * hero's common state, reusing the same status model as the full engine.
 */

#include "engine/TrisTypes.hpp"

#include "gmActor/actors/ActorStore.hpp"

#include <string>

namespace gmTris
{

/**
 * @class Players
 * @brief Maps the two marks to gmActor heroes and tracks per-player statuses.
 */
class Players
{
  public:
	/// @brief Builds the two heroes (Player_X / Player_O) in the actor store.
	Players();

	/// @brief Returns the stable actor id for @p mark ("Player_X" / "Player_O").
	std::string actor_id(Mark mark) const;

	/// @brief Returns the human-readable display name for @p mark.
	std::string display_name(Mark mark) const;

	/// @brief Sets which mark currently owns the ACTIVE_TURN status.
	void set_active(Mark mark);

	/// @brief Returns the mark that currently owns ACTIVE_TURN.
	Mark active() const;

	/// @brief Flags @p mark as the winner (WINNER status) and clears ACTIVE_TURN.
	void mark_winner(Mark mark);

	/// @brief Flags both players with the DRAW status and clears ACTIVE_TURN.
	void mark_draw();

	/// @brief Clears all per-player statuses (new game).
	void reset_statuses();

	/// @brief Returns true if @p mark currently carries @p status.
	bool has_status(Mark mark, const std::string &status) const;

	/// @brief Returns the opponent of @p mark.
	static Mark opponent(Mark mark);

	/// @brief Read-only access to the backing actor store.
	const gmActor::ActorStore &store() const;

  private:
	/// @brief Status id carried by the player whose turn it is.
	static const std::string STATUS_ACTIVE_TURN;
	/// @brief Status id carried by the winning player.
	static const std::string STATUS_WINNER;
	/// @brief Status id carried by both players on a draw.
	static const std::string STATUS_DRAW;

	/// @brief Source id recorded on status instances applied by the engine.
	static const std::string SOURCE_ENGINE;

	/// @brief Returns the actor id for @p mark without consulting the store.
	static std::string id_of(Mark mark);

	/// @brief Adds @p status to the hero for @p mark (replace semantics).
	void add_status(Mark mark, const std::string &status);

	/// @brief Removes @p status from the hero for @p mark, if present.
	void remove_status(Mark mark, const std::string &status);

	gmActor::ActorStore _store;
	Mark _active = Mark::X;
};

} // namespace gmTris

#endif // GMTRIS_PLAYERS_HPP
