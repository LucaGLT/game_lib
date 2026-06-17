"""Wire protocol: 4-byte big-endian length prefix + UTF-8 JSON payload.

Format of each TCP frame::

    [ uint32_t payload_length — big-endian, 4 bytes ]
    [ payload_length bytes   — UTF-8 encoded JSON   ]

This format matches the C++ IpSocketChannel wire protocol exactly.
Full implementation: Phase 2.
"""
from __future__ import annotations

import socket
import struct


def send_frame(sock: socket.socket, payload: str) -> None:
    """Encodes *payload* as UTF-8 and sends it as a length-prefixed TCP frame.

    Args:
        sock:    Connected TCP socket.
        payload: JSON string to transmit.

    Raises:
        NotImplementedError: stub — implemented in Phase 2.
    """
    # TODO: Phase 2 — encode payload, struct.pack(">I", len), sock.sendall()
    raise NotImplementedError("framing.send_frame: implemented in Phase 2")


def recv_frame(sock: socket.socket) -> str:
    """Reads one length-prefixed frame and returns the decoded UTF-8 string.

    Args:
        sock: Connected TCP socket.

    Returns:
        The decoded UTF-8 payload string.

    Raises:
        NotImplementedError: stub — implemented in Phase 2.
        ConnectionError:     (Phase 2) if the socket closes before the frame completes.
    """
    # TODO: Phase 2 — _recv_exact(sock, 4), struct.unpack(">I"), _recv_exact(sock, n)
    raise NotImplementedError("framing.recv_frame: implemented in Phase 2")


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    """Reads exactly *n* bytes from *sock*, blocking until all bytes arrive.

    Args:
        sock: Connected TCP socket.
        n:    Number of bytes to read.

    Returns:
        Exactly *n* bytes.

    Raises:
        NotImplementedError: stub — implemented in Phase 2.
        ConnectionError:     (Phase 2) if the socket closes before *n* bytes arrive.
    """
    # TODO: Phase 2 — loop sock.recv(n - len(buf)) until len(buf) == n
    raise NotImplementedError("framing._recv_exact: implemented in Phase 2")
