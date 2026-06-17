"""EngineReceiver — QThread that listens for C++ engine events on TCP port 9000.

The C++ side connects using gmDispatch::IpSocketChannel("127.0.0.1", 9000).
Each arriving frame is deserialised from JSON and emitted as a Qt Signal so
that all widget updates happen on the Qt main thread.

Full implementation: Phase 2.
Phase 1 stub: run() exits immediately; Signals are declared but never emitted.
"""
from __future__ import annotations

from PySide6.QtCore import QThread, Signal


class EngineReceiver(QThread):
    """TCP server on port 9000 that receives engine events and emits Qt Signals.

    Usage::

        receiver = EngineReceiver()
        receiver.envelope_received.connect(window._on_envelope)
        receiver.start()          # starts the background thread
        ...
        receiver.stop()           # graceful shutdown

    Signals:
        envelope_received: Emitted for each valid JSON frame received.
                           Payload is a dict with keys:
                           ``typeId``, ``source``, ``data``, ``time``.
        connection_lost:   Emitted when the C++ client disconnects unexpectedly.
    """

    envelope_received: Signal = Signal(dict)
    connection_lost: Signal = Signal()

    def __init__(self, host: str = "127.0.0.1", port: int = 9000) -> None:
        super().__init__()
        self._host: str = host
        self._port: int = port
        self._running: bool = False

    def run(self) -> None:
        """Thread entry point.

        Phase 2: binds TCP server on *port*, accepts the C++ client connection,
        reads length-prefixed frames, deserialises JSON, and emits
        ``envelope_received`` for each message.

        Phase 1 stub: sets ``_running`` and returns immediately.
        """
        # TODO: Phase 2 — implement TCP server loop with framing.recv_frame()
        self._running = True
        # stub: no-op, exits immediately

    def stop(self) -> None:
        """Signals the run() loop to exit and waits for the thread to finish."""
        self._running = False
        self.wait()
