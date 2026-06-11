#ifndef GMDISPATCH_IPSOCKETCHANNEL_HPP
#define GMDISPATCH_IPSOCKETCHANNEL_HPP

/**
 * @file channels/IpSocketChannel.hpp
 * @brief TCP client channel — platform-agnostic interface.
 *
 * ### Status
 * The interface and connection lifecycle are fully defined here.
 * The OS-specific socket implementation lives in @c IpSocketChannel.cpp
 * which uses:
 * - **Windows**: Winsock2 (@c \<winsock2.h\>)
 * - **POSIX** (Linux / macOS): BSD sockets (@c \<sys/socket.h\>)
 *
 * ### Design
 * IpSocketChannel is a **TCP client** — it connects to a remote host:port
 * on the first @ref send() call (lazy connect) and keeps the connection open.
 * Each envelope is serialized and sent as a length-prefixed frame:
 * @code
 *   [ uint32_t length (network byte order) ][ payload bytes ]
 * @endcode
 *
 * If the connection is lost, the next @ref send() attempts a single reconnect.
 *
 * ### Future extensions
 * - TLS/DTLS transport
 * - UDP (datagram) variant
 * - Server-side (listen + accept) variant for multiple consumers
 */

#include "../IChannel.hpp"
#include "../ISerializer.hpp"

#include <memory>
#include <string>

namespace GmDispatch {

/**
 * @brief TCP client channel that sends serialized envelopes to a remote endpoint.
 */
class IpSocketChannel : public IChannel {
public:
    /**
     * @brief Constructs an IpSocketChannel.
     *
     * The channel is not connected until the first @ref send() call
     * (lazy connect).
     *
     * @param host        Remote hostname or IPv4 address.
     * @param port        Remote TCP port (1–65535).
     * @param channelName Optional name for targeted delivery.  Empty = anonymous.
     * @param serializer  Optional serializer.  Defaults to @ref JsonSerializer.
     */
    explicit IpSocketChannel(const std::string&           host,
                             uint16_t                     port,
                             const std::string&           channelName = "",
                             std::unique_ptr<ISerializer> serializer  = nullptr);

    /**
     * @brief Closes the socket connection if open.
     */
    ~IpSocketChannel();

    IpSocketChannel(const IpSocketChannel&)            = delete;
    IpSocketChannel& operator=(const IpSocketChannel&) = delete;

    /// @brief Returns the channel name provided at construction.
    std::string name() const override;

    /**
     * @brief Serializes @p envelope and sends it over TCP.
     *
     * Connects lazily on the first call.  On send failure, closes the socket
     * and throws @c std::runtime_error.
     *
     * @param envelope The dispatch event to transmit.
     * @throws std::runtime_error on connection or send failure.
     */
    void send(const Envelope& envelope) override;

    /**
     * @brief Flushes any OS-level write buffers.
     *
     * Sets @c TCP_NODELAY for the duration of the flush (optional — may
     * be a no-op on platforms where it is not needed).
     */
    void flush() override;

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /// @brief Returns the remote host passed at construction.
    const std::string& host() const;

    /// @brief Returns the remote port passed at construction.
    uint16_t port() const;

    /// @brief Returns @c true if the socket is currently connected.
    bool isConnected() const;

private:
    /// Establishes the TCP connection.  Throws on failure.
    void connect();

    /// Closes the socket silently.
    void closeSocket();

    std::string                  name_;
    std::string                  host_;
    uint16_t                     port_;
    std::unique_ptr<ISerializer> serializer_;

    // OS socket handle — stored as int (POSIX fd / SOCKET on Windows).
    // -1 / INVALID_SOCKET when not connected.
    int socketFd_{-1};
};

} // namespace GmDispatch

#endif // GMDISPATCH_IPSOCKETCHANNEL_HPP
