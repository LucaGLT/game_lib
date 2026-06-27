/**
 * @file bridge/EldhomGuiBridge.cpp
 * @brief Implementation of outbound event bridge for Eldhom GUI.
 */

#include "GAME/Eldhom/CoreEngine/bridge/EldhomGuiBridge.hpp"

#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/GmDispatchError.hpp"

namespace eldhom {

EldhomGuiBridge::EldhomGuiBridge(const std::string& host, uint16_t port)
	: _channel(std::make_unique<gmDispatch::IpSocketChannel>(host, port, "eldhom_gui"))
{
}

void EldhomGuiBridge::send_event(const std::string& typeId, const nlohmann::json& data)
{
	gmDispatch::Envelope env;
	env.typeId = typeId;
	env.source = "EldhomCore";
	env.headers["data"] = data.dump();

	try
	{
		_channel->send(env);
	}
	catch (const gmDispatch::EDispatchError&)
	{
		// Keep engine alive headless: drop event if GUI is not reachable.
	}
}

} // namespace eldhom
