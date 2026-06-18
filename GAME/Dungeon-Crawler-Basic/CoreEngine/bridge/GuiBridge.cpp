/**
 * @file bridge/GuiBridge.cpp
 * @brief Implementation of outbound event bridge for Dungeon GUI.
 */

#include "bridge/GuiBridge.hpp"

#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/GmDispatchError.hpp"

namespace gmDungeonBasic
{

GuiBridge::GuiBridge(const std::string& host, uint16_t port)
	: _channel(std::make_unique<gmDispatch::IpSocketChannel>(host, port, "dungeon_gui"))
{
}

void GuiBridge::send_event(const std::string& typeId, const nlohmann::json& data)
{
	gmDispatch::Envelope env;
	env.typeId = typeId;
	env.source = "DungeonCore";
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

} // namespace gmDungeonBasic
