"""engine_listener — plain-Python (Qt-free) TCP transport towards a gmXxx engine.

Reuses the wire format and outbound command client from
``pyLib/gmGui/engine_bridge`` as-is:

- :mod:`engine_bridge.framing`     — 4-byte big-endian length prefix + UTF-8 JSON.
- :class:`engine_bridge.sender.EngineSender` — pure-socket command client (no
  Qt dependency), reused unchanged.

``engine_bridge.receiver.EngineReceiver`` could **not** be reused as-is: it is
a ``PySide6.QtCore.QThread`` that emits Qt Signals, which requires a running
Qt event loop. :class:`EngineEventListener` below is an equivalent built on a
plain ``threading.Thread`` and a regular Python callback, so it can run
inside a Qt-less FastAPI/uvicorn process.
"""
from __future__ import annotations

import json
import socket
import sys
import threading
from collections.abc import Callable
from pathlib import Path

# ── Make the shared engine_bridge package importable ──────────────────────────
# Appended (not prepended) so this package's own modules keep priority over
# any same-named module inside pyLib/gmGui. Deliberately points at
# pyLib/gmGui itself (not pyLib) so `engine_bridge` resolves as a flat
# top-level module WITHOUT going through `gmGui/__init__.py` — that module
# eagerly imports MainWindow/PySide6, which must never load in this Qt-free
# process.
_GMGUI_DIR = Path(__file__).resolve().parents[1] / "gmGui"
if _GMGUI_DIR.is_dir() and str(_GMGUI_DIR) not in sys.path:
    sys.path.append(str(_GMGUI_DIR))

from engine_bridge.framing import recv_frame  # noqa: E402
from engine_bridge.sender import EngineSender  # noqa: E402

__all__ = ["EngineEventListener", "EngineSender"]

EnvelopeCallback = Callable[[dict], None]
ConnectionLostCallback = Callable[[], None]


class EngineEventListener:
    """TCP server that accepts one engine connection and streams parsed envelopes.

    Lifecycle::

        listener = EngineEventListener(host, port, on_envelope=handler)
        listener.bind()      # bind + listen; MUST happen before the engine
                              # subprocess is told to connect back
        listener.start()     # background thread: accept() + read loop
        ...
        listener.stop()      # graceful shutdown

    Envelope normalisation mirrors ``engine_bridge.receiver.EngineReceiver``:
    the C++ side puts real payload data in ``headers["data"]`` as a JSON
    string; this class parses that field into ``msg["data"]`` (dict) before
    invoking the callback.
    """

    _ACCEPT_TIMEOUT = 0.2
    _RECV_TIMEOUT = 0.2

    def __init__(
        self,
        host: str,
        port: int,
        on_envelope: EnvelopeCallback,
        on_connection_lost: ConnectionLostCallback | None = None,
    ) -> None:
        self._host: str = host
        self._port: int = port
        self._on_envelope: EnvelopeCallback = on_envelope
        self._on_connection_lost: ConnectionLostCallback | None = on_connection_lost
        self._srv: socket.socket | None = None
        self._thread: threading.Thread | None = None
        self._running: bool = False

    def bind(self) -> None:
        """Binds and starts listening on the event port.

        Must be called before the engine subprocess is spawned/commanded,
        otherwise its lazy connect-back attempt could fail.
        """
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self._host, self._port))
        srv.listen(1)
        srv.settimeout(self._ACCEPT_TIMEOUT)
        self._srv = srv

    def start(self) -> None:
        """Starts the background thread that accepts the engine and reads frames."""
        if self._srv is None:
            raise RuntimeError("bind() must be called before start()")
        self._running = True
        self._thread = threading.Thread(
            target=self._run, name="EngineEventListener", daemon=True
        )
        self._thread.start()

    def stop(self) -> None:
        """Stops the background thread and closes the listening socket."""
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=2.0)
            self._thread = None
        if self._srv is not None:
            try:
                self._srv.close()
            except OSError:
                pass
            self._srv = None

    def _run(self) -> None:
        assert self._srv is not None
        client: socket.socket | None = None
        try:
            while self._running:
                if client is None:
                    try:
                        client, _addr = self._srv.accept()
                        client.settimeout(self._RECV_TIMEOUT)
                    except socket.timeout:
                        continue
                    except OSError:
                        break

                try:
                    raw: str = recv_frame(client)
                except socket.timeout:
                    continue
                except (ConnectionError, OSError):
                    if self._on_connection_lost is not None:
                        self._on_connection_lost()
                    _close_quietly(client)
                    client = None
                    continue

                try:
                    msg: dict = json.loads(raw)
                except (json.JSONDecodeError, ValueError):
                    continue

                headers = msg.get("headers", {})
                if isinstance(headers, dict) and "data" in headers:
                    try:
                        msg["data"] = json.loads(headers["data"])
                    except (json.JSONDecodeError, TypeError, ValueError):
                        msg["data"] = headers["data"]
                elif "data" not in msg:
                    msg["data"] = {}

                self._on_envelope(msg)
        finally:
            _close_quietly(client)


def _close_quietly(sock: socket.socket | None) -> None:
    """Closes *sock* silently, ignoring any OSError."""
    if sock is not None:
        try:
            sock.close()
        except OSError:
            pass
