#ifndef GMTRIS_CMDSERVER_HPP
#define GMTRIS_CMDSERVER_HPP

/**
 * @file bridge/CmdServer.hpp
 * @brief Inbound command channel: TCP server that receives GUI commands.
 *
 * A background thread accepts a single GUI client on @ref gmTris::ports::COMMANDS
 * and reads length-prefixed JSON frames (4-byte big-endian length + UTF-8
 * payload) — the same wire format used by @ref gmDispatch::IpSocketChannel.
 * Every decoded command invokes the user-supplied @ref CommandHandler with the
 * command @c typeId and its JSON @c data object.
 */

#include "gmSave/json.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace gmTris
{

/// @brief Callback invoked for every command decoded from the GUI.
using CommandHandler =
    std::function<void(const std::string& typeId, const nlohmann::json& data)>;

/**
 * @class CmdServer
 * @brief Single-client TCP server that decodes GUI command frames.
 */
class CmdServer
{
  public:
	/**
	 * @brief Constructs the command server.
	 *
	 * @param port    TCP port to listen on.
	 * @param handler Callback invoked for each received command.
	 */
	explicit CmdServer(uint16_t port, CommandHandler handler);

	/// @brief Stops the server and joins the worker thread.
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
	int               _server_fd{-1};
};

} // namespace gmTris

#endif // GMTRIS_CMDSERVER_HPP
