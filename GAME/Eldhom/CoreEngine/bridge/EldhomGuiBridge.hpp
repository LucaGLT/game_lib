#ifndef ELDHOM_BRIDGE_ELDHOMGUIBRIDGE_HPP
#define ELDHOM_BRIDGE_ELDHOMGUIBRIDGE_HPP

/**
 * @file bridge/EldhomGuiBridge.hpp
 * @brief Outbound event channel: sends engine events to the Eldhom GUI.
 *
 * EldhomGuiBridge wraps a @ref gmDispatch::IpSocketChannel TCP client that
 * connects lazily (on first @ref send_event call) to the GUI event server on
 * port @ref eldhom::ports::EVENTS (9210).
 *
 * Wire format: 4-byte big-endian length prefix + UTF-8 JSON.
 * The payload is placed in `headers["data"]` (Envelope convention) so the
 * PySide6 EngineReceiver normalises it on the same code path as every other
 * gmGui bridge.
 *
 * @note Connection and send failures are silently swallowed so a missing GUI
 *       never crashes the engine.
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include "gmDispatch/channels/IpSocketChannel.hpp"
#include "gmSave/json.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace eldhom {

/**
 * @brief Sends Eldhom engine events to the GUI via TCP length-prefixed frames.
 */
class EldhomGuiBridge
{
public:
	/**
	 * @brief Constructs the bridge pointing at the GUI event server.
	 *
	 * @param host  Remote host of the GUI event server (default: loopback).
	 * @param port  TCP port of the GUI event server.
	 */
	explicit EldhomGuiBridge(
		const std::string& host = "127.0.0.1",
		uint16_t           port = ports::EVENTS);

	/**
	 * @brief Sends one event to the GUI.
	 *
	 * Builds an Envelope with @p typeId, source "EldhomCore" and
	 * `headers["data"]` = @p data serialised as a JSON string, then transmits
	 * it.  Any transport error is silently ignored.
	 *
	 * @param typeId  Event type identifier (see eldhom::EVT_*).
	 * @param data    JSON payload object for the GUI.
	 */
	void send_event(const std::string& typeId, const nlohmann::json& data);

private:
	std::unique_ptr<gmDispatch::IpSocketChannel> _channel;
};

} // namespace eldhom

#endif // ELDHOM_BRIDGE_ELDHOMGUIBRIDGE_HPP
