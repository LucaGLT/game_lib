/**
 * @file main.cpp
 * @brief CoreEngine entry point for the Dungeon Crawler Basic game.
 *
 * Creates the @ref gmDungeonBasic::DungeonEngine facade and a
 * @ref gmDungeonBasic::CmdServer that listens for GUI commands on
 * @ref gmDungeonBasic::ports::COMMANDS (9201). Every decoded command is
 * forwarded to the engine on the CmdServer worker thread; all game-state
 * mutations are therefore serialised there. The engine connects back to the
 * GUI event server on port @ref gmDungeonBasic::ports::EVENTS (9200) lazily
 * on the first emitted event.
 *
 * @note Ports 9200/9201 are chosen to avoid conflicts with the Tic-Tac-Toe
 *       engine (9100/9001) and with software that occupies port 9000 on
 *       Windows.
 */

#include "bridge/CmdServer.hpp"
#include "engine/DungeonEngine.hpp"
#include "engine/DungeonTypes.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace
{
/// @brief Set to false by the SIGINT/SIGTERM handler to stop the main loop.
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int)
{
	g_running = 0;
}
} // namespace

int main()
{
	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);

	gmDungeonBasic::DungeonEngine engine;

	gmDungeonBasic::CmdServer server(
	    gmDungeonBasic::ports::COMMANDS,
	    [&engine](const std::string& typeId, const nlohmann::json& data)
	    { engine.handle_command(typeId, data); });

	server.start();

	std::cout << "[DungeonEngine] CoreEngine listening on port "
	          << gmDungeonBasic::ports::COMMANDS << ".\n"
	          << "[DungeonEngine] Send 'dungeon.new_game' from the GUI to begin.\n";

	while (g_running)
	{
		engine.advance_turn();
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	server.stop();
	std::cout << "[DungeonEngine] CoreEngine stopped.\n";
	return 0;
}
