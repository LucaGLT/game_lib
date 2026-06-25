#ifndef GMDUNGEONBASIC_CMDSERVER_HPP
#define GMDUNGEONBASIC_CMDSERVER_HPP

/**
 * @file bridge/CmdServer.hpp
 * @brief Inbound command channel: TCP server that receives GUI commands.
 *
 * CmdServer listens on @ref gmDungeonBasic::ports::COMMANDS (9201) for a
 * single GUI client. It reads length-prefixed JSON frames (4-byte big-endian
 * length + UTF-8 payload) on a background thread and invokes the supplied
 * @ref CommandHandler callback for every successfully decoded command.
 *
 * The wire format is identical to the one used by @c gmDispatch::IpSocketChannel
 * and by the Tic-Tac-Toe CmdServer, ensuring that the PySide6 @c EngineSender
 * works with this server without modification.
 *
 * @note JSON parse failures and unknown typeIds are silently discarded so that
 *       malformed GUI messages never crash the engine.
 */

#include "gmSave/json.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace gmDungeonBasic
{

/**
 * @brief Callback invoked for every command decoded from the GUI.
 *
 * @param typeId  Command type identifier string.
 * @param data    Command payload as a JSON object.
 */
using CommandHandler =
    std::function<void(const std::string& typeId, const nlohmann::json& data)>;

/**
 * @brief Single-client TCP command server for the dungeon engine.
 *
 * Accepts one GUI connection at a time. If the client disconnects the server
 * re-enters the accept loop and waits for the next connection, enabling GUI
 * restarts without restarting the engine.
 */
class CmdServer
{
public:
	/**
	 * @brief Constructs the command server.
	 *
	 * @param port     TCP port to listen on.
	 * @param handler  Callback invoked for each decoded command.
	 */
	explicit CmdServer(uint16_t port, CommandHandler handler);

	/// @brief Stops the server if running and joins the worker thread.
	~CmdServer();

	CmdServer(const CmdServer&)            = delete;
	CmdServer& operator=(const CmdServer&) = delete;

	/// @brief Starts the accept/receive loop on a background thread.
	void start();

	/// @brief Signals the loop to stop and joins the worker thread.
	void stop();

private:
	/// @brief Worker-thread entry point: bind, listen, accept, receive loop.
	void run();

	uint16_t          _port;
	CommandHandler    _handler;
	std::thread       _thread;
	std::atomic<bool> _running{false};
	int               _server_fd{-1};  ///< Listening socket descriptor.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_CMDSERVER_HPP
