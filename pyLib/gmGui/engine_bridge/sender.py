"""EngineSender — TCP client that sends GUI commands to the C++ engine on port 9001.

The C++ side listens with a CmdServer thread on port 9001.
Each command is serialised as a JSON string and sent as a length-prefixed frame
using the same wire protocol as IpSocketChannel.

Full implementation: Phase 2.
Phase 1 stub: send_command() is a silent no-op.
"""
from __future__ import annotations

import socket


class EngineSender:
    """Lazy-connecting TCP client that transmits command frames to the C++ engine.

    Usage::

        sender = EngineSender()
        sender.send_command("gmFlow.session.pause", {})
        sender.close()

    The connection is established on the first :meth:`send_command` call
    (lazy connect), matching the pattern of ``gmDispatch::IpSocketChannel``.
    """

    def __init__(self, host: str = "127.0.0.1", port: int = 9001) -> None:
        self._host: str = host
        self._port: int = port
        self._sock: socket.socket | None = None

    def send_command(self, type_id: str, data: dict) -> None:
        """Serialises and sends one command frame to the C++ CmdServer.

        Args:
            type_id: gmDispatch typeId string (e.g. ``"gmFlow.session.pause"``).
            data:    JSON-serialisable dict carrying the command payload.

        Phase 1 stub: no-op (connection is not attempted).
        Full implementation: Phase 2.
        """
        # TODO: Phase 2 — _ensure_connected(), framing.send_frame()
        pass

    def close(self) -> None:
        """Closes the TCP connection if it is currently open."""
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def _ensure_connected(self) -> None:
        """Opens a TCP connection to the C++ CmdServer if not already connected.

        Phase 1 stub: not called.
        Full implementation: Phase 2.
        """
        # TODO: Phase 2 — socket.connect((host, port))
        pass
