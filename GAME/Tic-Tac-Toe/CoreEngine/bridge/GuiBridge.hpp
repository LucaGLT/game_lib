#ifndef GMTRIS_GUIBRIDGE_HPP
#define GMTRIS_GUIBRIDGE_HPP

/**
 * @file bridge/GuiBridge.hpp
 * @brief Outbound event channel: serialises engine events towards the GUI.
 *
 * The bridge wraps a @ref gmDispatch::IpSocketChannel TCP client that connects
 * (lazily, on first send) to the GUI event server on @ref gmTris::ports::EVENTS.
 * Each event is delivered as an @ref gmDispatch::Envelope whose
 * @c headers["data"] field carries the JSON payload as a string — the exact
 * shape expected by the PySide6 @c EngineReceiver.
 */

#include "gmDispatch/channels/IpSocketChannel.hpp"
#include "gmSave/json.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace gmTris
{

/**
 * @class GuiBridge
 * @brief Sends engine events to the GUI over a length-prefixed JSON TCP frame.
 */
class GuiBridge
{
  public:
	/**
	 * @brief Constructs the bridge towards a GUI event server.
	 *
	 * @param host Remote host of the GUI event server (default loopback).
	 * @param port TCP port of the GUI event server.
	 */
	explicit GuiBridge(const std::string& host = "127.0.0.1",
	                   uint16_t           port = 9000);

	/**
	 * @brief Sends one event to the GUI.
	 *
	 * Builds an envelope with @c typeId, @c source = "CoreEngine" and
	 * @c headers["data"] = @p data.dump(), then transmits it. Connection and
	 * send failures are swallowed so a missing GUI never crashes the engine.
	 *
	 * @param typeId Event type identifier (see @ref gmTris::event_id).
	 * @param data   JSON payload for the GUI.
	 */
	void send_event(const std::string& typeId, const nlohmann::json& data);

  private:
	std::unique_ptr<gmDispatch::IpSocketChannel> _channel;
};

} // namespace gmTris

#endif // GMTRIS_GUIBRIDGE_HPP
