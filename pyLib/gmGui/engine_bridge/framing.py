"""Wire protocol: 4-byte big-endian length prefix + UTF-8 JSON payload.

Format of each TCP frame::

    [ uint32_t payload_length — big-endian, 4 bytes ]
    [ payload_length bytes   — UTF-8 encoded JSON   ]

This format matches the C++ ``IpSocketChannel`` wire protocol exactly
(see ``gmDispatch/channels/IpSocketChannel.cpp``).
"""
from __future__ import annotations

import socket
import struct


def send_frame(sock: socket.socket, payload: str) -> None:
    """Encodes *payload* as UTF-8 and sends it as a length-prefixed TCP frame.

    The frame layout is identical to the C++ ``IpSocketChannel``::

        [ 4 bytes: uint32_t payload length, big-endian ]
        [ N bytes: UTF-8 encoded payload               ]

    Args:
        sock:    Connected TCP socket.
        payload: JSON string to transmit.

    Raises:
        OSError: If the underlying ``sendall`` fails (broken pipe, reset, etc.).
    """
    data: bytes = payload.encode("utf-8")
    frame: bytes = struct.pack(">I", len(data)) + data
    sock.sendall(frame)


def recv_frame(sock: socket.socket) -> str:
    """Reads one length-prefixed frame and returns the decoded UTF-8 string.

    Blocks until the full frame has arrived.

    Args:
        sock: Connected TCP socket.

    Returns:
        The decoded UTF-8 payload string.

    Raises:
        ConnectionError: If the socket closes before the full frame is received.
        socket.timeout:  If the socket has a timeout set and it fires mid-read
                         (propagates from ``_recv_exact``).
        OSError:         On other socket-level errors.
    """
    raw_len: bytes = _recv_exact(sock, 4)
    length: int = struct.unpack(">I", raw_len)[0]
    return _recv_exact(sock, length).decode("utf-8")


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    """Reads exactly *n* bytes from *sock*, looping until all bytes arrive.

    Args:
        sock: Connected TCP socket.
        n:    Number of bytes to read.  If 0, returns immediately with ``b""``.

    Returns:
        Exactly *n* bytes.

    Raises:
        ConnectionError: If the remote end closes the connection before
                         all *n* bytes have been delivered.
        socket.timeout:  If the socket has a timeout set and it fires
                         (caller is responsible for handling this).
        OSError:         On other socket-level errors.
    """
    if n == 0:
        return b""
    buf: bytes = b""
    while len(buf) < n:
        chunk: bytes = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError(
                f"Socket closed after {len(buf)} of {n} bytes"
            )
        buf += chunk
    return buf
