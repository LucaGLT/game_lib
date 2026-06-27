/**
 * @file bridge/EldhomCmdServer.cpp
 * @brief Implementation of inbound command server for Eldhom GUI.
 */

#include "GAME/Eldhom/CoreEngine/bridge/EldhomCmdServer.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
static const SocketHandle INVALID_SOCK = INVALID_SOCKET;
static void close_native_socket(SocketHandle s) { ::closesocket(s); }
static bool platform_init()
{
	static bool init = false;
	if (!init)
	{
		WSADATA wd;
		if (::WSAStartup(MAKEWORD(2, 2), &wd) != 0) { return false; }
		init = true;
	}
	return true;
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using SocketHandle = int;
static const SocketHandle INVALID_SOCK = -1;
static void close_native_socket(SocketHandle s) { ::close(s); }
static bool platform_init() { return true; }
#endif

namespace {

bool recv_exact(SocketHandle sock, char* buffer, std::size_t n)
{
	std::size_t received = 0;
	while (received < n)
	{
		int chunk = ::recv(sock, buffer + received,
		                   static_cast<int>(n - received), 0);
		if (chunk <= 0) { return false; }
		received += static_cast<std::size_t>(chunk);
	}
	return true;
}

bool recv_frame(SocketHandle sock, std::string& out)
{
	uint32_t len_be = 0;
	if (!recv_exact(sock, reinterpret_cast<char*>(&len_be), sizeof(len_be)))
	{
		return false;
	}
	const uint32_t len = ntohl(len_be);
	if (len == 0) { out.clear(); return true; }

	std::vector<char> payload(len);
	if (!recv_exact(sock, payload.data(), len)) { return false; }

	out.assign(payload.data(), len);
	return true;
}

} // namespace

namespace eldhom {

EldhomCmdServer::EldhomCmdServer(uint16_t port, CommandHandler handler)
	: _port(port), _handler(std::move(handler))
{
}

EldhomCmdServer::~EldhomCmdServer()
{
	stop();
}

void EldhomCmdServer::start()
{
	if (_running.exchange(true)) { return; }
	_thread = std::thread(&EldhomCmdServer::run, this);
}

void EldhomCmdServer::stop()
{
	if (!_running.exchange(false)) { return; }

	if (_server_fd != static_cast<int>(INVALID_SOCK))
	{
		close_native_socket(static_cast<SocketHandle>(_server_fd));
		_server_fd = static_cast<int>(INVALID_SOCK);
	}

	if (_thread.joinable()) { _thread.join(); }
}

void EldhomCmdServer::run()
{
	if (!platform_init()) { _running = false; return; }

	SocketHandle server = ::socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCK) { _running = false; return; }

	_server_fd = static_cast<int>(server);

	int opt = 1;
	::setsockopt(server, SOL_SOCKET, SO_REUSEADDR,
	             reinterpret_cast<const char*>(&opt), sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port        = htons(_port);

	if (::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
	{
		close_native_socket(server);
		_server_fd = static_cast<int>(INVALID_SOCK);
		_running   = false;
		return;
	}

	if (::listen(server, 1) != 0)
	{
		close_native_socket(server);
		_server_fd = static_cast<int>(INVALID_SOCK);
		_running   = false;
		return;
	}

	while (_running)
	{
		SocketHandle client = ::accept(server, nullptr, nullptr);
		if (client == INVALID_SOCK) { break; }

		std::string raw;
		while (_running && recv_frame(client, raw))
		{
			try
			{
				const nlohmann::json msg = nlohmann::json::parse(raw);
				const std::string type_id = msg.value("typeId", std::string{});
				const nlohmann::json data = msg.contains("data")
					? msg.at("data")
					: nlohmann::json::object();
				if (!type_id.empty() && _handler)
				{
					_handler(type_id, data);
				}
			}
			catch (const nlohmann::json::exception&)
			{
				// Ignore malformed frames.
			}
		}

		close_native_socket(client);
	}
}

} // namespace eldhom
