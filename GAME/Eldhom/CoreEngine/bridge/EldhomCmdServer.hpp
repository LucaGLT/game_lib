#ifndef ELDHOM_BRIDGE_ELDHOMCMDSERVER_HPP
#define ELDHOM_BRIDGE_ELDHOMCMDSERVER_HPP

/**
 * @file bridge/EldhomCmdServer.hpp
 * @brief Inbound command channel: TCP server that receives GUI commands.
 *
 * EldhomCmdServer listens on @ref eldhom::ports::COMMANDS (9211) for a single
 * GUI client.  It reads length-prefixed JSON frames (4-byte big-endian length
 * + UTF-8 payload) on a background thread and invokes the supplied
 * @ref CommandHandler callback for every successfully decoded command.
 *
 * Wire format is identical to gmDispatch::IpSocketChannel so the PySide6
 * EngineSender works with this server without modification.
 *
 * @note JSON parse failures and unknown typeIds are silently discarded.
 */

#include "gmSave/json.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace eldhom {

/**
 * @brief Callback invoked for every command decoded from the GUI.
 */
using CommandHandler =
	std::function<void(const std::string& typeId, const nlohmann::json& data)>;

/**
 * @brief Single-client TCP command server for the Eldhom engine.
 */
class EldhomCmdServer
{
public:
	/**
	 * @brief Constructs the command server.
	 *
	 * @param port     TCP port to listen on.
	 * @param handler  Callback invoked for each decoded command.
	 */
	explicit EldhomCmdServer(uint16_t port, CommandHandler handler);

	/// @brief Stops the server if running and joins the worker thread.
	~EldhomCmdServer();

	EldhomCmdServer(const EldhomCmdServer&)            = delete;
	EldhomCmdServer& operator=(const EldhomCmdServer&) = delete;

	/// @brief Starts the accept/receive loop on a background thread.
	void start();

	/// @brief Signals the loop to stop and joins the worker thread.
	void stop();

private:
	void run();

	uint16_t          _port;
	CommandHandler    _handler;
	std::thread       _thread;
	std::atomic<bool> _running{false};
	int               _server_fd{-1};
};

} // namespace eldhom

#endif // ELDHOM_BRIDGE_ELDHOMCMDSERVER_HPP
