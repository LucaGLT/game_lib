/**
 * @file tests/test_cmdserver.cpp
 * @brief Robustness tests for the inbound command bridge (CmdServer).
 *
 * Exercises the real loopback path: a CmdServer is started on a test port and a
 * client socket sends length-prefixed JSON frames, asserting that:
 *   - a well-formed command frame invokes the handler with the right typeId/data,
 *   - a frame carrying malformed JSON is ignored without crashing the server,
 *     and the connection keeps working for the next valid frame,
 *   - a frame missing the typeId field does not invoke the handler,
 *   - after a client disconnects, the server returns to accept() and serves a
 *     freshly connected client.
 *
 * This mirrors the Phase 4 requirement "Test bridge parsing/frame handling
 * (messaggi validi e corrotti)" against the actual networking code.
 */

#include "bridge/CmdServer.hpp"
#include "tests/test_harness.hpp"

#include "gmSave/json.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle                  = int;
static const SocketHandle INVALID_SOCKET_VALUE = -1;
#endif

using namespace gmTris;
using gmtris_test::check;

namespace
{

#if defined(_WIN32)
const SocketHandle BAD_SOCKET = INVALID_SOCKET;
void               close_socket(SocketHandle s)
{
	::closesocket(s);
}
void platform_init()
{
	WSADATA wd;
	::WSAStartup(MAKEWORD(2, 2), &wd);
}
#else
const SocketHandle BAD_SOCKET = INVALID_SOCKET_VALUE;
void               close_socket(SocketHandle s)
{
	::close(s);
}
void platform_init()
{
}
#endif

/// @brief Thread-safe record of the commands delivered to the handler.
class CommandLog
{
  public:
	void add(const std::string& typeId, const nlohmann::json& data)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_entries.push_back({typeId, data});
	}

	std::size_t size() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _entries.size();
	}

	std::pair<std::string, nlohmann::json> at(std::size_t index) const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _entries.at(index);
	}

  private:
	mutable std::mutex                                  _mutex;
	std::vector<std::pair<std::string, nlohmann::json>> _entries;
};

/// @brief Connects a client socket to the loopback server, retrying briefly.
SocketHandle connect_client(uint16_t port)
{
	for (int attempt = 0; attempt < 100; ++attempt)
	{
		SocketHandle sock = ::socket(AF_INET, SOCK_STREAM, 0);
		if (sock == BAD_SOCKET)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port   = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
		{
			return sock;
		}
		close_socket(sock);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return BAD_SOCKET;
}

/// @brief Sends @p payload as a 4-byte big-endian length-prefixed frame.
bool send_frame(SocketHandle sock, const std::string& payload)
{
	const uint32_t len   = static_cast<uint32_t>(payload.size());
	const uint32_t lenBe = htonl(len);
	char           header[4];
	std::memcpy(header, &lenBe, sizeof(lenBe));

	if (::send(sock, header, 4, 0) != 4)
	{
		return false;
	}
	if (!payload.empty())
	{
		const int sent = ::send(sock, payload.data(),
		                        static_cast<int>(payload.size()), 0);
		if (sent != static_cast<int>(payload.size()))
		{
			return false;
		}
	}
	return true;
}

/// @brief Blocks until @p log holds at least @p expected entries or times out.
bool wait_for_count(const CommandLog& log, std::size_t expected, int timeout_ms)
{
	const int step = 10;
	for (int waited = 0; waited < timeout_ms; waited += step)
	{
		if (log.size() >= expected)
		{
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(step));
	}
	return log.size() >= expected;
}

} // namespace

int main()
{
	platform_init();

	const uint16_t port = 9071;
	CommandLog     log;

	CmdServer server(port, [&log](const std::string& typeId, const nlohmann::json& data)
	                 { log.add(typeId, data); });
	server.start();

	// ── 1. Valid command frame is decoded and dispatched ──────────────────────
	SocketHandle client = connect_client(port);
	check("client_connected", client != BAD_SOCKET);

	if (client != BAD_SOCKET)
	{
		const nlohmann::json move = {{"typeId", "gmTris.move"},
		                             {"source", "GUI"},
		                             {"data", {{"player", "X"}, {"row", 1}, {"col", 2}}}};
		send_frame(client, move.dump());

		const bool got = wait_for_count(log, 1, 2000);
		bool       ok  = got;
		if (got)
		{
			const auto entry = log.at(0);
			ok = entry.first == "gmTris.move" && entry.second.value("player", "") == "X" &&
			     entry.second.value("row", 0) == 1 && entry.second.value("col", 0) == 2;
		}
		check("valid_frame_dispatched", ok);

		// ── 2. Malformed JSON frame is ignored; server stays alive ────────────
		send_frame(client, "this is not json {{{");
		const nlohmann::json move2 = {{"typeId", "gmTris.move"},
		                              {"source", "GUI"},
		                              {"data", {{"player", "O"}, {"row", 3}, {"col", 3}}}};
		send_frame(client, move2.dump());

		const bool survived = wait_for_count(log, 2, 2000);
		bool       ok2      = survived;
		if (survived)
		{
			const auto entry = log.at(1);
			ok2 = entry.first == "gmTris.move" && entry.second.value("row", 0) == 3;
		}
		check("malformed_frame_ignored_then_recovers", ok2);

		// ── 3. Frame missing typeId does not invoke the handler ───────────────
		const std::size_t before = log.size();
		send_frame(client, nlohmann::json{{"data", {{"x", 1}}}}.dump());
		send_frame(client, nlohmann::json{{"typeId", "gmTris.new_game"},
		                                  {"data", {{"starter_mode", "fixed_x"}}}}
		                       .dump());
		const bool got3 = wait_for_count(log, before + 1, 2000);
		bool       ok3  = got3;
		if (got3)
		{
			// Exactly one new entry, and it is the new_game (not the typeId-less frame).
			ok3 = log.size() == before + 1 &&
			      log.at(before).first == "gmTris.new_game";
		}
		check("missing_typeid_skipped", ok3);

		close_socket(client);
	}

	// ── 4. Server returns to accept after a disconnect ────────────────────────
	const std::size_t before_reconnect = log.size();
	SocketHandle      client2          = connect_client(port);
	bool              ok4              = client2 != BAD_SOCKET;
	if (client2 != BAD_SOCKET)
	{
		send_frame(client2, nlohmann::json{{"typeId", "gmTris.move"},
		                                   {"source", "GUI"},
		                                   {"data", {{"player", "X"}, {"row", 2}, {"col", 2}}}}
		                        .dump());
		ok4 = wait_for_count(log, before_reconnect + 1, 2000);
		close_socket(client2);
	}
	check("server_recovers_after_disconnect", ok4);

	server.stop();
	return gmtris_test::summary("test_cmdserver");
}
