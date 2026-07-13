#ifndef GMDUNGEONBASIC_GUIBRIDGE_HPP
#define GMDUNGEONBASIC_GUIBRIDGE_HPP

/**
 * @file bridge/GuiBridge.hpp
 * @brief Outbound event channel: sends engine events to the GUI over TCP.
 *
 * GuiBridge wraps a @ref gmDispatch::IpSocketChannel TCP client that connects
 * lazily (on first @ref send_event call) to the GUI event server on port
 * @ref gmDungeonBasic::ports::EVENTS (9200). Each event is serialised as a
 * length-prefixed JSON frame (4-byte big-endian length + UTF-8 payload)
 * compatible with the @c engine_bridge framing layer in @c pyLib/gmGui.
 *
 * The @c headers["data"] convention from Tic-Tac-Toe is reused: the JSON
 * payload is embedded as a string inside @c headers["data"] so that the
 * PySide6 @c EngineReceiver can normalise it with the same code path.
 *
 * @note Connection and send failures are silently swallowed so a missing GUI
 *       never crashes the engine.
 */

#include "engine/DungeonTypes.hpp"

#include "gmDispatch/channels/IpSocketChannel.hpp"
#include "gmSave/json.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Sends dungeon engine events to the GUI via a TCP length-prefixed frame.
 */
class GuiBridge
{
public:
	/**
	 * @brief Constructs the bridge pointing at the GUI event server.
	 *
	 * @param host  Remote host of the GUI event server (default: loopback).
	 * @param port  TCP port of the GUI event server.
	 */
	explicit GuiBridge(const std::string& host = "127.0.0.1",
	                   uint16_t           port = ports::EVENTS);

	/**
	 * @brief Sends one event to the GUI.
	 *
	 * Builds an @c Envelope with @p typeId, source @c "DungeonCore" and
	 * @c headers["data"] = @p data serialised as a JSON string, then transmits
	 * it. Any transport error is silently ignored.
	 *
	 * @param typeId  Event type identifier (see @ref gmDungeonBasic::event_id).
	 * @param data    JSON payload object for the GUI.
	 */
	void send_event(const std::string& typeId, const nlohmann::json& data);

private:
	std::unique_ptr<gmDispatch::IpSocketChannel> _channel;  ///< Lazy TCP client.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_GUIBRIDGE_HPP
