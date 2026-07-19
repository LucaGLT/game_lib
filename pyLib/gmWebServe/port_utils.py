"""port_utils — OS-assigned free TCP port allocation for per-session engines.

Used by :class:`gmWebServe.session_registry.SessionRegistry` to give every new
game session its own, previously-unused events/commands port pair, so several
engine subprocesses can run concurrently on the same machine (process-per-
session multi-user model).
"""
from __future__ import annotations

import socket

__all__ = ["find_free_port"]


def find_free_port(host: str = "127.0.0.1") -> int:
    """Asks the OS for an unused TCP port by binding to port 0 and releasing it.

    Args:
        host: Interface to probe on (must match the interface the real
            listener/server will later bind to).

    Returns:
        A TCP port that was free at the time of the call.

    Note:
        There is an inherent, small race between releasing the probe socket
        here and the real caller binding the same port number again — this is
        the standard "find a free port" technique (also used by most test
        frameworks) and is accepted as good-enough for pilot-grade, local,
        low-concurrency use. Callers should bind/spawn as soon as possible
        after calling this.
    """
    probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        probe.bind((host, 0))
        return probe.getsockname()[1]
    finally:
        probe.close()
