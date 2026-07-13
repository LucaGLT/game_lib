/**
 * @file bridge/GuiBridge.cpp
 * @brief Implementation of the outbound GUI event channel.
 */

#include "GuiBridge.hpp"

#include "gmDispatch/Envelope.hpp"
#include "gmDispatch/GmDispatchError.hpp"

namespace gmTris
{

GuiBridge::GuiBridge(const std::string& host, uint16_t port)
    : _channel(std::make_unique<gmDispatch::IpSocketChannel>(host, port, "gui"))
{
}

void GuiBridge::send_event(const std::string& typeId, const nlohmann::json& data)
{
	gmDispatch::Envelope env;
	env.typeId          = typeId;
	env.source          = "CoreEngine";
	env.headers["data"] = data.dump();

	try
	{
		_channel->send(env);
	}
	catch (const gmDispatch::EDispatchError&)
	{
		// GUI not connected yet or transport error: drop the event silently so
		// the engine keeps running headless. The next event will retry connect.
	}
}

} // namespace gmTris
