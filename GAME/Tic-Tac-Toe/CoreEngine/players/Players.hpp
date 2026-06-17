#ifndef GMTRIS_PLAYERS_HPP
#define GMTRIS_PLAYERS_HPP

/**
 * @file players/Players.hpp
 * @brief Player identity and per-player status tracking.
 *
 * @note Phase 1 tracks players and statuses in plain members. Phase 2 will back
 *       this with a gmActor ActorStore (two actors, status ACTIVE_TURN / WINNER
 *       / DRAW), reusing gmActor's status container.
 */

#include "engine/TrisTypes.hpp"

#include <string>

namespace gmTris
{

/**
 * @class Players
 * @brief Maps the two marks to actor ids and tracks the active-turn owner.
 */
class Players
{
  public:
	/// @brief Returns the stable actor id for @p mark ("Player_X" / "Player_O").
	std::string actor_id(Mark mark) const;

	/// @brief Returns the human-readable display name for @p mark.
	std::string display_name(Mark mark) const;

	/// @brief Sets which mark currently owns ACTIVE_TURN.
	void set_active(Mark mark);

	/// @brief Returns the mark that currently owns ACTIVE_TURN.
	Mark active() const;

	/// @brief Returns the opponent of @p mark.
	static Mark opponent(Mark mark);

  private:
	Mark _active = Mark::X;
};

} // namespace gmTris

#endif // GMTRIS_PLAYERS_HPP
