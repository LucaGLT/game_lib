/**
 * @file channels/IpSocketChannel.cpp
 * @brief TCP client channel — OS-specific implementation.
 *
 * Platform detection:
 *   _WIN32   → Winsock2
 *   otherwise → POSIX BSD sockets
 *
 * Wire format: each message is sent as a length-prefixed frame:
 *   [ 4 bytes: uint32_t payload length, network byte order ]
 *   [ N bytes: UTF-8 serialized envelope                  ]
 */

#include "IpSocketChannel.hpp"
#include "../serializers/JsonSerializer.hpp"

#include <stdexcept>
#include <string>
#include <cstring>

// ── Platform includes ─────────────────────────────────────────────────────────

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Link against Ws2_32.lib
    // In CMake: target_link_libraries(mytarget PRIVATE ws2_32)
    using SocketHandle = SOCKET;
    static const SocketHandle INVALID_SOCK = INVALID_SOCKET;
    static void closeNativeSocket(SocketHandle s) { ::closesocket(s); }
    static void platformInit() {
        static bool init = false;
        if (!init) {
            WSADATA wd;
            if (::WSAStartup(MAKEWORD(2,2), &wd) != 0) {
                throw std::runtime_error("IpSocketChannel: WSAStartup failed");
            }
            init = true;
        }
    }
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    using SocketHandle = int;
    static const SocketHandle INVALID_SOCK = -1;
    static void closeNativeSocket(SocketHandle s) { ::close(s); }
    static void platformInit() {} // no-op on POSIX
#endif

namespace GmDispatch {

// ── Constructor / destructor ──────────────────────────────────────────────────

IpSocketChannel::IpSocketChannel(const std::string&           host,
                                 uint16_t                     port,
                                 const std::string&           channelName,
                                 std::unique_ptr<ISerializer> serializer)
    : name_(channelName)
    , host_(host)
    , port_(port)
    , serializer_(std::move(serializer))
{
    if (!serializer_) {
        serializer_ = std::make_unique<JsonSerializer>();
    }
    platformInit();
}

IpSocketChannel::~IpSocketChannel()
{
    closeSocket();
}

// ── IChannel interface ────────────────────────────────────────────────────────

std::string IpSocketChannel::name() const
{
    return name_;
}

void IpSocketChannel::send(const Envelope& envelope)
{
    if (socketFd_ == static_cast<int>(INVALID_SOCK)) {
        connect();
    }

    const std::string payload = serializer_->serialize(envelope);

    // Length-prefixed frame: 4-byte big-endian length + payload
    const uint32_t len   = static_cast<uint32_t>(payload.size());
    const uint32_t lenBe = htonl(len);

    // Send length prefix
    const char* lenPtr = reinterpret_cast<const char*>(&lenBe);
    std::size_t sent   = 0;
    while (sent < sizeof(lenBe)) {
        int n = ::send(static_cast<SocketHandle>(socketFd_),
                       lenPtr + sent,
                       static_cast<int>(sizeof(lenBe) - sent), 0);
        if (n <= 0) {
            closeSocket();
            throw std::runtime_error(
                "IpSocketChannel: send() failed while writing length prefix");
        }
        sent += static_cast<std::size_t>(n);
    }

    // Send payload
    sent = 0;
    while (sent < payload.size()) {
        int n = ::send(static_cast<SocketHandle>(socketFd_),
                       payload.data() + sent,
                       static_cast<int>(payload.size() - sent), 0);
        if (n <= 0) {
            closeSocket();
            throw std::runtime_error(
                "IpSocketChannel: send() failed while writing payload");
        }
        sent += static_cast<std::size_t>(n);
    }
}

void IpSocketChannel::flush()
{
    // TCP_NODELAY could be toggled here; for V1 this is a no-op.
    // The kernel will flush on the next scheduled send.
}

// ── Diagnostics ───────────────────────────────────────────────────────────────

const std::string& IpSocketChannel::host() const { return host_; }
uint16_t           IpSocketChannel::port() const  { return port_; }
bool IpSocketChannel::isConnected() const
{
    return socketFd_ != static_cast<int>(INVALID_SOCK);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void IpSocketChannel::connect()
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const std::string portStr = std::to_string(port_);
    addrinfo* result = nullptr;

    if (::getaddrinfo(host_.c_str(), portStr.c_str(), &hints, &result) != 0) {
        throw std::runtime_error(
            "IpSocketChannel: cannot resolve host: " + host_);
    }

    SocketHandle sock = INVALID_SOCK;
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCK) continue;

        if (::connect(sock, rp->ai_addr,
                      static_cast<int>(rp->ai_addrlen)) == 0) {
            break; // connected
        }
        closeNativeSocket(sock);
        sock = INVALID_SOCK;
    }
    ::freeaddrinfo(result);

    if (sock == INVALID_SOCK) {
        throw std::runtime_error(
            "IpSocketChannel: cannot connect to " + host_ + ":"
            + std::to_string(port_));
    }

    socketFd_ = static_cast<int>(sock);
}

void IpSocketChannel::closeSocket()
{
    if (socketFd_ != static_cast<int>(INVALID_SOCK)) {
        closeNativeSocket(static_cast<SocketHandle>(socketFd_));
        socketFd_ = static_cast<int>(INVALID_SOCK);
    }
}

} // namespace GmDispatch
