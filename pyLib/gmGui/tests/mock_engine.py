"""MockEngine — simulates the C++ Core Engine over TCP for E2E integration tests.

Wire roles (mirror of the real architecture):

- **Event sender** : TCP *client* that connects to :``event_port``
  (EngineReceiver's listening port) and sends engine→GUI event frames.
- **Command server**: TCP *server* that listens on :``cmd_port``
  (CmdServer equivalent) and receives GUI→engine command frames from
  EngineSender.

Both sides use the same length-prefix wire format (4-byte big-endian +
UTF-8 JSON) as ``gmDispatch::IpSocketChannel``.

Default test ports (well away from the real 9000/9001 pair):
    - ``DEFAULT_EVENT_PORT = 19000``  ← MockEngine connects here
    - ``DEFAULT_CMD_PORT   = 19001``  ← MockEngine listens here
"""
from __future__ import annotations

import json
import socket
import threading
import time

from gmGui.engine_bridge.framing import recv_frame, send_frame

# Default ports reserved for E2E tests.
DEFAULT_EVENT_PORT: int = 19000
DEFAULT_CMD_PORT: int = 19001


def _close(sock: socket.socket | None) -> None:
    """Closes *sock* silently, ignoring any OSError."""
    if sock is not None:
        try:
            sock.close()
        except OSError:
            pass


class MockEngine:
    """TCP mock that replaces the C++ Core Engine during integration tests.

    Lifecycle::

        engine = MockEngine()
        engine.start_cmd_server()       # bind :19001, start accept thread
        engine.connect_to_receiver()    # connect to :19000 (EngineReceiver)
        # --- run tests ---
        engine.stop()

    Convenience shortcut::

        engine = MockEngine()
        engine.start()                  # start_cmd_server + connect_to_receiver
        # --- run tests ---
        engine.stop()
    """

    def __init__(
        self,
        event_port: int = DEFAULT_EVENT_PORT,
        cmd_port: int = DEFAULT_CMD_PORT,
    ) -> None:
        self._event_port: int = event_port
        self._cmd_port: int = cmd_port

        # TCP client socket — sends events to EngineReceiver.
        self._event_sock: socket.socket | None = None

        # TCP server — receives commands from EngineSender.
        self._cmd_server: socket.socket | None = None
        self._cmd_client: socket.socket | None = None
        self._cmd_thread: threading.Thread | None = None

        self._running: bool = False
        self.received_commands: list[dict] = []
        self._cmd_lock: threading.Lock = threading.Lock()
        self._cmd_event: threading.Event = threading.Event()

    # ── Public API ────────────────────────────────────────────────────────────

    def start_cmd_server(self) -> None:
        """Binds the command server on *cmd_port* and starts its accept thread."""
        self._running = True
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("127.0.0.1", self._cmd_port))
        srv.listen(1)
        srv.settimeout(1.0)
        self._cmd_server = srv
        self._cmd_thread = threading.Thread(
            target=self._cmd_loop,
            daemon=True,
            name="MockEngine-CmdServer",
        )
        self._cmd_thread.start()

    def connect_to_receiver(self, timeout: float = 5.0) -> None:
        """Connects as TCP client to EngineReceiver, retrying until *timeout*.

        Raises:
            ConnectionRefusedError: If EngineReceiver does not accept within
                                    *timeout* seconds.
        """
        deadline = time.time() + timeout
        last_err: Exception | None = None
        while time.time() < deadline:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                sock.connect(("127.0.0.1", self._event_port))
                self._event_sock = sock
                return
            except OSError as exc:
                last_err = exc
                _close(sock)
                time.sleep(0.1)
        raise ConnectionRefusedError(
            f"MockEngine: could not connect to EngineReceiver on "
            f":{self._event_port} after {timeout:.1f}s — {last_err}"
        )

    def start(self, timeout: float = 5.0) -> None:
        """Convenience: ``start_cmd_server()`` + ``connect_to_receiver()``."""
        self.start_cmd_server()
        self.connect_to_receiver(timeout=timeout)

    def send_event(self, type_id: str, data: dict) -> None:
        """Sends one engine→GUI event frame using the ``headers["data"]`` convention.

        ``EngineReceiver`` normalises ``headers["data"]`` into ``msg["data"]``
        before emitting the Qt Signal, so all modules receive a consistent dict.

        Args:
            type_id: gmDispatch typeId string (e.g. ``"gmFlow.session.started"``).
            data:    JSON-serialisable payload dict.

        Raises:
            RuntimeError: If not yet connected (call :meth:`start` first).
        """
        if self._event_sock is None:
            raise RuntimeError(
                "MockEngine: not connected — call start() or connect_to_receiver() first"
            )
        envelope: dict = {
            "typeId": type_id,
            "source": "MockEngine",
            "headers": {"data": json.dumps(data)},
            "payload": "",
        }
        send_frame(self._event_sock, json.dumps(envelope))

    def wait_for_command(
        self,
        type_id: str,
        timeout: float = 3.0,
    ) -> dict | None:
        """Blocks until a command matching *type_id* arrives or *timeout* expires.

        Args:
            type_id: typeId string to match against collected commands.
            timeout: Maximum wait time in seconds.

        Returns:
            First matching command dict, or ``None`` on timeout.
        """
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._cmd_lock:
                for cmd in self.received_commands:
                    if cmd.get("typeId") == type_id:
                        return cmd
            remaining = max(0.0, deadline - time.time())
            self._cmd_event.wait(timeout=min(0.1, remaining))
            self._cmd_event.clear()
        return None

    def clear_commands(self) -> None:
        """Resets the collected commands list."""
        with self._cmd_lock:
            self.received_commands.clear()
        self._cmd_event.clear()

    def disconnect_event(self) -> None:
        """Closes the event socket, simulating an engine disconnection."""
        _close(self._event_sock)
        self._event_sock = None

    def reconnect_event(self, timeout: float = 5.0) -> None:
        """Re-establishes the event socket after a previous disconnect."""
        self.connect_to_receiver(timeout=timeout)

    def stop(self) -> None:
        """Stops all threads and closes all sockets."""
        self._running = False
        self.disconnect_event()
        _close(self._cmd_client)
        self._cmd_client = None
        _close(self._cmd_server)
        self._cmd_server = None
        if self._cmd_thread is not None and self._cmd_thread.is_alive():
            self._cmd_thread.join(timeout=3.0)

    # ── Internal ──────────────────────────────────────────────────────────────

    def _cmd_loop(self) -> None:
        """Accepts one EngineSender connection and reads command frames."""
        assert self._cmd_server is not None
        while self._running:
            try:
                client, _ = self._cmd_server.accept()
            except socket.timeout:
                continue
            except OSError:
                break

            self._cmd_client = client
            client.settimeout(1.0)
            while self._running:
                try:
                    raw = recv_frame(client)
                    msg: dict = json.loads(raw)
                    with self._cmd_lock:
                        self.received_commands.append(msg)
                    self._cmd_event.set()
                except socket.timeout:
                    continue
                except (ConnectionError, OSError, json.JSONDecodeError):
                    break
            _close(client)
            self._cmd_client = None
