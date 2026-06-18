/**
 * @file bridge/GuiBridge.cpp
 * @brief Stub implementation of GuiBridge.
 *
 * Real IpSocketChannel integration (lazy connect, envelope build, send) will
 * be introduced in FASE B, mirroring the Tic-Tac-Toe GuiBridge pattern.
 */

#include "bridge/GuiBridge.hpp"

#include <iostream>

namespace gmDungeonBasic
{

GuiBridge::GuiBridge(const std::string& host, uint16_t port)
{
	(void)host;
	(void)port;
	// ToBeImplemented //
}

void GuiBridge::send_event(const std::string& typeId, const nlohmann::json& data)
{
	(void)typeId;
	(void)data;
	// ToBeImplemented //
	std::cout << "[GuiBridge] send_event(" << typeId << ") — stub.\n";
}

} // namespace gmDungeonBasic
