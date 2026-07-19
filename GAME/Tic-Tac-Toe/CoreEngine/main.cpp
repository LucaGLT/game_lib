/**
 * @file main.cpp
 * @brief CoreEngine entry point for the Tic-Tac-Toe game.
 *
 * Creates the TrisEngine facade and a CmdServer that listens for GUI commands
 * on @ref gmTris::ports::COMMANDS. Every command is forwarded to the engine on
 * the server thread, so all game-state mutations are serialised there. The
 * engine connects back to the GUI event server (port 9000) lazily on its first
 * emitted event.
 *
 * Optional CLI arguments `--events-port <port>` and `--commands-port <port>`
 * override the compiled-in defaults (@ref gmTris::ports::EVENTS / COMMANDS).
 * This is what lets eng_serve (see GAME/Tic-Tac-Toe/WebApp) run several
 * independent engine instances at once, one per user session, each pair on
 * its own dynamically-allocated ports. Running with no arguments (as the
 * desktop GUI's launch scripts do) is unchanged.
 */

#include "bridge/CmdServer.hpp"
#include "engine/TrisEngine.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace
{
/// @brief Set to false by the SIGINT/SIGTERM handler to stop the main loop.
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int)
{
	g_running = 0;
}

/// @brief Parses "--flag_name <value>" from argv, or returns fallback if absent/invalid.
uint16_t read_port_option(int argc, char** argv, const std::string& flag_name, uint16_t fallback)
{
	for (int i = 1; i < argc - 1; ++i)
	{
		if (flag_name == argv[i])
		{
			try
			{
				return static_cast<uint16_t>(std::stoi(argv[i + 1]));
			}
			catch (const std::exception&)
			{
				return fallback;
			}
		}
	}
	return fallback;
}
} // namespace

int main(int argc, char** argv)
{
	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);

	const uint16_t events_port =
	    read_port_option(argc, argv, "--events-port", gmTris::ports::EVENTS);
	const uint16_t commands_port =
	    read_port_option(argc, argv, "--commands-port", gmTris::ports::COMMANDS);

	gmTris::TrisEngine engine(events_port);

	gmTris::CmdServer server(
	    commands_port,
	    [&engine](const std::string& typeId, const nlohmann::json& data)
	    { engine.handle_command(typeId, data); });

	server.start();
	std::cout << "[Tris] CoreEngine listening for GUI commands on port " << commands_port << ".\n"
	          << "[Tris] Sending engine events to GUI event server on port " << events_port << ".\n"
	          << "[Tris] Send 'gmTris.new_game' from the GUI to begin.\n";

	while (g_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	server.stop();
	std::cout << "[Tris] CoreEngine stopped.\n";
	return 0;
}
