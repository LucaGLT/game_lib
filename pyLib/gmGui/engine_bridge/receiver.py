"""EngineReceiver — QThread that listens for C++ engine events on TCP port 9000.

The C++ side connects using ``gmDispatch::IpSocketChannel("127.0.0.1", 9000)``.
Each arriving frame is deserialised from JSON and emitted as a Qt Signal so
that all widget updates happen on the Qt main thread.
"""
from __future__ import annotations

import json
import socket

from PySide6.QtCore import QThread, Signal

from .framing import recv_frame


class EngineReceiver(QThread):
    """TCP server on port 9000 that receives engine events and emits Qt Signals.

    Lifecycle::

        receiver = EngineReceiver()
        receiver.envelope_received.connect(window._on_envelope)
        receiver.start()   # binds port, waits for C++ client
        ...
        receiver.stop()    # graceful shutdown, at most ~1 s wait

    The server socket uses a 1-second ``accept()`` timeout and the client
    socket also uses a 1-second ``recv()`` timeout.  This lets ``stop()``
    interrupt the thread within one timeout cycle without needing to close
    sockets from the outside.

    Envelope normalisation
    ----------------------
    The C++ ``JsonSerializer`` puts real payload data in
    ``headers["data"]`` as a JSON string (see bridge design).  This class
    parses that field and stores it as ``msg["data"]`` (dict) before emitting
    so that modules always receive a consistent structure::

        {
            "typeId":  str,
            "source":  str,
            "time":    str,   # ISO-8601 UTC timestamp
            "data":    dict,  # deserialised from headers["data"], or {}
            ...               # all other original fields preserved
        }

    Signals:
        envelope_received: Emitted for each successfully parsed JSON frame.
        connection_lost:   Emitted when the C++ client disconnects.
    """

    envelope_received: Signal = Signal(dict)
    connection_lost: Signal = Signal()

    # Timeout in seconds for both accept() and recv().
    # Controls the maximum delay between stop() and thread exit.
    _TIMEOUT: float = 0.1

    def __init__(self, host: str = "127.0.0.1", port: int = 9000) -> None:
        super().__init__()
        self._host: str = host
        self._port: int = port
        self._running: bool = True  # set to False by stop()

    def run(self) -> None:
        """TCP server loop: accept, read frames, emit Signals.

        Thread entry point — do not call directly; use ``start()``.
        """
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self._host, self._port))
        srv.listen(1)
        srv.settimeout(self._TIMEOUT)

        client: socket.socket | None = None
        try:
            while self._running:
                # ── Phase A: accept a client if none is connected ─────────────
                if client is None:
                    try:
                        client, _ = srv.accept()
                        client.settimeout(self._TIMEOUT)
                    except socket.timeout:
                        continue  # no C++ client yet; check _running again
                    except OSError:
                        break  # server socket closed externally

                # ── Phase B: read one frame ───────────────────────────────────
                try:
                    raw: str = recv_frame(client)
                except socket.timeout:
                    continue  # no data in this window; check _running again
                except (ConnectionError, OSError):
                    self.connection_lost.emit()
                    _close_socket(client)
                    client = None
                    continue

                # ── Phase C: parse JSON ───────────────────────────────────────
                try:
                    msg: dict = json.loads(raw)
                except (json.JSONDecodeError, ValueError):
                    continue  # malformed frame; ignore and keep reading

                # ── Phase D: normalise msg["data"] ────────────────────────────
                headers = msg.get("headers", {})
                if isinstance(headers, dict) and "data" in headers:
                    try:
                        msg["data"] = json.loads(headers["data"])
                    except (json.JSONDecodeError, TypeError, ValueError):
                        msg["data"] = headers["data"]
                elif "data" not in msg:
                    msg["data"] = {}

                self.envelope_received.emit(msg)

        finally:
            _close_socket(client)
            _close_socket(srv)

    def stop(self) -> None:
        """Signals the run() loop to exit and waits for the thread to finish."""
        self._running = False
        self.wait()


# ── Module-level helper ────────────────────────────────────────────────────────

def _close_socket(sock: socket.socket | None) -> None:
    """Closes *sock* silently, ignoring any OSError."""
    if sock is not None:
        try:
            sock.close()
        except OSError:
            pass
