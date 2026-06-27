/**
 * @file bridge/EldhomGuiBridge.cpp
 * @brief Implementation of outbound event bridge for Eldhom GUI.
 */

#include "GAME/Eldhom/CoreEngine/bridge/EldhomGuiBridge.hpp"

#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/GmDispatchError.hpp"

#include <iostream>

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
		std::cout << "[EldhomGuiBridge] 📤 Sending to GUI: " << typeId << " (" 
		          << env.headers["data"].size() << " bytes)\n" << std::flush;
		_channel->send(env);
		std::cout << "[EldhomGuiBridge] ✓ Event sent to GUI.\n" << std::flush;
	}
	catch (const gmDispatch::EDispatchError& e)
	{
		// Keep engine alive headless: drop event if GUI is not reachable.
		std::cerr << "[EldhomGuiBridge] ✗ Failed to send event: " << e.what() << "\n";
	}
}

} // namespace eldhom
