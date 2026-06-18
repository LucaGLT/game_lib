/**
 * @file main.cpp
 * @brief CoreEngine entry point for the Tic-Tac-Toe game.
 *
 * Creates the TrisEngine facade and a CmdServer that listens for GUI commands
 * on @ref gmTris::ports::COMMANDS. Every command is forwarded to the engine on
 * the server thread, so all game-state mutations are serialised there. The
 * engine connects back to the GUI event server (port 9000) lazily on its first
 * emitted event.
 */

#include "bridge/CmdServer.hpp"
#include "engine/TrisEngine.hpp"

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

	gmTris::TrisEngine engine;

	gmTris::CmdServer server(
	    gmTris::ports::COMMANDS,
	    [&engine](const std::string& typeId, const nlohmann::json& data)
	    { engine.handle_command(typeId, data); });

	server.start();
	std::cout << "[Tris] CoreEngine listening for GUI commands on port "
	          << gmTris::ports::COMMANDS << ".\n"
	          << "[Tris] Send 'gmTris.new_game' from the GUI to begin.\n";

	while (g_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	server.stop();
	std::cout << "[Tris] CoreEngine stopped.\n";
	return 0;
}
