"""engine_process — spawns and supervises the ``tris_engine`` subprocess.

Phase 1 scope: a single subprocess for the whole eng_serve instance.
Multi-session pooling (one subprocess per user session) is Phase 2's
responsibility — see GAME/Tic-Tac-Toe/WebApp/PLAN.md.
"""
from __future__ import annotations

import socket
import subprocess
import time
from pathlib import Path


class EngineProcessError(RuntimeError):
    """Raised when the engine subprocess cannot be started or reached."""


class EngineProcess:
    """Owns one running ``tris_engine`` subprocess and its lifecycle.

    Usage::

        engine = EngineProcess(executable, "127.0.0.1", 9301)
        engine.start()   # blocks until the command port is reachable
        ...
        engine.stop()
    """

    def __init__(
        self,
        executable: Path,
        command_host: str,
        command_port: int,
        connect_timeout_s: float = 10.0,
    ) -> None:
        self._executable: Path = executable
        self._command_host: str = command_host
        self._command_port: int = command_port
        self._connect_timeout_s: float = connect_timeout_s
        self._proc: subprocess.Popen | None = None

    @property
    def is_running(self) -> bool:
        """True if the subprocess was started and has not exited yet."""
        return self._proc is not None and self._proc.poll() is None

    def start(self) -> None:
        """Spawns the engine and waits until its command port accepts connections.

        Raises:
            EngineProcessError: If the executable is missing, or the command
                port does not become reachable within ``connect_timeout_s``.
        """
        if not self._executable.exists():
            raise EngineProcessError(
                f"Engine executable not found: {self._executable} "
                "(build it with: cmake --build build --target tris_engine --config Debug)"
            )
        self._proc = subprocess.Popen(
            [str(self._executable)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if not self._wait_for_command_port():
            self.stop()
            raise EngineProcessError(
                f"Engine did not open command port {self._command_port} "
                f"within {self._connect_timeout_s}s"
            )

    def stop(self) -> None:
        """Terminates the subprocess (``terminate()``, then ``kill()`` on timeout)."""
        if self._proc is None:
            return
        self._proc.terminate()
        try:
            self._proc.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            self._proc.kill()
        self._proc = None

    def _wait_for_command_port(self) -> bool:
        """Polls the command port with short-lived probe connections."""
        deadline = time.time() + self._connect_timeout_s
        while time.time() < deadline:
            try:
                probe = socket.create_connection(
                    (self._command_host, self._command_port), timeout=1.0
                )
                probe.close()
                return True
            except OSError:
                time.sleep(0.1)
        return False
