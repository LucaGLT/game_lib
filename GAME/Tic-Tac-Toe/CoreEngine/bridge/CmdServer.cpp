/**
 * @file bridge/CmdServer.cpp
 * @brief OS-specific implementation of the inbound GUI command server.
 *
 * Platform detection:
 *   _WIN32   → Winsock2
 *   otherwise → POSIX BSD sockets
 *
 * Wire format (identical to gmDispatch::IpSocketChannel):
 *   [ 4 bytes: uint32_t payload length, network byte order ]
 *   [ N bytes: UTF-8 encoded JSON command                  ]
 */

#include "CmdServer.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

// ── Platform includes ─────────────────────────────────────────────────────────

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle                     = SOCKET;
static const SocketHandle INVALID_SOCK = INVALID_SOCKET;
static void               closeNativeSocket(SocketHandle s)
{
	::closesocket(s);
}
static bool platformInit()
{
	static bool init = false;
	if (!init)
	{
		WSADATA wd;
		if (::WSAStartup(MAKEWORD(2, 2), &wd) != 0)
		{
			return false;
		}
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
using SocketHandle                     = int;
static const SocketHandle INVALID_SOCK = -1;
static void               closeNativeSocket(SocketHandle s)
{
	::close(s);
}
static bool platformInit()
{
	return true;
}
#endif

namespace gmTris
{

namespace
{

/// @brief Reads exactly @p n bytes from @p sock. Returns false on close/error.
bool recv_exact(SocketHandle sock, char* buffer, std::size_t n)
{
	std::size_t received = 0;
	while (received < n)
	{
		int chunk = ::recv(sock,
		                   buffer + received,
		                   static_cast<int>(n - received),
		                   0);
		if (chunk <= 0)
		{
			return false;
		}
		received += static_cast<std::size_t>(chunk);
	}
	return true;
}

/// @brief Reads one length-prefixed frame into @p out. Returns false on error.
bool recv_frame(SocketHandle sock, std::string& out)
{
	uint32_t lenBe = 0;
	if (!recv_exact(sock, reinterpret_cast<char*>(&lenBe), sizeof(lenBe)))
	{
		return false;
	}

	const uint32_t len = ntohl(lenBe);
	if (len == 0)
	{
		out.clear();
		return true;
	}

	std::vector<char> payload(len);
	if (!recv_exact(sock, payload.data(), len))
	{
		return false;
	}

	out.assign(payload.data(), len);
	return true;
}

} // namespace

CmdServer::CmdServer(uint16_t port, CommandHandler handler)
    : _port(port), _handler(std::move(handler))
{
}

CmdServer::~CmdServer()
{
	stop();
}

void CmdServer::start()
{
	if (_running.exchange(true))
	{
		return; // already running
	}
	_thread = std::thread(&CmdServer::run, this);
}

void CmdServer::stop()
{
	if (!_running.exchange(false))
	{
		return; // already stopped
	}
	if (_server_fd != static_cast<int>(INVALID_SOCK))
	{
		closeNativeSocket(static_cast<SocketHandle>(_server_fd));
		_server_fd = static_cast<int>(INVALID_SOCK);
	}
	if (_thread.joinable())
	{
		_thread.join();
	}
}

void CmdServer::run()
{
	if (!platformInit())
	{
		_running = false;
		return;
	}

	SocketHandle server = ::socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCK)
	{
		_running = false;
		return;
	}
	_server_fd = static_cast<int>(server);

	int opt = 1;
	::setsockopt(server,
	            SOL_SOCKET,
	            SO_REUSEADDR,
	            reinterpret_cast<const char*>(&opt),
	            sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port        = htons(_port);

	if (::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
	{
		closeNativeSocket(server);
		_server_fd = static_cast<int>(INVALID_SOCK);
		_running   = false;
		return;
	}

	if (::listen(server, 1) != 0)
	{
		closeNativeSocket(server);
		_server_fd = static_cast<int>(INVALID_SOCK);
		_running   = false;
		return;
	}

	while (_running)
	{
		SocketHandle client = ::accept(server, nullptr, nullptr);
		if (client == INVALID_SOCK)
		{
			break; // server socket closed by stop()
		}

		std::string raw;
		while (_running && recv_frame(client, raw))
		{
			try
			{
				const nlohmann::json msg    = nlohmann::json::parse(raw);
				const std::string    typeId = msg.value("typeId", "");
				const nlohmann::json data    = msg.contains("data")
				                                   ? msg.at("data")
				                                   : nlohmann::json::object();
				if (!typeId.empty() && _handler)
				{
					_handler(typeId, data);
				}
			}
			catch (const nlohmann::json::exception&)
			{
				// Malformed frame: ignore and keep reading the next one.
			}
		}

		closeNativeSocket(client);
	}
}

} // namespace gmTris
