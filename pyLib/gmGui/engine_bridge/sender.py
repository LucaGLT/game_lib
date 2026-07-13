"""EngineSender — TCP client that sends GUI commands to the C++ engine on port 9001.

The C++ side listens with a ``CmdServer`` thread on port 9001.
Each command is serialised as a JSON string and sent as a length-prefixed frame
using the same wire protocol as ``IpSocketChannel``.
"""
from __future__ import annotations

import json
import socket
from datetime import datetime

from .framing import send_frame


def _ts() -> str:
    """Returns the current local time as ``HH:MM:SS.mmm`` for log prefixes."""
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


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

    def __init__(self, host: str = "127.0.0.1", port: int = 9001,
                 connect_timeout: float = 3.0) -> None:
        self._host: str = host
        self._port: int = port
        self._connect_timeout: float = connect_timeout
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
            frame_data = json.dumps(msg)
            print(f"[EngineSender {_ts()}] \U0001f4e4 Sending command -> {self._host}:{self._port}",
                  flush=True)
            print(f"[EngineSender {_ts()}]    {frame_data}", flush=True)
            send_frame(self._sock, frame_data)  # type: ignore[arg-type]
            print(f"[EngineSender {_ts()}] \u2713 Command sent ({len(frame_data)} byte).",
                  flush=True)
        except OSError as e:
            # Engine not reachable; reset so the next call retries the connection.
            print(f"[EngineSender {_ts()}] \u2717 Connection error: {e}", flush=True)
            self.close()

    def connect(self) -> bool:
        """Eagerly attempts to open the TCP connection.

        Returns True on success, False if the engine is not yet reachable.
        Unlike :meth:`send_command`, this method does not raise and does not
        log to avoid spurious error output during startup probing.
        """
        try:
            self._ensure_connected()
            return True
        except OSError:
            return False

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

        Applies *connect_timeout* so the call does not block indefinitely
        when the engine has not yet started.

        Raises:
            OSError: If the connection cannot be established within the timeout.
        """
        if self._sock is None:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(self._connect_timeout)
            try:
                sock.connect((self._host, self._port))
            except socket.timeout:
                sock.close()
                raise OSError(
                    f"Connection to {self._host}:{self._port} timed out "
                    f"after {self._connect_timeout}s"
                )
            sock.settimeout(None)   # blocking mode for data transfer
            self._sock = sock
