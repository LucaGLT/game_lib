"""engine_process — spawns and supervises one gmXxx engine subprocess.

Game-agnostic: the caller supplies the executable path, the command port to
poll, and any extra CLI arguments the specific engine needs (e.g. a data
directory). Session/process-pooling policy (one subprocess per whole
process vs. one per user session) is each game's own
``eng_serve/session_manager.py`` responsibility, not this module's.
"""
from __future__ import annotations

import socket
import subprocess
import time
from pathlib import Path


class EngineProcessError(RuntimeError):
    """Raised when the engine subprocess cannot be started or reached."""


class EngineProcess:
    """Owns one running game-engine subprocess and its lifecycle.

    Usage::

        engine = EngineProcess(executable, "127.0.0.1", 9211, extra_args=[str(data_dir)])
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
        extra_args: list[str] | None = None,
    ) -> None:
        self._executable: Path = executable
        self._command_host: str = command_host
        self._command_port: int = command_port
        self._connect_timeout_s: float = connect_timeout_s
        self._extra_args: list[str] = list(extra_args) if extra_args else []
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
                "(build it first with CMake)"
            )
        self._proc = subprocess.Popen(
            [str(self._executable), *self._extra_args],
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
