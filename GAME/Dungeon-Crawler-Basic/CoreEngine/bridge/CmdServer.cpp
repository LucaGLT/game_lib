/**
 * @file bridge/CmdServer.cpp
 * @brief Stub implementation of CmdServer.
 *
 * Real Winsock2/POSIX socket accept/receive loop with length-prefix framing
 * will be introduced in FASE B, mirroring the Tic-Tac-Toe CmdServer pattern.
 */

#include "bridge/CmdServer.hpp"

#include <iostream>

namespace gmDungeonBasic
{

CmdServer::CmdServer(uint16_t port, CommandHandler handler)
	: _port(port), _handler(std::move(handler))
{
	// ToBeImplemented //
}

CmdServer::~CmdServer()
{
	stop();
}

void CmdServer::start()
{
	// ToBeImplemented //
	std::cout << "[CmdServer] start() on port " << _port << " — stub.\n";
	_running = true;
}

void CmdServer::stop()
{
	// ToBeImplemented //
	if (_running)
	{
		_running = false;
		if (_thread.joinable())
		{
			_thread.join();
		}
	}
}

void CmdServer::run()
{
	// ToBeImplemented //
}

} // namespace gmDungeonBasic
