"""Dungeon Crawler Basic — engine bridge adapter.

DungeonBridge wraps the shared ``gmGui.engine_bridge`` framing layer and
exposes a simplified API for the GUI:

- ``receiver``: starts the event server on port 9200 (GUI is TCP server).
- ``sender``:   connects commands to the engine on port 9201 (Engine is server).
- ``send_command(type_id, data)``: helper to send a single JSON command.

Wire format: 4-byte big-endian length prefix + UTF-8 JSON payload, identical
to the format used by the Tic-Tac-Toe bridge and the C++ IpSocketChannel.
"""
from __future__ import annotations

import sys
from pathlib import Path
from typing import Callable

# Ensure pyLib is on sys.path (main.py already does this, but guard here too).
_PYLIB_DIR = Path(__file__).resolve().parents[3] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.append(str(_PYLIB_DIR))

from gmGui.engine_bridge.receiver import EngineReceiver  # noqa: E402
from gmGui.engine_bridge.sender   import EngineSender    # noqa: E402

# Ports must match CoreEngine DungeonTypes.hpp ports namespace.
_EVENTS_PORT   = 9200  # GUI is TCP server; Engine connects as client.
_COMMANDS_PORT = 9201  # Engine is TCP server; GUI connects as client.


class DungeonBridge:
    """Thin wrapper around the shared engine_bridge framing infrastructure.

    Provides a ``send_command`` convenience method and exposes the raw
    ``receiver`` and ``sender`` for callers that need direct access.
    """

    def __init__(self) -> None:
        """Creates receiver and sender but does not yet start them."""
        # ToBeImplemented //
        self._receiver = EngineReceiver(port=_EVENTS_PORT)
        self._sender   = EngineSender(host="127.0.0.1", port=_COMMANDS_PORT)

    @property
    def receiver(self) -> EngineReceiver:
        """The event receiver (listens on port 9200).

        Returns:
            The EngineReceiver instance.
        """
        return self._receiver

    @property
    def sender(self) -> EngineSender:
        """The command sender (connects to port 9201).

        Returns:
            The EngineSender instance.
        """
        return self._sender

    def send_command(self, type_id: str, data: dict) -> None:
        """Sends a single command to the CoreEngine.

        Args:
            type_id: Command typeId string (e.g. ``"dungeon.move"``).
            data:    Command payload dict.
        """
        self._sender.send_command(type_id, data)

    def set_on_event(self, handler: Callable[[dict], None]) -> None:
        """Registers the callback invoked for every incoming engine event.

        Args:
            handler: Callable receiving the decoded event dict.
        """
        # ToBeImplemented //
        self._receiver.on_message = handler
