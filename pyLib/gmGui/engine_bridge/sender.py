"""EngineSender — TCP client that sends GUI commands to the C++ engine on port 9001.

The C++ side listens with a ``CmdServer`` thread on port 9001.
Each command is serialised as a JSON string and sent as a length-prefixed frame
using the same wire protocol as ``IpSocketChannel``.
"""
from __future__ import annotations

import json
import socket

from .framing import send_frame


class EngineSender:
    """Lazy-connecting TCP client that transmits command frames to the C++ engine.

    The connection to ``CmdServer`` (port 9001) is established on the first
    :meth:`send_command` call, matching the lazy-connect pattern of
    ``gmDispatch::IpSocketChannel``.

    If the engine is not running or the connection is lost, :meth:`send_command`
    closes the socket silently and resets it so the next call retries the
    connection.  This keeps the GUI resilient to engine restarts.

    Usage::

        sender = EngineSender()
        sender.send_command("gmFlow.session.pause", {})
        sender.close()
    """

    def __init__(self, host: str = "127.0.0.1", port: int = 9001) -> None:
        self._host: str = host
        self._port: int = port
        self._sock: socket.socket | None = None

    def send_command(self, type_id: str, data: dict) -> None:
        """Serialises and sends one command frame to the C++ CmdServer.

        Wire format of the JSON payload::

            {
                "typeId": "<type_id>",
                "source": "GUI",
                "data":   { ... }
            }

        On connection failure the socket is closed and reset so the next call
        will attempt to reconnect.  No exception is raised — the GUI remains
        responsive even when the engine is not running.

        Args:
            type_id: ``gmDispatch`` typeId string (e.g. ``"gmFlow.session.pause"``).
            data:    JSON-serialisable dict carrying the command payload.
        """
        try:
            self._ensure_connected()
            msg: dict = {"typeId": type_id, "source": "GUI", "data": data}
            send_frame(self._sock, json.dumps(msg))  # type: ignore[arg-type]
        except OSError:
            # Engine not reachable; reset so the next call retries the connection.
            self.close()

    def close(self) -> None:
        """Closes the TCP connection if it is currently open."""
        if self._sock is not None:
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None

    def _ensure_connected(self) -> None:
        """Opens a TCP connection to the C++ CmdServer if not already connected.

        Raises:
            OSError: If the connection cannot be established (engine not running).
        """
        if self._sock is None:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self._host, self._port))
            self._sock = sock
